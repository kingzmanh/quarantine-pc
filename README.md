# Quarantine - PC Native

A native Windows version of *Quarantine*. No DOSBox: the game's own code is converted from x86
machine code to C by a static recompiler and runs on a small PC runtime that replaces DOS, the DOS
extender, the BIOS, the sound and video hardware with Windows services.

**This repository contains no game files and no game code.** You need your own copy of the game.

## Installing

Run **`Quarantine PC Setup.exe`**, point it at your CD (disc, ISO / BIN+CUE image, or a copied folder),
choose a folder, press Install.

## Playing

- **Alt+Enter** switches fullscreen / window (remembered in `quarantine_pc.ini`).
- **Video menu** (menu bar of the window): upscale filter (xBR default), sharpening, FXAA, aspect
  ratio, widescreen (zoom to the picture / fill), window size, FPS.

## Building

Windows Python with capstone + Pillow, MSYS2 MINGW64 (gcc, SDL2), Ghidra for the function lists.

```bash
python dosport.py ghidra     # function lists -> re/<tag>/functions.tsv (needs gamedata/cd)
./build.sh gen               # recompile + build
./run_test.sh 30             # hidden test run (env prefix QU_: QU_SHOT=ms, QU_KEYS=..., see KNOWLEDGE.md of the Dos tools kit)
make release                 # "Quarantine PC Setup.exe"
```

Made with the Dos tools kit (from the Aliens: A Comic Book Adventure and Alien Odyssey ports).
