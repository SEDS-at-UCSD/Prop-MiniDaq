import React, { useState, useEffect } from 'react';
import yaml from 'js-yaml'; // Import to convert JS object to YAML format

let SERVER_ADDRESS = window.location.hostname;
const SERVER_PORT = 8000;

function ChannelConfig() {
  const [sections, setSections] = useState({});
  const [isSaving, setIsSaving] = useState(false);
  const [saveStatus, setSaveStatus] = useState(''); // Track 'success', 'failure', or ''
  const [savedConfig, setSavedConfig] = useState(null); // Stores loaded configuration

  const NUM_CALCS = 4; // Define the number of calculation fields per mapping

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
        console.log('Parsed configuration:', parsedConfig);

        // Ensure each board has exactly NUM_CALCS fields
        const formattedConfig = Object.keys(parsedConfig || {}).reduce((acc, key) => {
          const calculations = parsedConfig[key]?.calculations || [];
          acc[key] = [...calculations, ...Array(NUM_CALCS - calculations.length).fill('')].slice(0, NUM_CALCS);
          return acc;
        }, {});

        setSections(formattedConfig);
        setSavedConfig(parsedConfig);
      })
      .catch((error) => {
        console.error('Error loading configuration:', error);
      });
  };

  useEffect(() => {
    fetchConfig();
  }, []);

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

  const saveConfig = () => {
    setIsSaving(true);

    const configData = Object.keys(sections).reduce((acc, key) => {
      acc[key] = {
        calculations: sections[key].filter((calc) => calc.trim() !== ''), // Only save non-empty calculations
      };
      return acc;
    }, {});

    const yamlConfig = yaml.dump(configData);

    const blob = new Blob([yamlConfig], { type: 'application/x-yaml' });
    const formData = new FormData();
    formData.append('file', blob, 'calculation_config.yaml');

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
        fetchConfig();
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
        {Object.keys(sections).length > 0 ? (
          Object.keys(sections).map((section) => (
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
          ))
        ) : (
          <p>Loading configuration...</p>
        )}

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

        {saveStatus && (
          <div
            style={{
              marginTop: '20px',
              color: saveStatus === 'success' ? 'green' : 'red',
            }}
          >
            {saveStatus === 'success'
              ? 'Configuration saved successfully!'
              : 'Error saving configuration.'}
          </div>
        )}
      </div>

      <div style={{ flex: 1 }}>
        <h2>Saved Configuration</h2>
        {savedConfig ? (
          <pre
            style={{
              backgroundColor: '#f4f4f4',
              padding: '15px',
              borderRadius: '5px',
              overflowX: 'auto',
              color: 'black',
            }}
          >
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