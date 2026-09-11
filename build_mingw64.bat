@echo off
setlocal

REM MSYS2-Installationspfad (liefert gcc und mingw32-make)
set "MSYS2_ROOT=C:\msys64"

REM Verzeichnis dieser .bat-Datei
set "PROJECT_DIR=%~dp0"

REM CMake-Build-Ordner und Spiel-Ordner mit den pk3s, in den installiert wird
set "BUILD_DIR=%PROJECT_DIR%build\cmake-mingw64"
if not defined GAME_DIR set "GAME_DIR=%PROJECT_DIR%build\release-mingw64-x86_64"

REM Eigener Temp-Ordner: Zeigt TEMP auf C:\Windows\Temp (z.B. beim Start aus einer App
REM mit System-Umgebung), findet ld die Response-Files von gcc nicht, weil normale User
REM diesen Ordner nicht auflisten duerfen
set "TMP=%BUILD_DIR%\tmp"
set "TEMP=%BUILD_DIR%\tmp"
if not exist "%TMP%" mkdir "%TMP%"

REM Zusaetzliche CFLAGS, Release bringt schon -O3 -DNDEBUG mit
set "EXTRA_CFLAGS=-march=native"

REM CMake suchen, bevor PATH neu gesetzt wird. Ist PATH laenger als 8191 Zeichen, findet
REM cmd keine Programme mehr und %PATH% wird leer, deshalb PATH kurz halten statt verlaengern.
REM git kommt nur fuer die Versionsnummer dazu.
set "CMAKE_EXE="
for %%I in (cmake.exe) do set "CMAKE_EXE=%%~$PATH:I"
if not defined CMAKE_EXE if exist "%ProgramFiles%\CMake\bin\cmake.exe" set "CMAKE_EXE=%ProgramFiles%\CMake\bin\cmake.exe"
if not defined CMAKE_EXE (echo CMake nicht gefunden & goto :error)
set "GIT_BIN="
for %%I in (git.exe) do set "GIT_BIN=%%~dp$PATH:I"
set "PATH=%MSYS2_ROOT%\mingw64\bin;%SystemRoot%\System32;%SystemRoot%;%SystemRoot%\System32\Wbem;%GIT_BIN%"

echo === Building in "%BUILD_DIR%" ===
"%CMAKE_EXE%" -S "%PROJECT_DIR%." -B "%BUILD_DIR%" -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS="%EXTRA_CFLAGS%" || goto :error
"%CMAKE_EXE%" --build "%BUILD_DIR%" -j %NUMBER_OF_PROCESSORS% || goto :error
"%CMAKE_EXE%" --install "%BUILD_DIR%" --prefix "%GAME_DIR%" || goto :error

REM qagame zusaetzlich als pk3: Bei sv_pure 1 laedt der lokale Server nach einem Mapwechsel
REM nur noch QVMs aus pk3s, und zz-hitpitch.pk3 wird vor pak8.pk3 durchsucht
pushd "%GAME_DIR%\baseq3" || goto :error
"%CMAKE_EXE%" -E tar cf zz-hitpitch.pk3 --format=zip vm/qagame.qvm
set "PK3_ERROR=%ERRORLEVEL%"
popd
if not "%PK3_ERROR%"=="0" goto :error

echo.
echo *** Build fertig. Starten mit: "%GAME_DIR%\ioquake3.exe" ***
echo *** ENTER zum Schliessen... ***
pause >nul
exit /b 0

:error
echo.
echo *** Build fehlgeschlagen. ENTER zum Schliessen... ***
pause >nul
exit /b 1
