import React, { useState, useEffect } from 'react';
import yaml from 'js-yaml'; // Import to convert JS object to YAML format

// Configurable server address (change 'localhost' if needed)
let SERVER_ADDRESS = window.location.hostname;
const SERVER_PORT = 8000;

function ChannelConfig() {
  const initialCalculations = ['', '', '', ''];
  const [sections, setSections] = useState({
    B1: initialCalculations,
    B2: initialCalculations,
    B3: initialCalculations,
    B4: initialCalculations,
  });
  const [isSaving, setIsSaving] = useState(false);
  const [saveStatus, setSaveStatus] = useState(''); // Track 'success', 'failure', or ''
  const [savedConfig, setSavedConfig] = useState(null); // Stores loaded configuration

  // Function to fetch saved configuration from the server
  const fetchConfig = () => {
    fetch(`http://${SERVER_ADDRESS}:${SERVER_PORT}/files/calculation_config.yaml`)
      .then((response) => {
        if (!response.ok) {
          throw new Error('Failed to fetch configuration');
        }
        return response.text();
      })
      .then((yamlText) => {
        const parsedConfig = yaml.load(yamlText);
        const formattedConfig = Object.keys(sections).reduce((acc, key) => {
          acc[key] = initialCalculations.map((_, idx) => parsedConfig[key]?.calculations[idx] || '');
          return acc;
        }, {});
        setSections(formattedConfig);
        setSavedConfig(parsedConfig);
      })
      .catch((error) => {
        console.error('Error loading configuration:', error);
      });
  };

  // Initial fetch of configuration on component mount
  useEffect(() => {
    fetchConfig();
  }, []);

  // Handle change in input fields for each section
  const handleCalcChange = (section, index, newValue) => {
    setSections((prevSections) => {
      const updatedSection = [...prevSections[section]];
      updatedSection[index] = newValue;
      return {
        ...prevSections,
        [section]: updatedSection,
      };
    });
  };

  // Handle save to YAML and upload to the server
  const saveConfig = () => {
    setIsSaving(true);

    // Create a data structure with the sections and their calculations
    const configData = Object.keys(sections).reduce((acc, key) => {
      acc[key] = {
        calculations: sections[key].filter((calc) => calc.trim() !== ''), // Only save non-empty calculations
      };
      return acc;
    }, {});

    // Convert the data structure to YAML format
    const yamlConfig = yaml.dump(configData);

    // Create a Blob from the YAML string
    const blob = new Blob([yamlConfig], { type: 'application/x-yaml' });
    const formData = new FormData();
    formData.append('file', blob, 'calculation_config.yaml');

    // Send the YAML file to the server
    fetch(`http://${SERVER_ADDRESS}:${SERVER_PORT}/upload/calculation_config.yaml`, {
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
        setSaveStatus('success');
        setIsSaving(false);
        setTimeout(() => setSaveStatus(''), 3000);
        fetchConfig(); // Re-fetch the configuration after saving
      })
      .catch(() => {
        setSaveStatus('failure');
        setIsSaving(false);
        setTimeout(() => setSaveStatus(''), 3000);
      });
  };

  return (
    <div style={{ display: 'flex', padding: '30px' }}>
      <div style={{ flex: 1, marginRight: '20px' }}>
        <h1>Configuration for Calculations</h1>
        {['B1', 'B2', 'B3', 'B4'].map((section) => (
          <div key={section} style={{ marginBottom: '20px' }}>
            <h2>{section}</h2>
            {sections[section].map((calc, index) => (
              <div key={index} style={{ marginBottom: '10px' }}>
                <label style={{ marginRight: '10px' }}>Calc {index + 1}:</label>
                <input
                  type="text"
                  value={calc}
                  onChange={(e) => handleCalcChange(section, index, e.target.value)}
                  style={{ padding: '5px', width: '300px' }}
                />
              </div>
            ))}
          </div>
        ))}

        <button
          onClick={saveConfig}
          disabled={isSaving}
          style={{
            padding: '10px 20px',
            fontSize: '16px',
            backgroundColor: isSaving ? '#ccc' : '#007BFF',
            color: '#fff',
            border: 'none',
            borderRadius: '5px',
            cursor: isSaving ? 'not-allowed' : 'pointer',
          }}
        >
          {isSaving ? 'Saving...' : saveStatus === 'success' ? 'Saved!' : saveStatus === 'failure' ? 'Failed!' : 'Save Config'}
        </button>

        {saveStatus && <div style={{ marginTop: '20px', color: saveStatus === 'success' ? 'green' : 'red' }}>{saveStatus === 'success' ? 'Configuration saved successfully!' : 'Error saving configuration.'}</div>}
      </div>

      <div style={{ flex: 1 }}>
        <h2>Saved Configuration</h2>
        {savedConfig ? (
          <pre style={{ backgroundColor: '#f4f4f4', padding: '15px', borderRadius: '5px', overflowX: 'auto', color: 'black' }}>
            {yaml.dump(savedConfig)}
          </pre>
        ) : (
          <p>Loading configuration...</p>
        )}
      </div>
    </div>
  );
}

export default ChannelConfig;
