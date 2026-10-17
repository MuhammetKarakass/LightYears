@echo off
rem Build LightYears with a sibling Vega checkout. Extra arguments go to cmake --build.
rem VEGA_ENGINE_DIR defaults to ..\Vega; VEGA_CMAKE_ARGS supplies configure flags.
rem Optional VEGA_DEPS_SEED contains FetchContent *-src directories; no generated builds are copied.
setlocal EnableDelayedExpansion
set VSLANG=1033
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
for %%I in ("%~dp0..") do set "REPO=%%~fI"
if "%VEGA_ENGINE_DIR%"=="" set "VEGA_ENGINE_DIR=%REPO%\..\Vega"
if "%VEGA_DEPS_SEED%"=="" set "VEGA_DEPS_SEED=%VEGA_ENGINE_DIR%\build\_deps"
set "BUILD_CONFIG=Debug"
set "BUILD_ARGS=%*"
if "%~1"=="--release" (
  set "BUILD_CONFIG=Release"
  set "BUILD_ARGS=!BUILD_ARGS:~9!"
  if "%VEGA_BUILD_DIR%"=="" set "VEGA_BUILD_DIR=%REPO%\build-release"
)
if "%VEGA_BUILD_DIR%"=="" set "VEGA_BUILD_DIR=%REPO%\build"
if exist "%VEGA_DEPS_SEED%" (
  for /d %%D in ("%VEGA_DEPS_SEED%\*-src") do (
    if not exist "%VEGA_BUILD_DIR%\_deps\%%~nxD" (
      robocopy "%%D" "%VEGA_BUILD_DIR%\_deps\%%~nxD" /E /R:1 /W:1 /NFL /NDL /NJH /NJS /NP >nul
      if errorlevel 8 exit /b 1
    )
  )
)
set "DISCONNECTED=OFF"
if exist "%VEGA_BUILD_DIR%\_deps\sfml-src" set "DISCONNECTED=ON"
cmake -S "%REPO%" -B "%VEGA_BUILD_DIR%" -G Ninja -DCMAKE_BUILD_TYPE=!BUILD_CONFIG! -DVEGA_ENGINE_DIR="%VEGA_ENGINE_DIR%" -DFETCHCONTENT_FULLY_DISCONNECTED=!DISCONNECTED! %VEGA_CMAKE_ARGS%
if errorlevel 1 exit /b 1
cmake --build "%VEGA_BUILD_DIR%" !BUILD_ARGS!
exit /b %errorlevel%
