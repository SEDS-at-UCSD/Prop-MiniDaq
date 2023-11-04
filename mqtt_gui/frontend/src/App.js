import './App.css';
import { useState, useEffect } from "react";
import * as mqtt from 'mqtt/dist/mqtt.min';
import { Knob } from 'primereact/knob';
import Draggable from 'react-draggable';

function App() {
  const [client, setClient] = useState(null);
  const [connectStatus, setConnectStatus] = useState("Not Connected");
  const [isSub, setIsSub] = useState(false);
  const ads_1015_min = useState(0);
  const ads_1015_max = useState(100);

  const [board1Data, setBoard1Data] = useState({
    ads_1015: {
      time: 0, 
      sensor_readings: [0, 0, 0, 0, 0],
    },
    ads_1115: {
      time: 0,
      sensor_readings: [0, 0, 0, 0 ,0]
    }
  });
  // const [altitudeData, setAltitudeData] = useState([{"altitude": 0, "time": 0}]);
  // const [currentAltitude, setCurrentAltitude] = useState({"altitude": 0, "time": 0});
  // const [plotAltitudes,setPlotAltitudes] = useState([]);
  // const [plotTimes,setPlotTimes] = useState([]);
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
      });
      client.on('error', (err) => {
        console.error('Connection error: ', err);
        client.end();
      });
      client.on('reconnect', () => {
        setConnectStatus('Reconnecting');
      });
      client.on('message', (topic, message) => {
        // setData(message);
      });
    }
  }, [client]);


  return (
    <div className="App">
      <p>{connectStatus}</p>
      <p>Recording data: {isSub ? "True" : "False"}</p>
      {!isSub && <button onClick={()=>mqttSub("log_data_1015")} disabled={!(connectStatus === "Connected")}>Start Recording Data</button>}
      <button onClick={()=>setArrangable(!arrangable)}> {arrangable ? "Stop Arranging" : "Arrange"} </button>

        { board1Data.ads_1015.sensor_readings.map((reading)=>{
          return (
          <Draggable disabled={!arrangable}>
            <div>
              <Knob 
                value={reading}
                min = {ads_1015_min}
                max = {ads_1015_max}
              />
            </div>
          </Draggable>)
        })}
    </div>
  );
}

export default App;
