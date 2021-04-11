#include <Arduino.h>

/*
static const uint8_t D0   = 16;
static const uint8_t D1   = 5;
static const uint8_t D2   = 4;
static const uint8_t D3   = 0;
static const uint8_t D4   = 2;
static const uint8_t D5   = 14;
static const uint8_t D6   = 12;
static const uint8_t D7   = 13;
static const uint8_t D8   = 15;
static const uint8_t D9   = 3;
static const uint8_t D10  = 1;
*/
#include "config.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Streaming.h>
#include <OneWire.h>
#include <DallasTemperature.h>


#define R0 5            // Relay 0 D1
#define R1 4            // Relay 1 D2
#define R2 0            // Relay 2 D3
#define R3 2            // Relay 3 D4

#define SLEEP_DELAY_IN_SECONDS  10
#define ONE_WIRE_BUS 14  // DS18B20 on arduino pin14 corresponds to D5 on physical board
#define MQTT_MAX_PACKET_SIZE 256

const int AnalogIn  = A0;
int readingIn = 0;

// Generally, you should use "unsigned long" for variables that hold time
// The value will quickly become too large for an int to store
unsigned long previousMillis = 0;        // will store last time LED was updated

// constants won't change :
const long interval = 6000;

WiFiClient espClient;
PubSubClient client(espClient);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);

float getTemperature() {
  float temp;
  do {
    DS18B20.requestTemperatures(); 
    temp = DS18B20.getTempCByIndex(0);
    delay(100);
  } while (temp == 85.0 || temp == (-127.0));
  return temp;
}

void send_messure_data(){
  // read the input pin
  char result[8]; // Buffer big enough for 7-character float 
  float temperature = getTemperature();
  dtostrf(temperature, 6, 2, result);

  readingIn = analogRead(AnalogIn);    // read the input pin
  // Convert data to JSON string 
  String json =
  "{"
  "\"temperature\": \"" + String(temperature) + "\","
  "\"analog\": \"" + String(readingIn) + "\","
  "\"relay_1\": \"" + digitalRead(R0) + "\","
  "\"relay_2\": \"" + digitalRead(R1) + "\","
  "\"relay_3\": \"" + digitalRead(R2) + "\","
  "\"relay_4\": \"" + digitalRead(R3) + "\"}";
  // Convert JSON string to character array
  // Serial.print("json length: ");
  // Serial.println(json.length()+1);
  char jsonChar[200];
  json.toCharArray(jsonChar, json.length()+1);
  // Publish JSON character array to MQTT topic
  // mqtt_topic = "/Gasmads/Outdoor/07/json/";

  if( client.publish(mqtt_topic,jsonChar)){
    Serial.println(json); 
  }
  else{
    Serial.println("Ikke Sendt");      
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  char message_buff[100];
  int x = 0;
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for ( unsigned int i = 0; i < length; i++) {
    //Serial.print((char)payload[i]);
    message_buff[i] = payload[i];
    x++;
  }
  message_buff[x] = '\0';
  String msgString = String(message_buff);
  Serial.println("Payload: " + msgString);

  switch (msgString[0]){
    case '0':     //Det er et alle kald
    if(msgString[2]=='1'){
      digitalWrite(R0, LOW);
      digitalWrite(R1, LOW);
      digitalWrite(R2, LOW);
      digitalWrite(R3, LOW);
    }
    else{
      digitalWrite(R0, HIGH);
      digitalWrite(R1, HIGH);
      digitalWrite(R2, HIGH);
      digitalWrite(R3, HIGH);
    }
    case '1':
    if(msgString[2]=='1'){
      digitalWrite(R0, LOW);
    }
    else{
      digitalWrite(R0, HIGH);
    }
    break;
    case '2':
    if(msgString[2]=='1'){
      digitalWrite(R1, LOW);
    }
    else{
      digitalWrite(R1, HIGH);
    }
    break;
    case '3':
    if(msgString[2]=='1'){
      digitalWrite(R2, LOW);
    }
    else{
      digitalWrite(R2, HIGH);
    }
    break;
    case '4':
    if(msgString[2]=='1'){
      digitalWrite(R3, LOW);
    }
    else{
      digitalWrite(R3, HIGH);
    }
    break;
  }
  send_messure_data();
}


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    Serial.println (mqtt_username);
    Serial.println (mqtt_password);
    if (client.connect("GasmadsO02", mqtt_username, mqtt_password)) {
      Serial.println("connected");
      client.subscribe("/Gasmads/Outdoor/02/input/#");  
      Serial.println("subscribe");
      //client.publish(mqtt_topic, "Så er vi igang");
    }
    else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  // setup serial port
  Serial.begin(115200);
  // setup WiFi
  setup_wifi();
  //DS18B20.begin();
  //client.setServer(mqtt_server, 17229);
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  pinMode(R0, OUTPUT);    
  pinMode(R1, OUTPUT);    
  pinMode(R2, OUTPUT);    
  pinMode(R3, OUTPUT);    
  digitalWrite(R0, HIGH);
  digitalWrite(R1, HIGH);
  digitalWrite(R2, HIGH);
  digitalWrite(R3, HIGH);
}

void loop() {
  unsigned long currentMillis = millis();
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    send_messure_data();
  }
}