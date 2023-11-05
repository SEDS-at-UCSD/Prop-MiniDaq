

export const SwitchConfigure = ({switchStates, sendMessage}) => {
    return (
        <div>
            { Object.entries(switchStates).map(([entry,value], num) => {
                return (
                <p>
                    {entry}: &nbsp;
                    <input 
                        className="number_input" 
                        type="radio" 
                        onChange={()=>{
                            const newStates = switchStates;
                            newStates[entry] = 1;
                            sendMessage("switch_states",newStates);
                        }}
                        checked={value === 1}
                    ></input>
                    On
                    <input 
                        className="number_input" 
                        type="radio" 
                        onChange={()=>{
                            sendMessage((prevStates)=>{
                                prevStates[entry] = 0;
                                return {
                                    ...prevStates
                                }
                            });
                        }}
                        checked={value === 0}
                    ></input>
                    Off
                </p>
                )
            })
            }
        </div>
    )
}
