@echo off
setlocal

for %%I in ("%~dp0..\..") do set "PROJECT_ROOT=%%~fI"
set "PLATFORMIO_CORE_DIR=%PROJECT_ROOT%\.tools\platformio-core"
set "PIP_CACHE_DIR=%PROJECT_ROOT%\.tools\pip-cache"
set "PYTHON=%PROJECT_ROOT%\.venv\Scripts\python.exe"

if not exist "%PYTHON%" (
  echo Project virtual environment not found: "%PYTHON%" 1>&2
  exit /b 1
)

pushd "%PROJECT_ROOT%\firmware"
"%PYTHON%" -m platformio %*
set "RESULT=%ERRORLEVEL%"
popd
exit /b %RESULT%
