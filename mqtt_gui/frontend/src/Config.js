import React, { useEffect, useState } from 'react';

// Configurable server address (change 'localhost' if needed)
let SERVER_ADDRESS = 'localhost';  // Change this to the desired server address or IP
const SERVER_PORT = 8000;  // Change port if needed

function Config() {
  const [conversionFactors, setConversionFactors] = useState(null);  // State to hold config
  const [isLoading, setIsLoading] = useState(true);  // State to indicate loading
  const [isSaving, setIsSaving] = useState(false);  // State for saving
  const [saveStatus, setSaveStatus] = useState('');  // Track 'success', 'failure', or ''
  const [saveMessage, setSaveMessage] = useState('');  // State for save feedback
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
  const handleValueChange = (prefix, key, index, newValue) => {
    setConversionFactors((prevState) => {
      const updatedFactors = { ...prevState };
      updatedFactors[`${prefix}_${key}`][index] = newValue;  // Update the specific value
      return updatedFactors;  // Return the updated state
    });
  };

  // Save the modified configuration back to FastAPI
  const saveConfig = () => {
    setIsSaving(true);  // Show saving state
  
    // Pretty print with 2 spaces
    let formattedConfig = JSON.stringify(conversionFactors, null, 2);
  
    // Clean up the formatting
    formattedConfig = formattedConfig
      .replace(/,\n(?!\s*\])/g, ',')  // Remove newline after commas, unless followed by a closing bracket
      .replace(/\[\n/g, '[')          // Remove newline after opening square brackets
      .replace(/\s*\],/g, '],\n')     // Ensure newline after closing square brackets with a comma
      .replace(/\s*\",/g, '",\n');    // Ensure newline after double-quoted strings followed by a comma
    
    console.log(formattedConfig)

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
        return response.json();
      })
      .then(() => {
        setSaveStatus('success');  // Mark as success
        setIsSaving(false);  // Stop saving state

        // Set timeout to fade back to blue after 3 seconds
        setTimeout(() => {
          let fadeInterval = setInterval(() => {
            setSaveStatus('');  // Reset the button color to default (blue)
            clearInterval(fadeInterval);  // Clear the interval once done
          }, 3000);
        }, 3000);
      })
      .catch(() => {
        setSaveStatus('failure');  // Mark as failure
        setIsSaving(false);

        // Set timeout to fade back to blue after 3 seconds
        setTimeout(() => {
          let fadeInterval = setInterval(() => {
            setSaveStatus('');  // Reset the button color to default (blue)
            clearInterval(fadeInterval);  // Clear the interval once done
          }, 3000);
        }, 3000);
      });
  };
  

  // Group the entries by their "block" (e.g., B1, B2)
  const groupByPrefix = (config) => {
    const grouped = {};
    Object.keys(config).forEach((key) => {
      const prefix = key.split('_')[0];  // Get the prefix before the first underscore (e.g., B1, B2)
      const suffix = key.split('_').slice(1).join('_');  // Get the rest of the key after the prefix
      
      if (!grouped[prefix]) {
        grouped[prefix] = {};
      }
      grouped[prefix][suffix] = config[key];  // Group the suffix under the prefix
    });
    return grouped;
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

  const groupedConfig = groupByPrefix(conversionFactors);

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

        {/* Save button with dynamic color */}
        <button onClick={() => window.location.href = '/'} className="status control_button"
            style={{
                borderRadius: '5px',
            }}    
        >
            Back to Dashboard
        </button>

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
        {/* Display grouped blocks */}
        {Object.entries(groupedConfig).map(([prefix, entries]) => (
          <div
            key={prefix}
            style={{
              border: '1px solid #fff',
              padding: '20px',
              borderRadius: '8px',
              marginBottom: '20px',
              boxShadow: '0 4px 8px rgba(0, 0, 0, 0.1)',
            }}
          >
            <h2>{prefix}</h2>
            <div style={{ display: 'grid', gridTemplateColumns: 'repeat(3, 1fr)', gap: '20px' }}>
              {Object.entries(entries).map(([key, values]) => (
                <div key={key}>
                  <h4>{key}</h4>
                  {Array.isArray(values) ? (
                    values.map((value, index) => (
                      <div key={index} style={{ marginBottom: '10px' }}>
                        <label>
                          {key}[{index}]:
                          <input
                            type="number"
                            value={value}
                            onChange={(e) => handleValueChange(prefix, key, index, parseFloat(e.target.value))}
                            style={{ marginLeft: '10px', width: '100px' }}
                          />
                        </label>
                      </div>
                    ))
                  ) : (
                    <p>{values}</p>
                  )}
                </div>
              ))}
            </div>
          </div>
        ))}
      </form>
    </div>
  );
}

export default Config;
