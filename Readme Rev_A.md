boards REVA

Requirements:
- 11-15 solenoids (at least 7 24V, at least 4 12 V, others can be either)
- 8 PTs (4 read during the test, none are differential to my knowledge)
- 5 TCs (use ADS1115 if possible)
- 1/2 flowmeters (use ADS1115 CPS but FFT if possible)

two ESP32:
	⁃	both chips access to CAN
	⁃	Common analog inputs (for redundancy)
	⁃	equal access to CAN


Daq
	⁃	ADS1015 x2 (8 PTs) (3300SPS)
	⁃	try to have sensor plugs on-board
	⁃	Output Capacitor to ground 
	⁃	Low freq (10uF)
	⁃	High frequency (1uF)
	⁃	Filter power input to ground 
	⁃	ADS1115 x 2 (8 TCs) (860SPS)
	⁃	voltage divider 2.5V reference on one side, single input on the other with capacitor to 2.5V
	⁃	two flowmeter pins 
	⁃	pins: gnd - 5V - signal 
	⁃	internal DAQ (0,5V,5 to 3.3voltage divider/0-20mA)

	•	LM2596-3.3 for 3.3V output.
	•	LM2596-5.0 is 5V output.
	•	LM2596-12 is 12V output.
And we should other appropriate components too.
	•	C1 should become 180uF (for LM2596-12)
	•	L1 = wire coil inductor, be 68uH (for LM2596-12)

Solenoids Board
	⁃	Try to have sensor plugs on-board
	⁃	24V switching supply?
	⁃	12V switching supply?
