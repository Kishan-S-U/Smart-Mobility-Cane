#include <HardwareSerial.h>
#include <WiFi.h>
#include "ThingSpeak.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
 
const char* ssid="Kishan"; //Type wifi name  
const char* password="jiia76s2";//TYpe wifi password
const String apikey = "dF0ko0QQoEAfE9RJYoMWxbRIiFAzEx_05OilIPeFbUX";

DynamicJsonDocument doc(1024);
String json;

HardwareSerial SerialPort(2); //use UART2
#define UART_TX_PIN 43
#define UART_RX_PIN 44
#define BAUD_RATE 115200
int led = 13;
 
typedef struct UART_message {
  // char a[32];
  int id; 
  bool ack;
  float b;
  float c;
  // uint8_t crc; //CRC for error checking
  // bool d;  // buzzer status
  uint8_t CRC_checksum; //CRC for error checking
  bool d;
} UART_message;
 
WiFiClient client;
 
unsigned long myChannelNumber = 1; //The channel number in ThingSpeak (e.g.Channel 1)
const char* myWriteAPIKey = "4C5ZTR1LVTOFWXMA"; //Write API Key, can be obtained from ThingSpeak,differenct channel has different API keyTimer variables
 
unsigned long lastTime = 0;
unsigned long timerDelay = 20000;

bool triggerEmail(float lat, float lon) {
  if (WiFi.status() == WL_CONNECTED) {
    doc["value1"] = "kishan.ncl20@gmail.com";
    doc["value2"] = lat;
    doc["value3"] = lon;
    serializeJson(doc, json);
    serializeJson(doc, Serial);

    HTTPClient http;

    String url = "https://maker.ifttt.com/trigger/Distress_call/with/key/";
    url += apikey;

    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    int httpcode = http.POST(json);
    Serial.println(httpcode);
    http.end();
    return true;
  }
  return false;
}

void setup() {
  Serial.begin(BAUD_RATE);
  SerialPort.begin(BAUD_RATE, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
  WiFi.mode(WIFI_STA);  
  WiFi.begin(ssid, password);
  if(WiFi.status() != WL_CONNECTED){
      Serial.print("Attempting to connect Wi-Fi");
      while(WiFi.status() != WL_CONNECTED){
        WiFi.begin(ssid, password);
        delay(5000);    
      }
      Serial.println("\nWiFi Connected.");
  }
  Serial.print("RSSI: ");
  Serial.println(WiFi.RSSI());
  pinMode(led,OUTPUT);
  ThingSpeak.begin(client);  // Initialize ThingSpeak
  delay(1000);    
}
 
void loop() {
 
// Variable definition for uploading data to ThingSpeak
    float humidity;
    float temperature;
    float TempVoltage;
    float LightVoltage;
    float LightDensity;
 
// Serial Port Scanning    
  if (SerialPort.available()) {
    // String message = SerialPort.readString();
    // Serial.println("Received message: " + message);
    size_t byteSize = sizeof(UART_message);
    byte* byteArray = new byte[byteSize];
    SerialPort.read(byteArray, byteSize);
 
    // Deserialize the byte array back into the structure
    UART_message UARTReceivedData;
    memcpy(&UARTReceivedData, byteArray, byteSize);
 
/*--------Process different types of data----------*/
    switch(UARTReceivedData.id){
       case 1:      
              // Print the received structure data and check the data valid or not
                Serial.print("Received from Board ID: ");
                Serial.println(UARTReceivedData.id);    
                Serial.print("   Mic status ");
                Serial.print(UARTReceivedData.d);
                Serial.print("    CRC = ");  // CRC=1 means data transmission right
                Serial.println(UARTReceivedData.CRC_checksum);
                ThingSpeak.setField(1, UARTReceivedData.d);
              break;
       case 2:
               Serial.print("Received from Board ID: ");
                Serial.println(UARTReceivedData.id);    
                Serial.print("   Stick status ");
                Serial.print(UARTReceivedData.d);
                Serial.print("    CRC = ");  // CRC=1 means data transmission right
                Serial.println(UARTReceivedData.CRC_checksum);
                ThingSpeak.setField(2, UARTReceivedData.d);
              break;
      case 3:
               if (UARTReceivedData.b > 0 ){
                  Serial.print("Received from Board ID: ");
                  Serial.println(UARTReceivedData.id);    
                  Serial.print("   temp: ");
                  Serial.print(UARTReceivedData.b);   // b is linked to light desnity
                  Serial.print("   LUX: ");
                  Serial.print(UARTReceivedData.b); 
                  Serial.print("    CRC = ");  // CRC=1 means data transmission right
                  Serial.println(UARTReceivedData.CRC_checksum);
                  ThingSpeak.setField(3, UARTReceivedData.b);
              }                    
              break;
      case 4:
               if (UARTReceivedData.b > 0 ){
                  Serial.print("Received from Board ID: ");
                  Serial.println(UARTReceivedData.id);    
                  Serial.print("   temp: ");
                  Serial.print(UARTReceivedData.b);   // b is linked to light desnity
                  Serial.print("   LUX: ");
                  Serial.print(UARTReceivedData.b); 
                  Serial.print("    CRC = ");  // CRC=1 means data transmission right
                  Serial.println(UARTReceivedData.CRC_checksum);
                  ThingSpeak.setField(4, UARTReceivedData.b);
              }   
              break;
      
      case 5:
               if (UARTReceivedData.b > 0 && UARTReceivedData.b > 0){
                  Serial.print("Received from Board ID: ");
                  Serial.println(UARTReceivedData.id);
                  Serial.print("   latitude: ");
                  Serial.print(UARTReceivedData.b);   // b is linked to light desnity
                  Serial.print("   longitude: ");
                  Serial.print(UARTReceivedData.b); 
                  Serial.print("    CRC = ");  // CRC=1 means data transmission right
                  Serial.println(UARTReceivedData.CRC_checksum);
                  bool t = triggerEmail(UARTReceivedData.b, UARTReceivedData.c);
                  if (t) {
                    Serial.println("message send to a friend");
                  }
              }else {
                bool t = triggerEmail(0.0, 0.0);
                  if (t) {
                    Serial.println("message send to a friend");
                  }
              }
              break;
    }
    digitalWrite(led, HIGH);
    delay(100);
 
  //ThingSpeak Data UIploading
  if ((millis() - lastTime) > timerDelay) {
 
    // Connect or reconnect to WiFi
 
    // if(WiFi.status() != WL_CONNECTED){
    //   Serial.print("Attempting to connect");
    //   while(WiFi.status() != WL_CONNECTED){
    //     WiFi.begin(ssid, password);
    //     delay(5000);    
    //   }
    //   Serial.println("\nConnected.");
    }
    // Get a new temperature reading
      // ThingSpeak.setField(1, humidity);
      // ThingSpeak.setField(2, temperature);
      // ThingSpeak.setField(3, TempVoltage);
      // ThingSpeak.setField(4, LightVoltage);
      // ThingSpeak.setField(5, LightDensity);
 
    // Write to ThingSpeak. There are up to 8 fields in a channel, allowing you to store up to 8 different
    // pieces of information in a channel.  Here, we write to field 1.
    int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
 
    if(x == 200){
      Serial.println("Channel update successful.");
    }
    else{
      Serial.println("Problem updating channel. HTTP error code " + String(x));
    }
    lastTime = millis();
  }
//  }
  digitalWrite(led, LOW);//digitalWrite(LED, HIGH);
 
}