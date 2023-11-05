import './App.css';
import { useState, useEffect } from "react";
import * as mqtt from 'mqtt/dist/mqtt.min';
import { DialCluster } from './components/DialCluster';


function App() {
  const [client, setClient] = useState(null);
  const [connectStatus, setConnectStatus] = useState("Not Connected");
  const [isSub, setIsSub] = useState(false);
  const [showDisplaySettings, setShowDisplaySettings] = useState(false);

  const [b1_data_1015, setB1_1015Data] = useState({
    time: 0, 
    sensor_readings: [0, 0, 0, 0, 0]
  });
  
  const [b1_data_1115, setB1_1115Data] = useState({
    time: 0, 
    sensor_readings: [0, 0, 0, 0, 0]
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

        message = JSON.parse(String(message));
        console.log(message);
        if (topic === 'b1_log_data_1015') {
          setB1_1015Data(message);
        } else {
          setB1_1115Data(message);
        }
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
            {/* <p>
              Ads1015 Dial Minimum: &nbsp;
              <input className="number_input" type="number" onChange={(e)=>setAds_1015_min(e.target.value)} value={ads_1015_min}></input>
            </p> */}
          </div>
        )}
      </div>
      
      <DialCluster 
        label="Board 1 ADS1015"
        data={b1_data_1015}
        arrangable
        sensor_name="ads_1015"
      />

      <DialCluster 
        label="Board 1 ADS1115"
        data={b1_data_1115}
        arrangable
        sensor_name="ads_1115"
      />
    </div>
  );
}

export default App;
