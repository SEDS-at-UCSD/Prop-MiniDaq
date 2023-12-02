import '../App.css';

export const SwitchConfigure = ({label, switchStates, sendMessage, editable}) => {
    return (
        <div>
            { Object.entries(switchStates).map(([entry,value], num) => {
                return (
                <div>
                <h3 className="solenoid_board">Board {label}</h3>
                <p className="solenoid_control">
                    Sol {entry}: &nbsp;
                    <button 
                        className={value === 1 ? "on_selected sol_button": "sol_button"}
                        type="radio" 
                        onClick={()=>{
                            const message = entry + "1";
                            sendMessage("switch_states_update_" + label ,message);
                        }}
                        disabled={!editable}
                    >
                    On
                    </button>
                    <button 
                        className={value === 0 ? "off_selected sol_button": "sol_button"}
                        type="radio" 
                        onClick={()=>{
                            const message = entry + "0";
                            sendMessage("switch_states_update_" + label,message);
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
