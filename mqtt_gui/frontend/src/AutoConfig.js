import React, { useEffect, useState } from 'react';
import YAML from 'js-yaml';
import { ReactFlow, ReactFlowProvider, MiniMap, Controls, Background } from 'reactflow';
import 'reactflow/dist/style.css';

let SERVER_ADDRESS = window.location.hostname;
const SERVER_PORT = 8000;

function AutoConfig() {
  const [automationSequence, setAutomationSequence] = useState([]);
  const [elements, setElements] = useState([]);
  const [error, setError] = useState(null);
  const [isLoading, setIsLoading] = useState(true);

  // Fetch YAML data
  useEffect(() => {
    fetch(`http://${SERVER_ADDRESS}:${SERVER_PORT}/files/automation_config.yaml`)
      .then((response) => response.text())
      .then((yamlText) => {
        try {
          const data = YAML.load(yamlText);
          console.log("Parsed YAML data: ", data);
          setAutomationSequence(data.automation_sequence);
          setIsLoading(false);
        } catch (e) {
          console.error("Error parsing YAML: ", e);
          setError("Error parsing YAML");
          setIsLoading(false);
        }
      })
      .catch((error) => {
        console.error('Error fetching automation sequence:', error);
        setError(error.message);
        setIsLoading(false);
      });
  }, []);

  // Generate elements after fetching YAML
  useEffect(() => {
    if (automationSequence.length > 0) {
      const newElements = createFlowElements(automationSequence);
      console.log("Generated Flow Elements: ", newElements);  // Log generated elements
      setElements(newElements);
    }
  }, [automationSequence]);

  // Create nodes and edges, with larger lines and arrows
  const createFlowElements = (sequence) => {
    const nodeSpacingX = 100;
    const nodeSpacingY = 100;

    const nodes = sequence.map((step, index) => ({
      id: `${step.id}`,
      data: { label: `${step.name} (Cmd: ${step.command})` },
      position: { x: (2 * (index % 2)) * nodeSpacingX, y: index * nodeSpacingY },
      style: {
        background: '#f39c12',  // Add a background color
        color: '#fff',          // Set text color
        padding: 10,            // Add padding
        border: '1px solid #333', // Add a border
        borderRadius: 5,        // Rounded corners
        textAlign: 'center',    // Align text to center
      },
    }));

    const edgeColors = ['#ff5733', '#33c1ff'];
    const edges = sequence.flatMap((step) =>
      step.next_id.map((next, index) => ({
        id: `e${step.id}-${next}`,
        source: `${step.id}`,
        target: `${next}`,
        type: 'straight',
        style: { 
          stroke: edgeColors[step.id % 2],  // Alternate edge color
          strokeWidth: 3,  // Increase line thickness
        },
        markerEnd: {
          type: 'arrowclosed',  // Arrow type
          markerWidth: 15,      // Increase width of arrow
          markerHeight: 15,     // Increase height of arrow
          color: edgeColors[step.id % 2],  // Arrow color matches edge
        },
      }))
    );

    return [...nodes, ...edges];
  };

  if (isLoading) {
    return <div>Loading automation sequence...</div>;
  }

  if (error) {
    return <div>Error: {error}</div>;
  }

  return (
    <ReactFlowProvider>
      <div style={{ padding: '15px', height: '80vh', width: '100%' }}>
        <h1>Automation Flow</h1>
        <button onClick={() => window.location.href = '/'} className="status control_button"
            style={{
                borderRadius: '5px',
            }}    
        >
            Back to Dashboard
        </button>

        {elements && elements.length > 0 ? (
          <ReactFlow
            nodes={elements.filter(e => !e.source)}  // Filter out nodes from elements
            edges={elements.filter(e => e.source)}   // Filter out edges from elements
            style={{ width: '100%', height: '100%' }}
            fitView
            nodesDraggable={true}  // Ensure nodes can be dragged
            elementsSelectable={true}  // Ensure elements can be selected
          >
            <MiniMap />
            <Controls />
            <Background />
          </ReactFlow>
        ) : (
          <div>No elements to display</div>
        )}
      </div>
    </ReactFlowProvider>
  );
}

export default AutoConfig;
