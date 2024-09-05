import './App.css';
import { useState, useEffect } from "react";
import * as mqtt from 'mqtt/dist/mqtt.min';
import { DialCluster } from './components/DialCluster';
import { SwitchConfigure } from './components/SwitchConfigure';

function App() {
  //const connectionurl = "ws://localhost:9002";
  const connectionurl = "ws://169.254.32.191:9002"
  const [client, setClient] = useState(null);
  const [connectStatus, setConnectStatus] = useState("Not Connected");
  const [isSub, setIsSub] = useState(false);
  const [arrangable, setArrangable] = useState(false);
  const [solenoidControl, setSolenoidControl] = useState(false);
  const [ignite, setIgnite] = useState(false);
  const [isAborting, setIsAborting] = useState(false);

  const solenoidLabels = {
    // "4": ['LOX DOME IN', 'LNG VENT NO', 'LNG MAIN NC', 'LOX MAIN NC', 'LOX VENT NO'],
    // "5": ['LNG DOME IN ', 'LNG DOME OUT', '12V: NIL', '12V: NIL', 'LOX DOME OUT']
    "4": ['E-match','OX MAIN','FUEL MAIN','OX PURGE','FUEL PURGE'],
    "5": ['OX VENT','FUEL VENT','OX DOME IN','FUEL DOME IN','4'],
    "6": ['0','1','2','3','4']
  };

  const PTLabels = {
    "log_data_1015": ['FUEL TANK 1K (PSI)', 'OX TANK 1K (PSI)', 'FUEL DOME 1K (PSI)', 'OX DOME 1K (PSI)'],
    "log_data_1115": ['CHAMBER 1K (PSI)', 'FUEL MANIFOLD 1K (PSI)', 'OX MANIFOLD 3K (PSI)', 'OX FLOWRATE 32.5 (GPM)']
  };

  const topics_list = [
    "b1_log_data_1015",
    "b1_log_data_1115",
    "b1_log_data_TC",
    "b2_log_data_1015",
    "b2_log_data_1115",
    "b2_log_data_TC",
    "b3_log_data_1015",
    "b3_log_data_1115",
    "b3_log_data_TC"
  ]

  const [boardData, setBoardData] = useState({})

  const [solenoidBoardsData, setSolenoidBoardsData] = useState({});
  
  const mqttSub = (topic) => {
    if (client) {
      client.subscribe(topic, (error) => {
        if (error) {
          console.log("Subscribe to " + topic + " error", error)
          return
        }
      });
    }
  };

  const sendMessage = (topic, message) => {
    client.publish(topic,message,(error)=>{
      if (error) {
        console.log("Publish to " + topic + " error", error)
        return
      }
    })
  }

  const handleIgnitionClick = () => {
    if (!ignite){
      // IN IGNITABLE STATE
      sendMessage("AUTO", "IGNITE");
      setIgnite(true); // Change to abortable state
    }
    else {
      // IN ABORTABLE STATE
      sendMessage("AUTO", "ABORT");
      setIsAborting(true);
    }
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
        topics_list.forEach((topic)=>{
          mqttSub(topic);
        })
        mqttSub("switch_states_status_4");
        mqttSub("switch_states_status_5");
        mqttSub("switch_states_status_6");
        mqttSub("AUTO");
        setIsSub(true);
      });

      client.on('error', (err) => {
        console.error('Connection error: ', err);
        client.end();
      });

      client.on('reconnect', () => {
        setConnectStatus('Lost connection, attempting to reconnect');
      });

      client.on('message', (topic, message) => {
        let parsedMessage;
        try {
          // Try to parse the message as JSON
          parsedMessage = JSON.parse(message);
        } catch (error) {
          // If parsing fails, treat it as a plain string
          parsedMessage = message.toString();
        }
        if (!isSub && topic === "topics_list") {
          parsedMessage.topics.forEach((subscription)=>{
            mqttSub(subscription);
          });
          setIsSub(true);
        }
        // CAN CHANGE TO IGNITE AGAIN
        // Expects JSON of format {state: "ABORT SUCCESSFUL"} or {state: "IGNITION SUCCESSFUL"} from back-end
        else if (topic === "AUTO" && parsedMessage === "ABORT SUCCESSFUL") {
          setIsAborting(false);
          setIgnite(false);
        }
        else if (topic === "AUTO" && parsedMessage === "IGNITION SUCCESSFUL") {
          setIgnite(true);
        }
        else if (topic === "switch_states_status_4") {
          setSolenoidBoardsData((prev)=>{
            const newSolenoidData = { ...prev };
            newSolenoidData[4] = parsedMessage.sensor_readings;
            return newSolenoidData
          });
        }
        else if (topic === "switch_states_status_5") {
          setSolenoidBoardsData((prev)=>{
            const newSolenoidData = { ...prev };
            newSolenoidData[5] = parsedMessage.sensor_readings; 
            return newSolenoidData
          });
        }
        else if (topic === "switch_states_status_6") {
          setSolenoidBoardsData((prev)=>{
            const newSolenoidData = { ...prev };
            newSolenoidData[6] = parsedMessage.sensor_readings; 
            return newSolenoidData
          });
        }
        else {
          setBoardData((prev)=>{
            return { ...prev, [topic]: parsedMessage };
          })
        }
      });
    }
  }, [client]);

  return (
    <div className="App">
      <div className="settings">
        <p className="status">{connectStatus}</p>
        <p className="status">Receiving data: {isSub ? "True" : "False"}</p>
        <div className="min_max_settings">
          <button onClick={()=>setArrangable(!arrangable)} className="status control_button"> {arrangable ? "Stop Arranging" : "Arrange Dials"} </button>
          <button onClick={()=>setSolenoidControl(!solenoidControl)} className="status control_button"> {solenoidControl ? "Disable Buttons" : "Enable Buttons"} </button>
        </div> 
        <div className="auto-ignition">
          <button onClick={handleIgnitionClick}
                  className={`auto-ignition-button ${!ignite ? "ignite" : "abort"}`}
                  disabled={isAborting}> {!ignite ? "IGNITE" : "ABORT"} </button>
        </div>
        <div className='solenoid_cluster'>
        {Object.entries(solenoidBoardsData).map(([key,value])=>{
          return (
            <div>
              <h3 className="solenoid_board">Board {key}</h3>
              <SwitchConfigure 
                boardlabel={key}
                switchStates={value}
                sendMessage={sendMessage}
                editable={solenoidControl}
                solLabels={solenoidLabels} 
              />
            </div>
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
              arrangable={arrangable}
              sensor_name={key.substring(3)}
              psiLabels={PTLabels} 
            />
          )
        })}
      </div>
      <img src="../seds_logo.png" alt="SEDS Logo" className="logo"/>
    </div>
  );
}

export default App;
