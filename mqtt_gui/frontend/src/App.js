import './App.css';
import { useState, useEffect } from "react";
import * as mqtt from 'mqtt/dist/mqtt.min';
import { Knob } from 'primereact/knob';
import Draggable from 'react-draggable';
import settings from "./settings.json";


function App() {
  const [client, setClient] = useState(null);
  const [connectStatus, setConnectStatus] = useState("Not Connected");
  const [isSub, setIsSub] = useState(false);
  const [ads_1015_min, setAds_1015_min] = useState(settings.ads_1015_min);
  const [ads_1015_max, setAds_1015_max] = useState(settings.ads_1015_max);
  const [ads_1115_min, setAds_1115_min] = useState(settings.ads_1115_min);
  const [ads_1115_max, setAds_1115_max] = useState(settings.ads_1115_max);
  const [showDisplaySettings, setShowDisplaySettings] = useState(false);

  const [boardData, setBoardData] = useState({
    b1_log_data_1015: {
      time: 0, 
      sensor_readings: [0, 0, 0, 0, 0],
    },
    b1_log_data_1115: {
      time: 0,
      sensor_readings: [0, 0, 0, 0 ,0]
    },
  });

  const [arrangable, setArrangable] = useState(false);
  
  const mqttSub = (topic) => {
    if (client) {
      client.subscribe(topic, (error) => {
        if (error) {
          console.log('Subscribe to topics error', error)
          return
        }
        setIsSub(true);
      });
    }
  };

  useEffect(()=> { 
    const mqttConnect = (host, mqttOption) => {
      setConnectStatus('Connecting');
      setClient(mqtt.connect(host, mqttOption));
    };

    const initialConnectionOptions = {
      protocol: 'ws',
      clientId: 'emqx_react_' + Math.random().toString(16).substring(2, 8),
      username: 'emqx_test',
      password: 'emqx_test',
    };
  
    const url = 'ws://localhost:9001';

    mqttConnect(url,initialConnectionOptions);
  }, []);

  useEffect(() => {
    if (client) {
      client.on('connect', () => {
        setConnectStatus('Connected');
        mqttSub("b1_log_data_1015");
        mqttSub("b1_log_data_1115");
      });

      client.on('error', (err) => {
        console.error('Connection error: ', err);
        client.end();
      });

      client.on('reconnect', () => {
        setConnectStatus('Reconnecting');
      });

      client.on('message', (topic, message) => {
        setBoardData((prevData)=>{
          prevData[topic] = message;
          return prevData;
        })
      });
    }
  }, [client]);


  return (
    <div className="App">
      <div className="settings">
        <p className="status">{connectStatus}</p>
        <p className="status">Recieving data: {isSub ? "True" : "False"}</p>
        <button onClick={()=>setShowDisplaySettings(!showDisplaySettings)} className="status">{showDisplaySettings ? "Hide Settings" : "Show Settings"}</button>
        { showDisplaySettings && (
          <div className="min_max_settings">
            <button onClick={()=>setArrangable(!arrangable)} className="status"> {arrangable ? "Stop Arranging" : "Arrange Dials"} </button>
            <p>
              Ads1015 Dial Minimum: &nbsp;
              <input className="number_input" type="number" onChange={(e)=>setAds_1015_min(e.target.value)} value={ads_1015_min}></input>
            </p>
          </div>
        )}
      </div>
      
      <Draggable disabled={!arrangable}>
        <div className="dial_cluster">
          <h3 className="readings_label">Board 1 ADS1015: Last updated at time {boardData.b1_log_data_1015.time}</h3>
          <div className="sensor_readings">
            { boardData.b1_log_data_1015.sensor_readings.map((reading)=>{
              return (
                  <div className="knob_container">
                    <Knob 
                      value={reading}
                      min = {ads_1015_min}
                      max = {ads_1015_max}
                    />
                  </div>
              );
            })}
          </div>
        </div>
      </Draggable>
      <Draggable disabled={!arrangable}>
        <div className="dial_cluster">
          <h3 className="readings_label">Board 1 ADS1115: Last updated at time {boardData.b1_log_data_1115.time}</h3>
          <div className="sensor_readings">
            { boardData.b1_log_data_1115.sensor_readings.map((reading)=>{
              return (
                <div className="knob_container">
                  <Knob 
                    value={reading}
                    min = {ads_1115_min}
                    max = {ads_1115_max}
                  />
                </div>
              );
            })}
          </div>
        </div>
      </Draggable>
    </div>
  );
}

export default App;
