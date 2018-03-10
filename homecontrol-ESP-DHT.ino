#include <ESP8266WiFi.h>
#include "DHT.h"
#include <ArduinoJson.h>

// Uncomment one of the lines below for whatever DHT sensor type you're using!
//#define DHTTYPE DHT11   // DHT 11
//#define DHTTYPE DHT21   // DHT 21 (AM2301)
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

void LEDflash(int sleepDelay = 300);

// Replace with your network details
const char* ssid = "SSID";
const char* password = "passwordHere";

// Web Server on port 80
WiFiServer server(80);

const int LEDPin = 2;

const int DHTPin = 12;
DHT dht(DHTPin, DHTTYPE);

const char* apiVersion = "v1";

IPAddress localIP;
IPAddress remoteIP;


// Temporary variables
static char celsiusTemp[7];
static char fahrenheitTemp[7];
static char humidityTemp[7];

void setup() {
	pinMode(LEDPin, OUTPUT);
	LEDoff();
	Serial.begin(115200);
	delay(1000);
	Serial.print("\n\nConnecting to ");
	Serial.println(ssid);
	WiFi.begin(ssid, password);

	while (WiFi.status() != WL_CONNECTED) {
		Serial.print(".");
		LEDflash(500);
	}

	Serial.println("");
	Serial.println("WiFi connected");

	server.begin();
	Serial.println("Web server running. Waiting for the ESP IP...");
	delay(10000);
	localIP = WiFi.localIP();
	Serial.println(localIP);
	LEDon();
	dht.begin();
}

void loop() {

	while (WiFi.status() != WL_CONNECTED) {
		Serial.print(".");
		LEDoff();
		delay(500);
	}

	WiFiClient client = server.available();
	if (client) {
		Serial.println("New client from ");
		//Serial.println(DisplayAddress(client.remoteIP));
		boolean blank_line = true;
		while (client.connected()) {
			if (client.available()) {
				char c = client.read();

				if (c == '\n' && blank_line) {
					float h = dht.readHumidity();
					float t = dht.readTemperature();
					if (isnan(h) || isnan(t)) {
						Serial.println("Failed to read from DHT sensor!");
						strcpy(celsiusTemp, "Failed");
						strcpy(humidityTemp, "Failed");

						client.println("HTTP/1.1 503 Service Unavailable");
						client.println("Content-Type: application/json");
						client.println("Connection: close");
						client.println();

						const size_t bufferSize = JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(5);
						DynamicJsonBuffer jsonBuffer(bufferSize);

						JsonObject& root = jsonBuffer.createObject();
						root["version"] = apiVersion;
						root["status"] = 503;
						root["message"] = "Failed to read from DHT sensor!";

						JsonObject& data = root.createNestedObject("data");
						

						root.printTo(Serial);
						root.printTo(client);
						client.stop();

						break;
					}
					else {
						// Computes temperature values in Celsius + Fahrenheit and Humidity
						float hic = dht.computeHeatIndex(t, h, false);
						dtostrf(hic, 6, 2, celsiusTemp);

						client.println("HTTP/1.0 200 OK");
						client.println("Content-Type: application/json");
						client.println("Connection: close");
						client.println();

						const size_t bufferSize = JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(5);
						DynamicJsonBuffer jsonBuffer(bufferSize);

						JsonObject& root = jsonBuffer.createObject();
						root["version"] = apiVersion;
						root["status"] = 200;
						root["message"] = "";

						JsonObject& data = root.createNestedObject("data");
						data["temperature"] = t;
						data["temperatureUnit"] = "C";
						data["humidity"] = h;
						data["heatIndex"] = hic;
						data["heatIndexUnit"] = "C";

						root["millisSinceBoot"] = millis();

						root.printTo(Serial);
						//root.prettyPrintTo(client);
						root.printTo(client);
						client.stop();
						break;
					}

				}
				if (c == '\n') {
					// when starts reading a new line
					blank_line = true;
				}
				else if (c != '\r') {
					// when finds a character on the current line
					blank_line = false;
				}
			}
		}
		// closing the client connection
		delay(1);
		client.stop();
		Serial.println("\nClient disconnected.");
		LEDflash(200);
	}
}

void LEDflash(int sleepDelay) {
	digitalWrite(LEDPin, !digitalRead(LEDPin));
	delay(sleepDelay/2);
	digitalWrite(LEDPin, !digitalRead(LEDPin));
	delay(sleepDelay/2);
}

void LEDon() {
	digitalWrite(LEDPin, LOW);
}
void LEDoff() {
	digitalWrite(LEDPin, HIGH);
}
