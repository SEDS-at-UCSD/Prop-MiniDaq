import './App.css';
import { useState, useEffect } from "react";
import * as mqtt from 'mqtt/dist/mqtt.min';
import { DialCluster } from './components/DialCluster';
import { SwitchConfigure } from './components/SwitchConfigure';

function App() {
  //const connectionurl = "ws://localhost:9002";
  const urllocal = '169.254.32.191';
  const connectionurl = `ws://${urllocal}:9002`
  const [client, setClient] = useState(null);
  const [connectStatus, setConnectStatus] = useState("Not Connected");
  const [isSub, setIsSub] = useState(false);
  const [arrangable, setArrangable] = useState(false);
  const [solenoidControl, setSolenoidControl] = useState(false);
  const [ignite, setIgnite] = useState(false);
  const [isAborting, setIsAborting] = useState(false);

  const [topicsList, setTopicsList] = useState([]);
  const [labels, setLabels] = useState({});
  const [boardConfig, setBoardConfig] = useState({});  // Store config here
  const [boardData, setBoardData] = useState({});
  const [solenoidBoardsData, setSolenoidBoardsData] = useState({});

  // Fetch the config file dynamically
  const fetchConfigData = async () => {
    try {
      const response = await fetch(`http://${urllocal}:8000/files/conversion_factor_config.json`);
      const configData = await response.json();
      setBoardConfig(configData);  // Store the full config
      generateTopicsAndLabels(configData);
    } catch (error) {
      console.error('Error fetching config:', error);
    }
  };

  // Generate topics and labels based on the config
  const generateTopicsAndLabels = (configData) => {
    const tempTopicsList = [];
    const tempLabels = {};

    Object.entries(configData).forEach(([boardKey, boardData]) => {
      boardData.data.forEach((sensorGroup) => {
        const topic = sensorGroup.topic;
        const sensors = sensorGroup.sensors;

        // Add topic to the list
        tempTopicsList.push(topic);

        // Store the labels for each sensor topic
        tempLabels[topic] = sensors.map(sensor => sensor.title);
      });
    });

    setTopicsList(tempTopicsList);
    setLabels(tempLabels);
  };

  // Get min/max values dynamically from the config
  const getMinMaxValues = (topic) => {
    const boardNum = topic.match(/\d+/)[0];  // Extract board number
    const boardConfigEntry = boardConfig[`B${boardNum}`];

    if (boardConfigEntry) {
      const sensorGroup = boardConfigEntry.data.find(group => group.topic === topic);
      if (sensorGroup) {
        const minValues = sensorGroup.sensors.map(sensor => sensor.min_value || 0);
        const maxValues = sensorGroup.sensors.map(sensor => sensor.max_value || 1000);
        return { minValues, maxValues };
      }
    }

    return { minValues: [0], maxValues: [1000] };  // Default min/max
  };

  useEffect(() => {
    // Fetch the config data when the component mounts
    fetchConfigData();
  }, []);

  const mqttSub = (topic) => {
    if (client) {
      client.subscribe(topic, (error) => {
        if (error) {
          console.log("Subscribe to " + topic + " error", error);
          return;
        }
      });
    }
  };

  const sendMessage = (topic, message) => {
    client.publish(topic, message, (error) => {
      if (error) {
        console.log("Publish to " + topic + " error", error);
        return;
      }
    });
  };

  const handleIgnitionClick = () => {
    if (!ignite) {
      sendMessage("AUTO", "IGNITE");
      setIgnite(true);
    } else {
      sendMessage("AUTO", "ABORT");
      setIsAborting(true);
    }
  };

  useEffect(() => {
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

    mqttConnect(connectionurl, initialConnectionOptions);
  }, []);

  useEffect(() => {
    if (client) {
      client.on('connect', () => {
        setConnectStatus('Connected');
  
        topicsList.forEach((topic) => {
          mqttSub(topic);
        });
  
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
          parsedMessage = JSON.parse(message);
        } catch (error) {
          parsedMessage = message.toString();
        }

        // Ensure the topic contains a valid board number (e.g., skip "AUTO")
        const boardNumMatch = topic.match(/\d+/);
        if (!boardNumMatch) {
          console.warn(`Invalid topic received: ${topic}, skipping processing.`);
          return; // Skip if no board number is found
        }
        const boardNum = boardNumMatch[0];
        const boardConfigEntry = boardConfig[`B${boardNum}`];
        
        if (boardConfigEntry) {
          const isSolenoid = boardConfigEntry.data.some(group => group.sensor_type === 'solenoids');
          if (isSolenoid) {
            setSolenoidBoardsData((prev) => ({
              ...prev,
              [topic]: parsedMessage.sensor_readings
            }));
          } else {
            setBoardData((prev) => ({
              ...prev,
              [topic]: parsedMessage
            }));
          }
        }
      });
    }
  }, [client, topicsList, boardConfig]);

  return (
    <div className="App">
      <div className="settings">
        <div className="two-column-layout">
          <div className="left-column">
            <p className="Auto:">Automation</p>
            <div className="auto-ignition">
              <button
                onClick={handleIgnitionClick}
                className={`auto-ignition-button ${!ignite ? "ignite" : "abort"}`}
                disabled={isAborting | !solenoidControl}
              >
                {!ignite ? "IGNITE" : "ABORT"}
              </button>
            </div>
          </div>

          <div className="right-column">
            <p className="status">{connectStatus}</p>
            <p className="status">Receiving data: {isSub ? "True" : "False"}</p>

            <div className="min_max_settings">
              <button onClick={() => window.location.href = '/config'} className="status control_button">
                Go to Config
              </button>
              <button onClick={() => window.location.href = '/autoconfig'} className="status control_button">
                View Auto Flow
              </button>
              <button onClick={() => setArrangable(!arrangable)} className="status control_button">
                {arrangable ? "Stop Arranging" : "Arrange Dials"}
              </button>
              <button onClick={() => setSolenoidControl(!solenoidControl)} className="status control_button">
                {solenoidControl ? "Disable Buttons" : "Enable Buttons"}
              </button>
            </div>
          </div>
        </div>

        <div className='solenoid_cluster'>
          {Object.entries(solenoidBoardsData).map(([topic, value]) => (
            <div key={topic}>
              <h3 className="solenoid_board">Board {topic}</h3>
              <SwitchConfigure
                boardlabel={topic}
                switchStates={value}
                sendMessage={sendMessage}
                editable={solenoidControl}
                solLabels={labels[topic]}
              />
            </div>
          ))}
        </div>
      </div>

      <div className="board_cluster">
        {Object.entries(boardData).map(([topic, value]) => {
          const { minValues, maxValues } = getMinMaxValues(topic);  // Get min/max values for all sensors
          return (
            <DialCluster
              key={topic}
              label={"Board " + topic}
              data={value}
              sensor_name={topic}
              arrangable={arrangable}
              psiLabels={labels[topic] || []}
              minValues={minValues}  // Pass all min values for each sensor
              maxValues={maxValues}  // Pass all max values for each sensor
            />
          );
        })}
      </div>
      <img src="../seds_logo.png" alt="SEDS Logo" className="logo" />
    </div>
  );
}

export default App;
