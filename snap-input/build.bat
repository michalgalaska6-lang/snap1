@echo off
setlocal
echo === Survivable Input Snap: build ===

where geode >nul 2>nul
if errorlevel 1 (
  echo Instaluje Geode CLI, CMake i narzedzia C++...
  winget install Kitware.CMake
  winget install Microsoft.VisualStudio.2022.BuildTools --override "--add Microsoft.VisualStudio.Workload.VCTools --includeRecommended --passive"
  winget install GeodeSDK.GeodeCLI
  echo.
  echo Zamknij to okno, otworz nowe i uruchom build.bat jeszcze raz.
  pause
  exit /b
)

if not defined GEODE_SDK (
  geode sdk install
  echo.
  echo SDK zainstalowane. Zamknij okno, otworz nowe i uruchom build.bat jeszcze raz.
  pause
  exit /b
)

if not exist "%LOCALAPPDATA%\Geode\geode-cli\config.json" (
  echo Konfiguracja profilu - podaj sciezke do folderu z Geometry Dash...
  geode config setup
)

geode sdk install-binaries
geode build
echo.
echo Gotowe. Plik .geode jest w folderze build (albo juz zainstalowany w grze).
pause
