{    
    Write-Output 'Starting MQTT Server'
    C:\PROGRA~1\mosquitto\mosquitto -v -c C:\PROGRA~1\mosquitto\mosquitto.conf
    wt -w 0 nt pwsh -NoExit -c{
        Write-Output 'Starting Backend'
        & 'c:\Users\Admin\Documents\GitHub\PropMiniDaq\Python Code\Prop PCB DAQ Logger\.daqvenv\bin\Activate.ps1'
        python main.py
    }
    wt -w 0 nt pwsh -NoExit -c{
        Write-Output 'Starting Frontend'
        cd frontend
        npm start 
    }

}