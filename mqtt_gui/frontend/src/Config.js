import React, { useEffect, useState } from 'react';

// Configurable server address (change 'localhost' if needed)
let SERVER_ADDRESS = 'localhost';  // Change this to the desired server address or IP
const SERVER_PORT = 8000;  // Change port if needed

function Config() {
  const [conversionFactors, setConversionFactors] = useState(null);  // State to hold config
  const [isLoading, setIsLoading] = useState(true);  // State to indicate loading
  const [isSaving, setIsSaving] = useState(false);  // State for saving
  const [saveStatus, setSaveStatus] = useState('');  // Track 'success', 'failure', or ''
  const [error, setError] = useState(null);  // State to handle errors

  // Fetch the JSON configuration from FastAPI server
  const loadConfig = () => {
    fetch(`http://${SERVER_ADDRESS}:${SERVER_PORT}/files/conversion_factor_config.json`)
      .then((response) => {
        if (!response.ok) {
          throw new Error(`Error fetching config: ${response.statusText}`);
        }
        return response.json();  // Parse the response as JSON
      })
      .then((data) => {
        setConversionFactors(data);  // Set the loaded data in state
        setIsLoading(false);  // Loading complete
      })
      .catch((error) => {
        console.error('Error loading configuration:', error);
        setError(error.message);  // Set error message
        setIsLoading(false);  // Stop loading
      });
  };

  // Handle input change in the textboxes
  const handleValueChange = (boardKey, sensorType, sensorIndex, field, newValue) => {
    setConversionFactors((prevState) => {
      const updatedFactors = { ...prevState };
      updatedFactors[boardKey].data.forEach((dataBlock) => {
        if (dataBlock.sensor_type === sensorType) {
          dataBlock.sensors[sensorIndex][field] = newValue;  // Update the specific field value
        }
      });
      return updatedFactors;  // Return the updated state
    });
  };

  // Save the modified configuration back to FastAPI
  const saveConfig = () => {
    setIsSaving(true);  // Show saving state
  
    const formattedConfig = JSON.stringify(conversionFactors, null, 2);

    const blob = new Blob([formattedConfig], { type: 'application/json' });
    const formData = new FormData();
    formData.append('file', blob, 'conversion_factor_config.json');  // Append the file blob
  
    fetch(`http://${SERVER_ADDRESS}:${SERVER_PORT}/upload/conversion_factor_config.json`, {
      method: 'POST',
      body: formData,
    })
      .then((response) => {
        if (!response.ok) {
          throw new Error('Failed to save configuration');
        }
        console.log(response)
        return response.json();
      })
      .then(() => {
        setSaveStatus('success');  // Mark as success
        setIsSaving(false);  // Stop saving state
        setTimeout(() => setSaveStatus(''), 3000);  // Reset after delay
      })
      .catch(() => {
        setSaveStatus('failure');  // Mark as failure
        setIsSaving(false);
        setTimeout(() => setSaveStatus(''), 3000);  // Reset after delay
      });
  };

  // Load the configuration when the component mounts
  useEffect(() => {
    loadConfig();  // Load configuration when the page loads
  }, []);

  if (isLoading) {
    return <div>Loading configuration...</div>;
  }

  if (error) {
    return <div>Error: {error}</div>;
  }

  if (!conversionFactors) {
    return <div>No configuration loaded.</div>;
  }

  // Determine the button color based on the save status
  const getButtonColor = () => {
    if (saveStatus === 'success') return '#4CAF50';  // Green for success
    if (saveStatus === 'failure') return '#f44336';  // Red for failure
    if (isSaving) return '#ccc';  // Gray for in-progress save
    return '#007BFF';  // Neutral blue as the default color
  };

  return (
    <div style={{ padding: '30px' }}>  {/* Add padding to the config section */}
      {/* Header and Save button */}
      <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: '30px' }}>
        <h1 style={{ margin: 0 }}>Configuration</h1>

        <button onClick={() => window.location.href = '/'} className="status control_button"
            style={{
                borderRadius: '5px',
            }}    
        >
        Back to Dashboard
        </button>
        {/* Save button */}
        <button
          onClick={saveConfig}
          disabled={isSaving}
          style={{
            padding: '10px 20px',  // Adjust padding for the button
            fontSize: '16px',
            backgroundColor: getButtonColor(),  // Dynamic color
            color: '#fff',
            border: 'none',
            borderRadius: '5px',
            cursor: isSaving ? 'not-allowed' : 'pointer'
          }}
        >
          {isSaving ? 'Saving...' : saveStatus === 'success' ? 'Saved!' : saveStatus === 'failure' ? 'Failed!' : 'Save Configuration'}
        </button>
      </div>

      <form>
        {/* Display each board */}
        {Object.entries(conversionFactors).map(([boardKey, boardData]) => (
          <div
            key={boardKey}
            style={{
              border: '1px solid #fff',
              padding: '20px',
              borderRadius: '8px',
              marginBottom: '20px',
              boxShadow: '0 4px 8px rgba(0, 0, 0, 0.1)',
            }}
          >
            <h2>{boardKey}</h2>

            {/* Display sensor types and sensors under each board */}
            {boardData.data.map((dataBlock, dataIndex) => (
              <div key={dataIndex}>
                <h3>{dataBlock.sensor_type}</h3>
                <div style={{ display: 'grid', gridTemplateColumns: 'repeat(2, 1fr)', gap: '20px' }}>
                  {/* Display each sensor */}
                  {dataBlock.sensors.map((sensor, sensorIndex) => (
                    <div key={sensorIndex}>
                      <h4>{sensor.title}</h4>
                      {Object.entries(sensor).map(([field, value]) => {
                        const inputType = typeof value === 'boolean'
                          ? 'checkbox'
                          : typeof value === 'number'
                          ? 'number'
                          : 'text';
                        return (
                          <label key={field} style={{ display: 'block', marginBottom: '10px' }}>
                            {field}:
                            <input
                              type={inputType}
                              value={inputType === 'checkbox' ? undefined : value}
                              checked={inputType === 'checkbox' ? value : undefined}
                              onChange={(e) =>
                                handleValueChange(
                                  boardKey,
                                  dataBlock.sensor_type,
                                  sensorIndex,
                                  field,
                                  inputType === 'checkbox' ? e.target.checked : e.target.value
                                )
                              }
                              style={{ marginLeft: '10px', width: '100px' }}
                            />
                          </label>
                        );
                      })}
                    </div>
                  ))}
                </div>
              </div>
            ))}
          </div>
        ))}
      </form>
    </div>
  );
}

export default Config;
