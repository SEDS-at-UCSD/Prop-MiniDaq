import './App.css';
import { useState, useEffect } from "react";
import * as mqtt from 'mqtt/dist/mqtt.min';
import { DialCluster } from './components/DialCluster';
import { SwitchConfigure } from './components/SwitchConfigure';


function App() {
  const connectionurl = "ws://localhost:9001";
  const initialState = {time: 0, sensor_readings: [0, 0, 0, 0]}
  const [client, setClient] = useState(null);
  const [connectStatus, setConnectStatus] = useState("Not Connected");
  const [isSub, setIsSub] = useState(false);
  const [arrangable, setArrangable] = useState(false);

  const [boardData, setBoardData] = useState({
    b1_log_data_1015: initialState,
    b1_log_data_1115: initialState,
    b2_log_data_1015: initialState,
    b2_log_data_1115: initialState,
    b3_log_data_1015: initialState,
    b3_log_data_1115: initialState,
    b4_log_data_1015: initialState,
    b4_log_data_1115: initialState,
    b5_log_data_1015: initialState,
    b5_log_data_1115: initialState,
  })

  const [switchStates, setSwitchStates] = useState({
    0: 0,
    1: 0,
    2: 0,
    3: 0,
    4: 0
  });

  const [solenoidBoardsData, setSolenoidBoardsData] = useState({
    4: {
      0: 0,
      1: 0,
      2: 0,
      3: 0,
      4: 0
    },
    5: {
      0: 0,
      1: 0,
      2: 0,
      3: 0,
      4: 0
    }
  });
  
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

    mqttConnect(connectionurl,initialConnectionOptions);
  }, []);

  useEffect(() => {
    if (client) {
      client.on('connect', () => {
        setConnectStatus('Connected');
        for (const key in boardData) {
          mqttSub(key);
          console.log("Subscribed to " + key);
        }
        mqttSub("switch_states_status_4");
        mqttSub("switch_states_status_5");
      });

      client.on('error', (err) => {
        console.error('Connection error: ', err);
        client.end();
      });

      client.on('reconnect', () => {
        setConnectStatus('Lost connection, attempting to reconnect');
      });

      client.on('message', (topic, message) => {
        message = JSON.parse(String(message));
        if (topic === "switch_states_status_4") {
          setSolenoidBoardsData((prev)=>{
            const newSolenoidData = { ...prev };
            for (let i = 0; i < 5; i++){
              newSolenoidData[4][i] = message.sensor_readings[i]; 
            }
            return newSolenoidData
          });
        }
        else if (topic === "switch_states_status_5") {
          setSolenoidBoardsData((prev)=>{
            const newSolenoidData = { ...prev };
            for (let i = 0; i < 5; i++){
              newSolenoidData[5][i] = message.sensor_readings[i]; 
            }
            return newSolenoidData
          });
        }
        else {
          setBoardData((prev)=>{
            return { ...prev, [topic]: message };
          })
        }
        console.log(solenoidBoardsData[4]);
        console.log(solenoidBoardsData[5]);
      });
    }
  }, [client]);

  return (
    <div className="App">
      <div className="settings">
        <p className="status">{connectStatus}</p>
        <p className="status">Receiving data: {isSub ? "True" : "False"}</p>
        <div className="min_max_settings">
          <button onClick={()=>setArrangable(!arrangable)} className="status"> {arrangable ? "Stop Arranging" : "Arrange Dials"} </button>
        </div> 
        <div className='solenoid_cluster'>
        {Object.entries(solenoidBoardsData).map(([key,value])=>{
          return (
            <SwitchConfigure 
              label={key}
              switchStates={solenoidBoardsData[key]}
              sendMessage={sendMessage}
              editable={isSub}
            />
          )
        })}
        </div>
      </div>

      <div className="board_cluster">
        {Object.entries(boardData).map(([key,value])=>{
          return (
            <DialCluster 
              label={"Board " + key[1] + " ADS " + key.substring(12)}
              data={value}
              arrangable
              sensor_name={key.substring(3)}
            />
          )
        })}
      </div>
    </div>
  );
}

export default App;
