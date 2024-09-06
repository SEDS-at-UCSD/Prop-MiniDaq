package main

import (
	"bufio"
	"fmt"
	"log"
	"strconv"
	"strings"
	"time"

	"github.com/jacobsa/go-serial/serial"
	"github.com/tidwall/gjson"
)

func main() {
	// Configure serial port options
	options := serial.OpenOptions{
		PortName:        "/dev/cu.usbserial-0001",
		BaudRate:        921600, // Supports 921600 baud rate
		DataBits:        8,
		StopBits:        1,
		MinimumReadSize: 1,
	}

	// Open the serial port
	port, err := serial.Open(options)
	if err != nil {
		log.Fatalf("serial.Open: %v", err)
	}

	defer port.Close()

	// Create a buffered reader to read from the serial port line-by-line
	reader := bufio.NewReader(port)

	// Dictionary to store counts for each board's sensor types
	boardData := make(map[string]map[string]int)

	// Store the last time we calculated frequencies
	lastTime := time.Now()
	interval := time.Second

	for {
		// Read a line from the serial port (this mimics Python's readline())
		serialData, err := reader.ReadString('\n')
		if err != nil {
			log.Fatal(err)
		}

		serialData = strings.TrimSpace(serialData)

		// Extract BoardID and SensorType using gjson
		boardIDHex := gjson.Get(serialData, "BoardID").String()
		sensorType := gjson.Get(serialData, "SensorType").String()
		boardIDString := boardIDHex
		// Skip if BoardID or SensorType is missing
		if boardIDString == "" || sensorType == "" {
			//fmt.Println("Skipping line due to missing BoardID or SensorType")

			continue
		} else {
			boardIDInt, err := strconv.ParseInt(boardIDHex, 16, 64)
			if err != nil {
				log.Fatalf("Error converting BoardID to integer: %v", err)
			}
			boardIDDivided := boardIDInt / 16
			boardIDString = fmt.Sprintf("%d", boardIDDivided)
		}

		// Update count for the specific board and sensor type
		if boardData[boardIDString] == nil {
			boardData[boardIDString] = make(map[string]int)
		}
		boardData[boardIDString][sensorType]++

		// Check if the interval has passed
		if time.Since(lastTime) >= interval {
			fmt.Println("\nData summary for the past second:")

			// Print data for each board ID
			for boardID, sensorData := range boardData {
				fmt.Printf("Board ID: %s\n", boardID)
				for sensorType, count := range sensorData {
					fmt.Printf("  Sensor Type %s: %d Hz\n", sensorType, count)
				}
			}

			// Reset the counts and timer
			boardData = make(map[string]map[string]int)
			lastTime = time.Now()
		}
	}
}
