#include <WiFi.h>
#include <esp_now.h>

/* Read Temperature and Light Sensor*/
#include <stdint.h>
#define microphonePin 7  //GPIO pin, check the pin number an dfunction of your board
#define threshold 500
#define boardOneTemp 1
#define boardOneLux 3
#define boardTwoTemp 2
#define boardTwoLux 4

unsigned long lastClapTime;
const int cooldownPeriod = 1000;
bool buzzer = false;  // virtual buzzer
bool clapDetected = false;  // are two claps detected or not
bool sentAck = true;  // acknowledgement sent status
bool dataOnSendingMode = false;
int clapCount = 0;

// Replace with the MAC address of the receiver ESP32S3 board
uint8_t broadcastAddress[] = {0x34, 0x85, 0x18, 0xAC, 0xB4, 0xF8}; // mac of master device

// Structure example to send data
// Must match the receiver structure
typedef struct struct_message {
  // char a[32];
  int id; 
  bool ack;
  float b;
  float c;
  // uint8_t crc; //CRC for error checking
  bool d;  // buzzer status
} struct_message;

/*------------------ CRC-8 Calculation-----------------------*/
// CRC-8 lookup table for polynomial 0x8C (reverse of 0x31)
const uint8_t crc8_table[256] = {
0x00, 0x8C, 0x94, 0x18, 0xA4, 0x28, 0x30, 0xBC, 0xC4, 0x48, 0x50, 0xDC, 0x60, 0xEC, 0xF4, 0x78,
0x04, 0x88, 0x90, 0x1C, 0xA0, 0x2C, 0x34, 0xB8, 0xC0, 0x4C, 0x54, 0xD8, 0x64, 0xE8, 0xF0, 0x7C,
0x08, 0x84, 0x9C, 0x10, 0xAC, 0x20, 0x38, 0xB4, 0xCC, 0x40, 0x58, 0xD4, 0x68, 0xE4, 0xFC, 0x70,
0x0C, 0x80, 0x98, 0x14, 0xA8, 0x24, 0x3C, 0xB0, 0xC8, 0x44, 0x5C, 0xD0, 0x6C, 0xE0, 0xF8, 0x74,
0x10, 0x9C, 0x84, 0x08, 0xB4, 0x38, 0x20, 0xAC, 0xD4, 0x58, 0x40, 0xCC, 0x70, 0xFC, 0xE4, 0x68,
0x14, 0x98, 0x80, 0x0C, 0xB0, 0x3C, 0x24, 0xA8, 0xD0, 0x5C, 0x44, 0xC8, 0x74, 0xF8, 0xE0, 0x6C,
0x18, 0x94, 0x8C, 0x00, 0xBC, 0x30, 0x28, 0xA4, 0xDC, 0x50, 0x48, 0xC4, 0x78, 0xF4, 0xEC, 0x60,
0x1C, 0x90, 0x88, 0x04, 0xB8, 0x34, 0x2C, 0xA0, 0xD8, 0x54, 0x4C, 0xC0, 0x7C, 0xF0, 0xE8, 0x64,
0x20, 0xAC, 0xB4, 0x38, 0x84, 0x08, 0x10, 0x9C, 0xE4, 0x68, 0x70, 0xFC, 0x40, 0xCC, 0xD4, 0x58,
0x24, 0xA8, 0xB0, 0x3C, 0x80, 0x0C, 0x14, 0x98, 0xE0, 0x6C, 0x74, 0xF8, 0x44, 0xC8, 0xD0, 0x5C,
0x28, 0xA4, 0xBC, 0x30, 0x8C, 0x00, 0x18, 0x94, 0xEC, 0x60, 0x78, 0xF4, 0x48, 0xC4, 0xDC, 0x50,
0x2C, 0xA0, 0xB8, 0x34, 0x88, 0x04, 0x1C, 0x90, 0xE8, 0x64, 0x7C, 0xF0, 0x4C, 0xC0, 0xD8, 0x54,
0x30, 0xBC, 0xA4, 0x28, 0x94, 0x18, 0x00, 0x8C, 0xF4, 0x78, 0x60, 0xEC, 0x50, 0xDC, 0xC4, 0x48,
0x34, 0xB8, 0xA0, 0x2C, 0x90, 0x1C, 0x04, 0x88, 0xF0, 0x7C, 0x64, 0xE8, 0x54, 0xD8, 0xC0, 0x4C,
0x38, 0xB4, 0xAC, 0x20, 0x9C, 0x10, 0x08, 0x84, 0xFC, 0x70, 0x68, 0xE4, 0x58, 0xD4, 0xCC, 0x40,
0x3C, 0xB0, 0xA8, 0x24, 0x98, 0x14, 0x0C, 0x80, 0xF8, 0x74, 0x6C, 0xE0, 0x5C, 0xD0, 0xC8, 0x44
};
// Function to calculate CRC-8
uint8_t calculateCRC8(const void* data, size_t length) {
    uint8_t crc = 0;
    uint8_t* buffer = (uint8_t*)data;

    for (size_t i = 0; i < length; i++) {
        crc = crc8_table[crc ^ buffer[i]];
    }

    return crc;
}
// Create a struct_message called myData
// struct_message myData;
struct_message sender_master;
struct_message ack_message;
struct_message boardOne;
struct_message boardTwo;
esp_now_peer_info_t peerInfo;

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

// callback function that will be executed when data is received
void OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len) {
  char macStr[18];
  Serial.print("Packet received from: ");
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.println(macStr);
  memcpy(&sender_master, incomingData, sizeof(sender_master));
  uint8_t ReceivedCRC = incomingData [len - 1];
  uint8_t calculatedCRC;
  int CRCCheckSum;

  if (sender_master.id == 99) {
    if (sender_master.d) {
      buzzer = true;
      sentAck = false;
      clapDetected = false;
    }else {
      buzzer = false;
      sentAck = false;
      clapDetected = true;
    }
  }
  delay(500);
}

void detectTwoClaps() {
  Serial.println("clap twice to activate buzzer");
  int clapCount = 0;
  unsigned long runTime = millis();
  while ((clapCount < 2) && (runTime < 5000)) {
    int sensorValue = analogRead(microphonePin);
    // unsigned long currentTime = millis();
    if (sensorValue > threshold) {
      // if ((runTime - lastClapTime) > coolDownPeriod) {
      clapCount += 1;
      //lastClapTime = runTime;
      // }
    }
  }
  // delay(5000);
}

void sendAck(int id, bool ack, bool d) {
  ack_message.id = id;
  ack_message.ack = ack;
  ack_message.d = d;

  uint8_t CRC =calculateCRC8(&ack_message, sizeof(ack_message));
  // Send data including CRC using ESP-NOW
  uint8_t dataToSend[sizeof(ack_message)+1];
  memcpy(dataToSend,&ack_message,sizeof(ack_message));
  dataToSend[sizeof(ack_message)] = CRC;

  // send message via esp_now
  esp_err_t result = esp_now_send(broadcastAddress, dataToSend, sizeof(dataToSend));
  delay(100);

  if (result == ESP_OK) {
    Serial.print(F("  buzzer status "));
    Serial.print(buzzer);
    Serial.println(" Acknowledgement 1: Sent with success");
    Serial.print("  Calculated CRC: ");
    Serial.println(dataToSend[sizeof(ack_message)]);
  }
  else {
    Serial.println("Error sending the data");
  }

  sentAck = true;
  delay(100);
}
 
void setup() {
  // Init Serial Monitor
  Serial.begin(115200);
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);

  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  delay(1000);
  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
  //Set values to send
  if (buzzer) {
    if (!sentAck) {
      detectTwoClaps();
      // unsigned long currentTime = millis();
      // while(clapCount < 2) {
      //   int sensorValue = analogRead(microphonePin);
      //   if (sensorValue > threshold) {
      //     if (currentTime - lastClapTime >= 1000) {
      //       // A new clap has been detected after the cooldown period
      //       clapCount = 1;
      //       Serial.print("Clap ");
      //       Serial.print(clapCount);
      //       Serial.println(" detected!");
      //       lastClapTime = currentTime;
      //     } else if (clapCount == 1) {
      //       // A second clap is detected within the cooldown period
      //       clapCount = 2;
      //       Serial.println("Second clap detected!");
      //       // Add your desired action when double clap is detected.
      //     }
      //   }
      // }
      sendAck(1, true, true);
      // sentAck = true;
    }
  }else {
    if (!sentAck) {
      sendAck(1, true, false);
    }
  }
  delay(1000);
  int sensorValue_tmone= analogRead(boardOneTemp);
  int sensorValue_opone = analogRead(boardOneLux);
  int sensorValue_tmtwo= analogRead(boardTwoTemp);
  int sensorValue_optwo = analogRead(boardTwoLux);
  // Convert the analog reading ADC:12bit (which goes from 0 - 4095) to a voltage (0 - 5V):
  float voltage_tmone = sensorValue_tmone * (3.3 / 4096.0); 
  float voltage_opone = sensorValue_opone * (3.3 / 4096.0);
  float voltage_tmtwo = sensorValue_tmtwo * (3.3 / 4096.0); 
  float voltage_optwo = sensorValue_optwo * (3.3 / 4096.0);

  boardOne.id=3;
  boardOne.b = -(22.2*voltage_tmone) + 61.6;
  boardOne.c = 142*pow(voltage_opone, -1.17);
  
  boardTwo.id=4;
  boardTwo.b = 54.5 - 42.6*log(voltage_tmtwo);
  boardTwo.c = 1299*exp(-1.96*voltage_optwo);

  // Calculate CRC  
  uint8_t CRCone =calculateCRC8(&boardOne, sizeof(boardOne));

  // Send data including CRC using ESP-NOW
  uint8_t dataToSendone[sizeof(boardOne)+1];
  memcpy(dataToSendone,&boardOne,sizeof(boardOne));
  dataToSendone[sizeof(boardOne)] = CRCone;
  esp_err_t resultone = esp_now_send(broadcastAddress, dataToSendone, sizeof(dataToSendone));

  if (resultone == ESP_OK) {
    Serial.print(F("  Temperature voltage: "));
    Serial.print(voltage_tmone);
    Serial.print(F("V  Light voltage: "));
    Serial.print(voltage_opone);
    Serial.print(F("V  "));
    Serial.println(" Sender 2: Sent with success");
    Serial.print("  Calculated CRC: ");
    Serial.println(dataToSendone[sizeof(boardOne)]);
  }
  else {
    Serial.println("Error sending the data");
  }
  delay(1000);

  // Calculate CRC  
  uint8_t CRCtwo =calculateCRC8(&boardTwo, sizeof(boardTwo));

  // Send data including CRC using ESP-NOW
  uint8_t dataToSendtwo[sizeof(boardTwo)+1];
  memcpy(dataToSendtwo,&boardTwo,sizeof(boardTwo));
  dataToSendtwo[sizeof(boardTwo)] = CRCtwo;
  esp_err_t resulttwo = esp_now_send(broadcastAddress, dataToSendtwo, sizeof(dataToSendtwo));

  if (resulttwo == ESP_OK) {
    Serial.print(F("  Temperature voltage: "));
    Serial.print(voltage_tmtwo);
    Serial.print(F("V  Light voltage: "));
    Serial.print(voltage_optwo);
    Serial.print(F("V  "));
    Serial.println(" Sender 2: Sent with success");
    Serial.print("  Calculated CRC: ");
    Serial.println(dataToSendtwo[sizeof(boardTwo)]);
  }
  else {
    Serial.println("Error sending the data");
  }
  delay(1000);
}