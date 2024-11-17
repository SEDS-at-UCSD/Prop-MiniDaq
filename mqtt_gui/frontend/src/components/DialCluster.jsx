import '../App.css';
import { Knob } from 'primereact/knob';
import Draggable from 'react-draggable';

export const DialCluster = ({ label, data, sensor_name, arrangable, psiLabels, minValues, maxValues }) => {
  return (
    <Draggable disabled={!arrangable}>
      <div className="dial_cluster">
        <h3 className="readings_label">{label}: Last updated at {data.time}</h3>
        <div className="sensor_readings">
          {data.sensor_readings && data.sensor_readings.map((reading, i) => {
            return (
              <div className="knob_container" key={i}>
                <Knob 
                  value={reading}
                  valueColor={reading < 0 ? "red" : "black"}
                  min={minValues[i]}  // Set individual min value for each sensor
                  max={maxValues[i]}  // Set individual max value for each sensor
                  textColor="white"
                />
                <p>{label.includes("TC") ? "°C" : psiLabels[i]}</p>
              </div>
            );
          })}
        </div>
      </div>
    </Draggable>
  );
};
