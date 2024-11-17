import '../App.css';

export const SwitchConfigure = ({boardlabel, switchStates, sendMessage, editable, solLabels}) => {
    return (
        <div>
            { switchStates.map((value, i) => {
                return (
                <div>
                <p className="solenoid_control">
                    {solLabels[i] || `Sol ${i}`}: &nbsp;
                    <button 
                        className={value !== 0 ? "on_selected sol_button": "sol_button"}
                        type="radio" 
                        onClick={()=>{
                            const message = i  + "1";
                            sendMessage("update_" + boardlabel,message);
                        }}
                        disabled={!editable}
                    >
                    On
                    </button>
                    <button 
                        className={value === 0 ? "off_selected sol_button": "sol_button"}
                        type="radio" 
                        onClick={()=>{
                            const message = i + "0";
                            sendMessage("update_" + boardlabel,message);
                        }}
                        disabled={!editable}
                    >
                    Off
                    </button>
                </p>
                </div>
                )
            })
            }
        </div>
    )
}
