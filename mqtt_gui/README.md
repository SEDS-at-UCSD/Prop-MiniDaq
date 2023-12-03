# Set Up Instructions
  
## Non Front End Dependencies
Install Node.js \
pyserial \
paho-mqtt 

## Front End Dependencies
run `npm install` 

## MQTT Server Download and Config
Download mosquitto from https://mosquitto.org/download/

### Make following changes to config file `mosquitto.conf`
In the "choose the protocol to use when listening section" add:
```
listener 1883
protocol mqtt

listener 9001
protocol websockets
```

Uncomment `allow_anonymous` and set to true as such:\
`allow_anonymous true`


## Order of Operations
1. **Run Local MQTT Server** on one terminal window\
   Run mqtt server with config file\
   Unix/Linux Command:\
   `/opt/homebrew/opt/mosquitto/sbin/mosquitto -c /opt/homebrew/etc/mosquitto/mosquitto.conf`\
   Windows Command:\
   `C:\PROGRA~1\mosquitto\mosquitto -v -c C:\PROGRA~1\mosquitto\mosquitto.conf`\

   match addresses of mosquitto and mosquitto.conf with local addresses
2. **Run main.py** in separate terminal window\
   `cd mqtt_gui`\
   `./main.py`\
    [Make sure PCB is plugged in]
3. **Run Frontend** in separate terminal window\
   `cd mqtt_gui/frontend`\
   `npm start`\
    Access GUI on `localhost:3000`


## Other Instructions
- Change conversion factor in config.json
   
   
   
