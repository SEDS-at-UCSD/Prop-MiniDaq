import '../App.css';

export const SwitchConfigure = ({switchStates, sendMessage, editable}) => {
    return (
        <div>
            { Object.entries(switchStates).map(([entry,value], num) => {
                return (
                <p className="solenoid_control">
                    Sol {entry}: &nbsp;
                    <button 
                        className={value === 1 ? "on_selected sol_button": "sol_button"}
                        type="radio" 
                        onClick={()=>{
                            const message = entry + "1";
                            sendMessage("switch_states_update",message);
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
                            sendMessage("switch_states_update",message);
                        }}
                        disabled={!editable}
                    >
                    Off
                    </button>
                </p>
                )
            })
            }
        </div>
    )
}
