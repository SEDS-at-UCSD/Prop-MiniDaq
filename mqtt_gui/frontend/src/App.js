import './App.css';
import { useState, useEffect } from "react";
import * as mqtt from 'mqtt/dist/mqtt.min';
import { DialCluster } from './components/DialCluster';
import { SwitchConfigure } from './components/SwitchConfigure';


function App() {
  const [client, setClient] = useState(null);
  const [connectStatus, setConnectStatus] = useState("Not Connected");
  const [isSub, setIsSub] = useState(false);
  const [configureSwitches, setConfigureSwitches] = useState(false);

  const initialState = {time: 0, sensor_readings: [0, 0, 0, 0]}
  
  const [b1_data_1015, setB1_1015Data] = useState(initialState);
  const [b1_data_1115, setB1_1115Data] = useState(initialState);
  const [b2_data_1015, setB2_1015Data] = useState(initialState);
  const [b2_data_1115, setB2_1115Data] = useState(initialState);
  const [b3_data_1015, setB3_1015Data] = useState(initialState);
  const [b3_data_1115, setB3_1115Data] = useState(initialState);

  const [switchStates, setSwitchStates] = useState({
    0: 0,
    1: 0,
    2: 0,
    3: 0,
    4: 0
  });

  const [arrangable, setArrangable] = useState(false);
  
  const mqttSub = (topic) => {
    if (client) {
      client.subscribe(topic, (error) => {
        if (error) {
          console.log("Subscribe to " + topic + " error", error)
          return
        }
        setIsSub(true);
      });
    }
  };

  const sendMessage = (topic, message) => {
    console.log(message);
    client.publish(topic,message,(error)=>{
      if (error) {
        console.log("Publish to " + topic + " error", error)
        return
      }
    })
  }

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
        mqttSub("switch_states_status");
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
        if (topic === 'b1_log_data_1015') {
          setB1_1015Data(message);
        } else if (topic === "switch_states_status") {
          console.log("here");
          setSwitchStates((prev)=>{return {...prev, ...message}});
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
        <div className="min_max_settings">
          <button onClick={()=>setArrangable(!arrangable)} className="status"> {arrangable ? "Stop Arranging" : "Arrange Dials"} </button>
        </div>
        <button onClick={()=>setConfigureSwitches(!configureSwitches)} className="status"> {configureSwitches ? "Stop Changing States" : "Change Switch States"} </button>
        {isSub && 
          <SwitchConfigure 
            switchStates={switchStates}
            sendMessage={sendMessage}
            editable={configureSwitches}
          />
        }
      </div>
      
      <div className="board_cluster">
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

      <div className="board_cluster">
        <DialCluster 
          label="Board 2 ADS1015"
          data={b2_data_1015}
          arrangable
          sensor_name="ads_1015"
        />
        <DialCluster 
          label="Board 2 ADS1115"
          data={b2_data_1115}
          arrangable
          sensor_name="ads_1115"
        />
      </div>

      <div className="board_cluster">
        <DialCluster 
          label="Board 3 ADS1015"
          data={b3_data_1015}
          arrangable
          sensor_name="ads_1015"
        />
        <DialCluster 
          label="Board 3 ADS1115"
          data={b3_data_1115}
          arrangable
          sensor_name="ads_1115"
        />
      </div>
      
    </div>
  );
}

export default App;
