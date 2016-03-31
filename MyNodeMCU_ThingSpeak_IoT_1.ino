#include <ESP8266WiFi.h>

#include "DHT.h"
#include <OneWire.h>
#include <DallasTemperature.h>

// Wi-Fi point
const char* ssid     = "";
const char* password = "";

// For ThingSpeak IoT
const String CHANNELID_0 = "104514";
const String WRITEAPIKEY_0 = "JE9ZA95KD73VR0IJ";

IPAddress thingspeak_server(184, 106, 153, 149);
const int httpPort = 80;

WiFiClient client;

#define SERVER_UPDATE_TIME 60000  // Update ThingSpeak data server every 60000 ms (1 minute)
#define DHT_UPDATE_TIME 3000      // Update time for DHT sensors 
#define DS_UPDATE_TIME 1000       // Update time for Dallas sensors 

// DHT11 and DHT22
#define DHT11_PIN 5               // D1 on NodeMCU
#define DHT22_PIN 14              // D5 on NodeMCU
DHT dht11(DHT11_PIN, DHT11, 15);
DHT dht22(DHT22_PIN, DHT22, 15);

// DS18B20
#define ONE_WIRE_BUS 12           // D6 on NodeMCU
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature ds_sensors(&oneWire);

float h1 = 0;
float t1 = 0;
float t2 = 0;
float h2 = 0;
float t3 = 0;

unsigned long timer_main = 0;
unsigned long timer_thingspeak = 0;
unsigned long timer_ds18b20 = 0;
unsigned long timer_dht = 0;

#define TIMEOUT 1000 // 1 second timout

// Send IoT packet to ThingSpeak
void sendThingSpeakStream()
{
  Serial.print("Connecting to ");
  Serial.print(thingspeak_server);
  Serial.println("...");

  if (client.connect(thingspeak_server, httpPort))
  {
    if (client.connected())
    {
      Serial.println("Sending data to ThingSpeak server...\n");

      String post_data = "field1=";
      post_data = post_data + String(t2, 2);
      post_data = post_data + "&field2=";
      post_data = post_data + String(h2, 2);
      post_data = post_data + "&field3=";
      post_data = post_data + String(t1, 2);
      post_data = post_data + "&field4=";
      post_data = post_data + String(h1, 2);
      post_data = post_data + "&field5=";
      post_data = post_data + String(t3, 2);

      Serial.println("Data to be send:");
      Serial.println(post_data);

      client.println("POST /update HTTP/1.1");
      client.println("Host: api.thingspeak.com");
      client.println("Connection: close");
      client.println("X-THINGSPEAKAPIKEY: " + WRITEAPIKEY_0);
      client.println("Content-Type: application/x-www-form-urlencoded");
      client.print("Content-Length: ");
      int thisLength = post_data.length();
      client.println(thisLength);
      client.println();
      client.println(post_data);

      client.println();

      delay(1000);

      timer_thingspeak = millis();
      while ((client.available() == 0) && (millis() < timer_thingspeak + TIMEOUT));

      while (client.available() > 0)
      {
        char inData = client.read();
        Serial.print(inData);
      }
      Serial.println("\n");

      client.stop();
    }
  }
}

// Print sensors data to terminal
void printAllSensors()
{
  Serial.print("In temperature: ");
  Serial.print(t1);
  Serial.println(" *C");
  Serial.print("Out temperature: ");
  Serial.print(t2);
  Serial.println(" *C");
  Serial.print("Soil temperature: ");
  Serial.print(t3);
  Serial.println(" *C");
  Serial.print("In humidity: ");
  Serial.print(h1);
  Serial.println(" %");
  Serial.print("Out humidity: ");
  Serial.print(h2);
  Serial.println(" %");
  Serial.println("");
}

// Read DHT11 sensor
void readDHT11()
{
  // DHT11
  h1 = dht11.readHumidity();
  t1 = dht11.readTemperature();
  if (isnan(h1) || isnan(t1)) {
    Serial.println("Failed to read from DHT11 sensor!");
  }
}

// Read DHT22 sensor
void readDHT22()
{
  // DHT22
  h2 = dht22.readHumidity();
  t2 = dht22.readTemperature();
  if (isnan(h2) || isnan(t2)) {
    Serial.println("Failed to read from DHT22 sensor!");
  }
}

// Read DS18B20 sensor
void readDS18B20()
{
  // DS18B20
  ds_sensors.requestTemperatures();
  t3 = ds_sensors.getTempCByIndex(0);
  if (isnan(t3)) {
    Serial.println("Failed to read from DS18B20 sensor!");
  }
}

// Main setup
void setup()
{
  // Init serial port
  Serial.begin(115200);
  delay(100);
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  // Init Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  // Init DHT11
  dht11.begin();

  // Init DHT22
  dht22.begin();

  // Init DS18B20
  ds_sensors.begin();
}

// Main loop cycle
void loop()
{
  // Main timeout
  if (millis() > timer_main + SERVER_UPDATE_TIME)
  {
    // Print data from sensors
    printAllSensors();
    // Send data to ThingSpeak server
    sendThingSpeakStream();
    // Reset timeout timer
    timer_main = millis();
  }
  // DHT sensors timeout
  if (millis() > timer_dht + DHT_UPDATE_TIME)
  {
    readDHT11();
    readDHT22();
    timer_dht = millis();
  }
  // Pressure sensors timeout
  if (millis() > timer_ds18b20 + DS_UPDATE_TIME)
  {
    readDS18B20();
    timer_ds18b20 = millis();
  }
}


