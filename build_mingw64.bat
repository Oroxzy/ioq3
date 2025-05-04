@echo off
setlocal

REM MSYS2-Installationspfad
set MSYS2_ROOT=C:\msys64

REM Verzeichnis dieser .bat-Datei
set PROJECT_DIR=%~dp0

REM Optimierte Release-CFLAGS
set "CFLAGS=-O3 -march=native -DNDEBUG"

REM Starte bash über mintty, baue Projekt, halte Fenster offen
%MSYS2_ROOT%\mingw64.exe -e /bin/bash -l -c "cd '%PROJECT_DIR%' && echo === Building in $PWD === && make clean && make CFLAGS='%CFLAGS%' -j$(nproc); echo; echo *** Build fertig. ENTER zum Schließen... ***; read"

endlocal
