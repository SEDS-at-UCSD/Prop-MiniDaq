import { Line } from "react-chartjs-2"
import {LinearScale, Chart, PointElement, LineElement, Tooltip, Title} from 'chart.js'; 
Chart.register(LinearScale, PointElement, LineElement, Tooltip, Title);

export const LineChart = ({data, xlabel,ylabel,title, min, precision, stepSize}) => {
    return (
        <Line
            data={data}
            options={{
            animation: false,
            tooltips: {
                intersect: false
            },
            hover: {
                intersect: false
            },
            scales: {
                x: {
                    type: "linear",
                    precision,
                    ticks: {
                        stepSize
                    },
                    title: {
                        display: true,
                        text: xlabel ? xlabel : "X Axis Label"
                    },
                    min
                },
                y: {
                    type: "linear",
                    title: {
                        display: true,
                        text: ylabel ? ylabel : "Y Axis Label"
                    }
                }
            },
            plugins: {
                title: {
                display: true,
                text: title ? title : "Title"
                },
                legend: {
                display: false
                }
            }
            }}
        />
    );
}
