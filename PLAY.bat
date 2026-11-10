@echo off
rem Quarantine - PC Native.  Add --state 3 after quarantine.exe to skip the videos.
cd /d "%~dp0"
start "" quarantine.exe %*
