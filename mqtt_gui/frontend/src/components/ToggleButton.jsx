// components/ToggleButton.js
import React, { useState } from "react";

export const ToggleButton = ({ topic, initialState = false, sendMessage }) => {
  const [toggled, setToggled] = useState(initialState);

  const handleToggle = () => {
    const newState = !toggled;
    setToggled(newState);

    // Send an MQTT message with the new state
    const message = newState ? "ON" : "OFF";
    sendMessage(topic, message);
  };

  return (
    <button
      onClick={handleToggle}
      className={`toggle-button ${toggled ? "on" : "off"}`}
    >
      {toggled ? "Turn OFF" : "Turn ON"}
    </button>
  );
};