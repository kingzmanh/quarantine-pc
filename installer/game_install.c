/* What the installer does that is particular to Quarantine.
 *
 * The CD is copied whole to <install>\game\cd (the game's D: drive).
 * <install>\game\hd is the game's C: drive, laid out like the DOS INSTALL.EXE
 * "complete install" (everything from the CD's root except the two intro
 * AVIs, which the Duck player reads from the CD) plus CD.DAT, the file the
 * DOS installer wrote with the CD drive's path.  SOUND.DAT comes from the CD
 * as "no sound card": the PC plays the sound itself.
 * The music is Red Book audio (tracks 2-12): ripped from the image into
 * <install>\game\cdaudio\trackNN.wav with the disc's real table of contents
 * in toc.txt (src/rt/cdaudio.c plays them). */
#include "setup.h"
#include <string.h>
#include <stdlib.h>
#include "install.h"

int game_is_cd(CdSrc *c)
{
    static const char *files[] = GAME_CD_FILES;
    if (!c) return 0;
    for (int i = 0; files[i]; i++)
        if (!cd_exists(c, files[i])) return 0;
    return 1;
}

int game_install_files(const char *cd, const char *hd)
{
    char pat[MAX_PATH * 2];
    snprintf(pat, sizeof pat, "%s\\*", cd);
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pat, &fd);
    if (h == INVALID_HANDLE_VALUE) { setup_log("ERROR: the copied CD is empty"); return 0; }
    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        const char *ext = strrchr(fd.cFileName, '.');
        if (ext && !_stricmp(ext, ".avi")) continue;
        char s[MAX_PATH * 3], d[MAX_PATH * 3];
        snprintf(s, sizeof s, "%s\\%s", cd, fd.cFileName);
        snprintf(d, sizeof d, "%s\\%s", hd, fd.cFileName);
        if (!copy_file(s, d, NULL)) { FindClose(h); setup_log("ERROR: cannot write %s", d); return 0; }
        SetFileAttributesA(d, FILE_ATTRIBUTE_NORMAL);
    } while (FindNextFileA(h, &fd));
    FindClose(h);
    char p[MAX_PATH * 2];
    snprintf(p, sizeof p, "%s\\CD.DAT", hd);
    return write_file(p, "D:\\\r\n", 5);
}

/* the CD's audio tracks -> game\cdaudio (bytes it will write, for progress) */
u64 game_audio_bytes(CdSrc *c)
{
    CdTrack t[100];
    int n = cd_tracks(c, t, 100);
    u64 b = 0;
    for (int i = 0; i < n; i++) if (t[i].audio) b += (u64)t[i].nsec * 2352 + 44;
    return b;
}

int game_install_audio(CdSrc *c, const char *game, Progress *pr)
{
    CdTrack t[100];
    int n = cd_tracks(c, t, 100);
    char dir[MAX_PATH * 2], p[MAX_PATH * 3];
    snprintf(dir, sizeof dir, "%s\\cdaudio", game);
    int naudio = 0;
    for (int i = 0; i < n; i++) naudio += t[i].audio;
    if (!naudio) {
        setup_log("No CD audio tracks in this source - the game's music needs an image with its tracks (.nrg or .cue/.bin).");
        return 1;
    }
    make_dirs(dir);
    char toc[4096];
    int tl = snprintf(toc, sizeof toc, "# track start_lba sectors (track 1 = data)\r\n");
    u32 leadout = 0;
    static u8 buf[75 * 2352];
    for (int i = 0; i < n; i++) {
        tl += snprintf(toc + tl, sizeof toc - tl, "%d %u %u\r\n", t[i].no, t[i].lba, t[i].nsec);
        if (t[i].lba + t[i].nsec > leadout) leadout = t[i].lba + t[i].nsec;
        if (!t[i].audio) continue;
        snprintf(p, sizeof p, "%s\\track%02d.wav", dir, t[i].no);
        HANDLE out = CreateFileA(p, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (out == INVALID_HANDLE_VALUE) { setup_log("ERROR: cannot write %s", p); return 0; }
        u32 data = t[i].nsec * 2352;
        u8 hd[44] = {'R', 'I', 'F', 'F', 0, 0, 0, 0, 'W', 'A', 'V', 'E', 'f', 'm', 't', ' ', 16, 0, 0, 0,
                     1, 0, 2, 0, 0x44, 0xac, 0, 0, 0x10, 0xb1, 2, 0, 4, 0, 16, 0, 'd', 'a', 't', 'a'};
        u32 riff = data + 36;
        memcpy(hd + 4, &riff, 4);
        memcpy(hd + 40, &data, 4);
        DWORD w;
        WriteFile(out, hd, 44, &w, NULL);
        for (u32 s = 0; s < t[i].nsec; s += 75) {
            u32 k = t[i].nsec - s < 75 ? t[i].nsec - s : 75;
            if (!cd_read_raw(c, i, s, k, buf)) { CloseHandle(out); setup_log("ERROR: reading audio track %d failed", t[i].no); return 0; }
            if (!WriteFile(out, buf, k * 2352, &w, NULL) || w != k * 2352) { CloseHandle(out); setup_log("ERROR: cannot write %s", p); return 0; }
            if (pr) progress_add(pr, k * 2352);
        }
        CloseHandle(out);
    }
    tl += snprintf(toc + tl, sizeof toc - tl, "leadout %u\r\n", leadout);
    snprintf(p, sizeof p, "%s\\toc.txt", dir);
    setup_log("CD music: %d tracks", naudio);
    return write_file(p, toc, (size_t)tl);
}

const char *game_icon_lbm(void) { return GAME_ICON_LBM; }
