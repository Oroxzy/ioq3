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
set "DOTNET_EXE="
for %%I in (dotnet.exe) do set "DOTNET_EXE=%%~$PATH:I"
if not defined DOTNET_EXE if exist "%ProgramFiles%\dotnet\dotnet.exe" set "DOTNET_EXE=%ProgramFiles%\dotnet\dotnet.exe"
if not defined DOTNET_EXE (echo .NET SDK nicht gefunden & goto :error)
set "GIT_BIN="
for %%I in (git.exe) do set "GIT_BIN=%%~dp$PATH:I"
set "PATH=%MSYS2_ROOT%\mingw64\bin;%SystemRoot%\System32;%SystemRoot%;%SystemRoot%\System32\Wbem;%GIT_BIN%"

echo === Building in "%BUILD_DIR%" ===
"%CMAKE_EXE%" -S "%PROJECT_DIR%." -B "%BUILD_DIR%" -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS="%EXTRA_CFLAGS%" || goto :error
"%CMAKE_EXE%" --build "%BUILD_DIR%" --clean-first -j %NUMBER_OF_PROCESSORS% || goto :error
"%CMAKE_EXE%" --install "%BUILD_DIR%" --prefix "%GAME_DIR%" || goto :error

REM qagame und ui zusaetzlich als pk3: Bei sv_pure 1 laedt der lokale Server nach einem Mapwechsel
REM nur noch QVMs aus pk3s, und zz-hitpitch.pk3 wird vor pak8.pk3 durchsucht
pushd "%GAME_DIR%\baseq3" || goto :error
"%CMAKE_EXE%" -E tar cf zz-hitpitch.pk3 --format=zip vm/qagame.qvm vm/ui.qvm
set "PK3_ERROR=%ERRORLEVEL%"
REM Die losen Kopien loeschen: sie werden vor jedem pk3 gefunden, dadurch meldet der
REM Server fuer vm/ui.qvm die Pruefsumme aus zz-hitpitch.pk3, bietet das pk3 aber nie
REM zum Download an - fremde Spieler fliegen dann als "Unpure Client" raus
if exist vm\qagame.qvm del vm\qagame.qvm
if exist vm\ui.qvm del vm\ui.qvm
popd
if not "%PK3_ERROR%"=="0" goto :error

REM Der Quake-Champions-Trefferton. Die Engine sucht ihn als sound/feedback/hit_qc.wav,
REM also muss er in einem pk3 liegen - lose Dateien findet der Server bei sv_pure nicht.
pushd "%PROJECT_DIR%assets\hitsound-qc" || goto :error
"%CMAKE_EXE%" -E tar cf "%GAME_DIR%\baseq3\zz-hitsound-qc.pk3" --format=zip sound
set "PK3_ERROR=%ERRORLEVEL%"
popd
if not "%PK3_ERROR%"=="0" goto :error

REM Die Silhouetten-Shader fuer die Gegner-Markierung. Wie der Trefferton muessen
REM sie in einem pk3 liegen, damit der lokale Server sie bei sv_pure findet.
pushd "%PROJECT_DIR%assets\bot-silhouette" || goto :error
"%CMAKE_EXE%" -E tar cf "%GAME_DIR%\baseq3\zz-bot-silhouette.pk3" --format=zip scripts
set "PK3_ERROR=%ERRORLEVEL%"
popd
if not "%PK3_ERROR%"=="0" goto :error

REM Die WinForms-App gehoert zum gleichen Pruefstand und wird ebenfalls frisch gebaut.
echo === Building Trefferton-Labor ===
"%DOTNET_EXE%" build "%PROJECT_DIR%tools\hitsound-lab\HitsoundLab.csproj" --configuration Release --no-incremental || goto :error

echo.
echo *** Build fertig. Starten mit: "%GAME_DIR%\ioquake3.exe" ***
echo *** Trefferton-Labor: "%PROJECT_DIR%tools\hitsound-lab\bin\Release\net10.0-windows\HitsoundLab.exe" ***
if /I "%~1"=="--no-pause" exit /b 0
echo *** ENTER zum Schliessen... ***
pause >nul
exit /b 0

:error
echo.
if /I "%~1"=="--no-pause" exit /b 1
echo *** Build fehlgeschlagen. ENTER zum Schliessen... ***
pause >nul
exit /b 1
