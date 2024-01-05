@echo off
REM Startup Sound
if exist "startup.wav" (
    echo Running startup sound
    start "Startup Sound" powershell cmd "(New-Object Media.SoundPlayer startup.wav).Play()"
)


REM Start Node.js application in a new window
if exist "c:\Users\Admin\Documents\GitHub\PropMiniDaq\mqtt_gui\frontend" (
    start "NPM Frontend" cmd /k "cd c:\Users\Admin\Documents\GitHub\PropMiniDaq\mqtt_gui\frontend & npm start frontend"
) else (
    echo NPM project folder not found.
)

REM Wait for 1 seconds
timeout /t 1 /nobreak

REM Start Mosquitto MQTT broker in a new window
if exist "C:\PROGRA~1\mosquitto\mosquitto.exe" (
    start "Mosquitto Broker" powershell cmd /k "C:\PROGRA~1\mosquitto\mosquitto -v -c 'C:\PROGRA~1\mosquitto\mosquitto.conf' "
    echo Mosquitto running
) else (
    echo Mosquitto executable not found.
    goto end
)


REM Wait for 1 seconds
timeout /t 1 /nobreak


REM Activate Python virtual environment
if exist "c:\Users\Admin\Documents\GitHub\PropMiniDaq\Python Code\Prop PCB DAQ Logger\.daqvenv\bin\Activate.ps1" (
    start "Activate daqvenv"  cmd /k "c:\Users\Admin\Documents\GitHub\PropMiniDaq\Python Code\Prop PCB DAQ Logger\.daqvenv\bin\Activate.ps1 & cd c:\Users\Admin\Documents\GitHub\PropMiniDaq\mqtt_gui & py c:\Users\Admin\Documents\GitHub\PropMiniDaq\mqtt_gui\main.py"
) else (
    echo Python virtual environment activation script not found.
    goto end
)

:end
echo "Running... 5 Seconds Waiting for NPM to start"
REM Wait for 5 seconds
timeout /t 5 /nobreak
start msedge --start-fullscreen http://localhost:3000/
pause