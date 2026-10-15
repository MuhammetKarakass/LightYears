@echo off
rem Configures (first run) and builds <repo>\build as Debug with Ninja and MSVC 2022.
rem Extra arguments go to "cmake --build", for example: tools\vega-build.cmd --target LightYearsGame
rem
rem First configure: when LY_DEPS_SEED (default D:\LightYears\build\_deps) holds already fetched dependency sources,
rem they are copied into build\_deps and FetchContent runs disconnected, so nothing is downloaded. The copy keeps
rem SFML's ${FETCHCONTENT_BASE_DIR}-relative unity build exclusions matching; FETCHCONTENT_SOURCE_DIR_* overrides break
rem them (vorbis/freetype unity sources then fail to compile). Without a seed, FetchContent downloads as usual.
setlocal EnableDelayedExpansion
set VSLANG=1033
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
for %%I in ("%~dp0..") do set "REPO=%%~fI"
if "%LY_DEPS_SEED%"=="" set "LY_DEPS_SEED=D:\LightYears\build\_deps"
if not exist "%REPO%\build\build.ninja" (
  set "DISCONNECTED=OFF"
  if exist "%LY_DEPS_SEED%\sfml-src" (
    for %%N in (box2d flac freetype harfbuzz json libssh2 mbedtls ogg sfml sheenbidi vorbis) do (
      if not exist "%REPO%\build\_deps\%%N-src" robocopy "%LY_DEPS_SEED%\%%N-src" "%REPO%\build\_deps\%%N-src" /E /NFL /NDL /NJH /NJS /NP >nul
    )
    rem The editor's dependencies are not in the game's seed; fetch the same pinned archives VegaEditor/CMakeLists.txt names.
    call :fetch_zip imgui https://github.com/ocornut/imgui/archive/refs/tags/v1.91.6-docking.zip imgui-1.91.6-docking
    if errorlevel 1 exit /b 1
    call :fetch_zip imgui-sfml https://github.com/SFML/imgui-sfml/archive/e8ee3667f1b1f731e5c9c32be4c544915291f987.zip imgui-sfml-e8ee3667f1b1f731e5c9c32be4c544915291f987
    if errorlevel 1 exit /b 1
    rem Scripting dependencies pinned in VegaScript/CMakeLists.txt; tar also unpacks the Lua .tar.gz.
    call :fetch_zip lua https://www.lua.org/ftp/lua-5.4.7.tar.gz lua-5.4.7
    if errorlevel 1 exit /b 1
    call :fetch_zip sol2 https://github.com/ThePhD/sol2/archive/refs/tags/v3.5.0.zip sol2-3.5.0
    if errorlevel 1 exit /b 1
    set "DISCONNECTED=ON"
  )
  cmake -S "%REPO%" -B "%REPO%\build" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DFETCHCONTENT_FULLY_DISCONNECTED=!DISCONNECTED!
  if errorlevel 1 exit /b 1
)
cmake --build "%REPO%\build" %*
exit /b %errorlevel%

rem :fetch_zip <name> <url> <top-level folder inside the zip>
:fetch_zip
if exist "%REPO%\build\_deps\%~1-src" exit /b 0
set "FETCH_TMP=%REPO%\build\_deps\%~1-download"
if exist "%FETCH_TMP%" rmdir /s /q "%FETCH_TMP%"
mkdir "%FETCH_TMP%"
curl -sSL -o "%FETCH_TMP%\src.zip" "%~2" || exit /b 1
tar -xf "%FETCH_TMP%\src.zip" -C "%FETCH_TMP%" || exit /b 1
move "%FETCH_TMP%\%~3" "%REPO%\build\_deps\%~1-src" >nul || exit /b 1
rmdir /s /q "%FETCH_TMP%"
exit /b 0
