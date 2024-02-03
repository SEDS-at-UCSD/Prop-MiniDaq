import '../App.css';
import { Knob } from 'primereact/knob';
import Draggable from 'react-draggable';
import settings from "../settings.json";

export const DialCluster = ({label, data, sensor_name, arrangable, psiLabels}) => {
    return (
        <Draggable disabled={!arrangable}>
            <div className="dial_cluster">
                <h3 className="readings_label">{label}: Last updated at {data.time}</h3>
                <div className="sensor_readings">
                    { data.sensor_readings && data.sensor_readings.map((reading,i)=>{
                    return (
                        <div className="knob_container">
                            <Knob 
                                value={reading}
                                valueColor={reading < 0 ? "red" : "black"}
                                min = {settings[sensor_name + "_min"]}
                                max = {settings[sensor_name + "_max"]}
                                textColor="white"
                            />
                            <p>{label.includes("TC") ? "°C" : (label.includes("ADS") ? psiLabels[sensor_name][i] : "PSI")}</p>
                        </div>
                    );
                    })}
                </div>
            </div>
        </Draggable>
    );
}
