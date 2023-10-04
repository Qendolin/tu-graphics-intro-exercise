@ECHO OFF
setlocal

vcvars64

set PROJECT_DIR=_project
set INSTALL_DIR=_bin
@REM I just couldn't get mingw64 to work
@REM set CMAKE_OPTIONS=-G "MinGW Makefiles" --verbose -DCMAKE_MAKE_PROGRAM=mingw32-make -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++
set CMAKE_OPTIONS=-A x64

if "%~1"=="" goto BLANK
if "%~1"=="debug" goto DEBUG
if "%~1"=="release" goto RELEASE
if "%~1"=="install" goto INSTALL
if "%~1"=="clean" goto CLEAN
@ECHO ON

:BLANK
cmake %CMAKE_OPTIONS% -S. -B %PROJECT_DIR% -DCMAKE_INSTALL_PREFIX=%INSTALL_DIR%
GOTO DONE

:DEBUG
cmake %CMAKE_OPTIONS% -S. -B %PROJECT_DIR% -DCMAKE_INSTALL_PREFIX=%INSTALL_DIR%
cmake --build %PROJECT_DIR% --config Debug --target install
GOTO DONE

:RELEASE
cmake %CMAKE_OPTIONS% -S. -B %PROJECT_DIR% -DCMAKE_INSTALL_PREFIX=%INSTALL_DIR%
cmake --build %PROJECT_DIR% --config Release --target install
GOTO DONE

:INSTALL
cmake %CMAKE_OPTIONS% -S. -B %PROJECT_DIR% -DCMAKE_INSTALL_PREFIX=%INSTALL_DIR%
cmake --build %PROJECT_DIR% --config Debug --target install
cmake --build %PROJECT_DIR% --config Release --target install
GOTO DONE

:CLEAN
rmdir /Q /S %PROJECT_DIR% 2>NUL
rmdir /Q /S %INSTALL_DIR% 2>NUL
GOTO DONE

:DONE
endlocal