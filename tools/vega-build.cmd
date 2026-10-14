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
    set "DISCONNECTED=ON"
  )
  cmake -S "%REPO%" -B "%REPO%\build" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DFETCHCONTENT_FULLY_DISCONNECTED=!DISCONNECTED!
  if errorlevel 1 exit /b 1
)
cmake --build "%REPO%\build" %*
exit /b %errorlevel%
