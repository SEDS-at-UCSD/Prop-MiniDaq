

export const SwitchConfigure = ({switchStates, sendMessage, editable}) => {
    return (
        <div>
            { Object.entries(switchStates).map(([entry,value], num) => {
                return (
                <p>
                    Switch {entry}: &nbsp;
                    <input 
                        className="number_input" 
                        type="radio" 
                        onChange={()=>{
                            const message = entry + "1";
                            sendMessage("switch_states_update",message);
                        }}
                        checked={value === 1}
                        disabled={!editable}
                    ></input>
                    On
                    <input 
                        className="number_input" 
                        type="radio" 
                        onChange={()=>{
                            const message = entry + "0";
                            sendMessage("switch_states_update",message);
                        }}
                        checked={value === 0}
                        disabled={!editable}
                    ></input>
                    Off
                </p>
                )
            })
            }
        </div>
    )
}
