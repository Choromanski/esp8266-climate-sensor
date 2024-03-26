#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Wire.h>
#include <BH1750.h>
#include <WEMOS_SHT3X.h>
#include <Adafruit_SGP30.h>

/* WiFi and IP connection info. */		// UPDATE THESE
const char* ssid = "changeme";			// SSID of WiFi Network
const char* password = "changeme";		// Password of WiFi Network
const char* deviceHostname = "";		// Optional but suggested. Custom hostname to help find device on network
const int port = 9926;					// Port to bind webserver

/* INIT LIBRARIES */
SHT3X sht30(0x45); 				// SHT30 - Temp and Hum
BH1750 lightMeter(0x23); 		// BH1750 - Light Lux
Adafruit_SGP30 sgp30;			// SGP30 - eCO2 and TVOC
ESP8266WebServer server(port);	// WebServer port

/* FUNCTION DECLARATIONS */		// Function need to be declared before they are called
uint16_t takeLightReading();	// Measures Light Lux
bool takeAirReading();			// Measures eCO2 and Total Volatile Organic Compounds (TVOC)
bool takeClimateReading(bool);	// Measures Temperature and Humidity Percentage
uint8_t takeAllReadings(bool);	// Measures Light, eCO2, TVOC, Temperature and Humidity
void HandleRoot();				// Handler page to return sensor data
void HandleNotFound();			// Handler page for undefined server paths
String generateDataPage();		// Generates sensor data page text


/* STATIC VARIABLES */			// Used to store the last value fo each metric
uint16_t light_level_reading; 	// light level in lux (1 - 65535)
uint16_t eco2_reading;			// eCO2 Level in ppm (400-60000)
uint16_t tvoc_reading;			// Total Volatile Organic Compounds (TVOC) in ppb (0-60000)
float temperature_reading;		// Temperature in fahrenheit/celsius depending on `temp_in_c` arg
float humidity_reading;			// Relative Humidity percentage

void setup() {
	Serial.begin(115200);
	Wire.begin();

	/* STARTS BH1750 - Light Level */
	Serial.println("Initialising BH1750");
	if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
		Serial.println(F("BH1750 Advanced begin"));
	} else {
		Serial.println(F("Error initialising BH1750"));
	}

	/* STARTS SGP30 - eCO2 ant TVOC */
	Serial.println("Initialising SGP30");
	if (sgp30.begin()){
		Serial.print("Found SGP30 serial #");
		Serial.print(sgp30.serialnumber[0], HEX);
		Serial.print(sgp30.serialnumber[1], HEX);
		Serial.println(sgp30.serialnumber[2], HEX);
	} else {
		Serial.println(F("Error initialising SGP30"));
	}

	/* CONNECT TO WIFI */
	Serial.println("Initialising WiFi");
	// Set WiFi mode to client (without this it may try to act as an AP)
	WiFi.mode(WIFI_STA);
	// Configure Hostname (this can make it easier to find device on network)
	if ((deviceHostname != NULL) && (deviceHostname[0] == '\0')) {
		// Board defaults if `deviceHostname` is left empty
		Serial.println("No Device ID is Defined, Defaulting to board defaults");
	} else {
		wifi_station_set_hostname(deviceHostname);
		WiFi.setHostname(deviceHostname);
	}
	// Connects to WiFi and waits for successful connection
	Serial.print("Connecting to WiFi");
	WiFi.begin(ssid, password);
	while (WiFi.status() != WL_CONNECTED) {
		Serial.print(".");
		delay(500);
	}
	// Dump WiFi Connection
	Serial.print("\nConnected to ");
	Serial.println(ssid);
	Serial.print("IP address: ");
	Serial.println(WiFi.localIP());
	Serial.print("MAC address: ");
	Serial.println(WiFi.macAddress());
	Serial.print("Hostname: ");
	Serial.println(WiFi.hostname());

	/* CONFIGURE WEB SERVER */
	// Binds sensor data to `/` and `/metrics`
	server.on("/", HandleRoot);
	server.on("/metrics", HandleRoot);
	// Binds 404 page to missing server paths
	server.onNotFound(HandleNotFound);
	// Starts the web server
	server.begin();
	Serial.println("HTTP server started at  http://" + WiFi.localIP().toString() + ":" + String(port));

}

void loop() {
	// Run Web Server Client
	server.handleClient();
	/* Example code yoinked from sensor docs */
	/*
	// SHT30
	if(sht30.get()==0) {
		Serial.print("Temperature in Celsius : ");
		Serial.println(sht30.cTemp);
		Serial.print("Temperature in Fahrenheit : ");
		Serial.println(sht30.fTemp);
		Serial.print("Relative Humidity : ");
		Serial.println(sht30.humidity);
		Serial.println();
	} else {
		Serial.println("Error!");
	}

	// BH1750
	uint16_t lux = lightMeter.readLightLevel();
	Serial.print("Light: ");
	Serial.print(lux);
	Serial.println(" lx");

	// SGP30
	// CO2: 400 ppm	TVOC: 0 ppb
	delay(1000); //Wait 1 second
	if (sgp30.IAQmeasure())
	{

		Serial.print("CO2: ");
		Serial.print(sgp30.eCO2);
		Serial.print(" ppm\tTVOC: ");
		Serial.print(sgp30.TVOC);
		Serial.println(" ppb");
	}
	else
	{
		Serial.println("Error...\r\n");
		while (1)
			;
	}

	// DELAY
	delay(1000);
	*/
}

/* CUSTOM FUNCTIONS READ DOCSTRINGS */
uint16_t takeLightReading() {
	/**
	 * Takes the light reading of the BH1750 and stores it in the static variable `light_level_reading`
	 * 
	 * Variables: 
	 * 		uint16_t light_level_reading - light level in lux (1 - 65535)
	 * Returns: 
	 * 		uint16_t light_level_reading
	*/
	light_level_reading = lightMeter.readLightLevel();
	return light_level_reading;
}

bool takeAirReading(){
	/**
	 * Takes the eC02 and TVOC readings of the SGP30 and stores it in the static variables `eco2_reading` and `tvoc_reading`
	 * 
	 * Variables:
	 * 		uint16_t eco2_reading - eCO2 Level in ppm (400-60000)
	 * 		uint16_t tvoc_reading - Total Volatile Organic Compounds in ppb (0-60000)
	 * Returns: boolean value dependent on the success of the reading
	 * 		true - if reading is successfull
	 * 		false - if unsuccessfull reading
	 * Remarks:
	 * 		eCO2 is not a direct measurement of CO2 instead it's an estimation of CO2 based off the TVOC value 
	*/
	if (sgp30.IAQmeasure())	{
		eco2_reading = sgp30.eCO2;
		tvoc_reading = sgp30.TVOC;
		return true;
	} else {
		return false;
	} 
}

bool takeClimateReading(bool temp_in_c=false){
	/**
	 * Takes the temp and humidity readings of the SHT30 and stores it in the static variables `temperature_reading` and `humidity_reading`
	 * 
	 * Arguments:
	 * 		boolean temp_in_c:
	 * 			false [Default] - stores the temperature in fahrenheit in temperature_reading
	 * 			true - stores the temperature in celsius in temperature_reading
	 * Variables: 
	 * 		float temperature_reading - Temperature in fahrenheit/celsius depending on `temp_in_c` arg
	 * 		float humidity_reading - Relative Humidity percentage
	 * Returns: boolean value dependent on the success of the reading
	 * 		true - if reading is successfull
	 * 		false - if unsuccessfull reading
	*/
	if(sht30.get()==0) {
		if(temp_in_c){
			temperature_reading = sht30.cTemp;
		} else {
			temperature_reading = sht30.fTemp;
		}
		humidity_reading = sht30.humidity;
		return true;
	} else {
		return false;
	}
}

uint8_t takeAllReadings(bool temp_in_c=false){
	/**
	 * Takes the readings of all sensors (BH1750, SGP30, SHT30) and stores it in there coresponding static variables (`light_level_reading`, `eco2_reading`, `tvoc_reading`, `temperature_reading`, `humidity_reading`)
	 * 
	 * Arguments:
	 * 		boolean temp_in_c:
	 * 		false [Default] - stores the temperature in fahrenheit in temperature_reading
	 * 		true - stores the temperature in celsius in temperature_reading
	 * Variables:
	 * 		uint16_t light_level_reading - light level in lux (1 - 65535)
	 * 		uint16_t eco2_reading - eCO2 Level in ppm (400-60000)
	 * 		uint16_t tvoc_reading - Total Volatile Organic Compounds in ppb (0-60000)
	 * 		float temperature_reading - Temperature in fahrenheit/celsius depending on `temp_in_c` arg
	 * 		float humidity_reading - Relative Humidity percentage
	 * Returns: 2 bit value determined by the success of different sensors
	 * 		Bit #1 - SPG30 Status
	 * 		Bit #2 - SHT30 Status
	 * 		Ex.  1 0
	 * 		     │ └── SHT30 Unsuccessfull
	 * 		     └──── SPG30 Successful
	*/
	takeLightReading();
	bool sgp30Status = takeAirReading();
	bool sht30Status = takeClimateReading(temp_in_c);
	return (sgp30Status << 1) | sht30Status;
}

void HandleRoot() {
	/**
	 * Handler function to return sensor data
	 * 
	 * Returns:
	 * 		HTTP 200 and a page with the sensor data
	*/
	server.send(200, "text/plain", generateDataPage());
}

void HandleNotFound() {
	/**
	 * Handler function for undefined server paths
	 * 
	 * Returns: 
	 * 		HTTP 404 and a page with request data
	*/
	String message = "File Not Found\n\n";
	message += "URI: ";
	message += server.uri();
	message += "\nMethod: ";
	message += (server.method() == HTTP_GET) ? "GET" : "POST";
	message += "\nArguments: ";
	message += server.args();
	message += "\n";
	for (uint i = 0; i < server.args(); i++) {
		message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
	}
	server.send(404, "text/html", message);
}

String generateDataPage(){
	/**
	 * Generates page text with sensor data
	 * 
	 * Each line contains the metric name and value seperated by the device identifier string
	 * 		Ex. `eco2{id="Sensor1",mac="01:23:45:67:89:AB"}570`
	 * 			Metric Name = eCO2
	 * 			Metric Value = 570
	 * 			Device ID String = {id="Sensor1",mac="01:23:45:67:89:AB"}
	 * 			Device ID = Sensor1
	 * 			Device MAC Address = 01:23:45:67:89:AB
	 * Returns:
	 * 		String of all the device lines concatenated together with a newline
	 * Remarks:
	 * 		Metric Value is "FAILED" if the sensor failed to read.
	*/
	String message = "";
	String idString = "{id=\"" + String(deviceHostname) + "\",mac=\"" + WiFi.macAddress().c_str() + "\"}";
	uint8_t sensorStatus = takeAllReadings();

	message += "light" + idString + String(light_level_reading) + "\n";
	if (sensorStatus & 1){
		message += "eco2" + idString + String(eco2_reading) + "\n";
		message += "tvoc" + idString + String(tvoc_reading) + "\n";
	} else {
		message += "eco2" + idString + "FAILED\n";
		message += "tvoc" + idString + "FAILED\n";
	}
	if (sensorStatus & (1<<1)){
		message += "temp" + idString + String(temperature_reading) + "\n";
		message += "humid" + idString + String(humidity_reading) + "\n";
	} else {
		message += "temp" + idString + "FAILED\n";
		message += "humid" + idString + "FAILED\n";
	}
	return message;
}