#include <esp_now.h>
#include <WiFi.h>
#include <HardwareSerial.h>

HardwareSerial SerialPort(2); //use UART2

#define UART_TX_PIN 43
#define UART_RX_PIN 44
#define BAUD_RATE 115200
bool microphoneStatus = false;

bool stateChange = false;
bool previousState = false;

uint8_t xiaoOne[] = {0x34, 0x85, 0x18, 0x8E, 0x2E, 0x78}; // mac of xiao one

// Structure example to receive data
// Must match the sender structure
typedef struct struct_message {
  // char a[32];
  int id; 
  bool ack;
  float b;
  float c;
  // uint8_t crc; //CRC for error checking
  bool d;  // buzzer status
} struct_message;

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
struct_message myData;
struct_message xiao_one;
struct_message sender1;
struct_message sender2;
struct_message sender3;
UART_message dataUARTSend;
esp_now_peer_info_t xiaoOne_peerInfo;
// Create an array with all the structures
struct_message boardsStruct[3] = {sender1, sender2, sender3};

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  // dataOnSendingMode = false;
}

// callback function that will be executed when data is received
void OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len) {
  char macStr[18];
  Serial.print("Packet received from: ");
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.println(macStr);
  memcpy(&myData, incomingData, sizeof(myData));
  uint8_t ReceivedCRC = incomingData [len - 1];
  uint8_t calculatedCRC; 
  int CRCCheckSum;
  byte* byteArray;
/**********----Process different types of incoming data----**********/    
 switch (myData.id){
  
    case 1:
            calculatedCRC = calculateCRC8(&myData, sizeof(myData));  // Mind how many data types are sent,//Board ID 2
            boardsStruct[myData.id-1].ack = myData.ack;
            boardsStruct[myData.id-1].d = myData.d;
            Serial.printf("    Board ID %u: %u bytes\n", myData.id, len);
            Serial.printf("    Acknowledgement %d\n", boardsStruct[myData.id-1].ack);
            Serial.printf("    buzzer status %d\n", boardsStruct[myData.id-1].d);
            Serial.printf("    Calcultaed CRC: %d \n", calculatedCRC);
            Serial.printf("    Received CRC: %d \n", ReceivedCRC);
            if (calculatedCRC == ReceivedCRC){
              Serial.printf("    Data Tramission Right!");
              CRCCheckSum = 1;
            }
            else{
              Serial.printf("    Data Tramission Wrong!");
              CRCCheckSum = 0;
            }  
            Serial.println();
            // if (myData.d) {
            //   microphoneStatus = true;
            //   buzzerSignalSent = false;
            // }else {
            //   microphoneStatus = false;
            //   buzzerSignalSent = false;
            // }
            // UART Data Transmission
            dataUARTSend.id=myData.id;
            dataUARTSend.ack=boardsStruct[myData.id-1].ack;
            dataUARTSend.d=boardsStruct[myData.id-1].d; 
            dataUARTSend.CRC_checksum=CRCCheckSum;           
            byteArray = reinterpret_cast<byte*>(&dataUARTSend);
            SerialPort.write(byteArray,sizeof(dataUARTSend));  
            delay(1000);
            break;

    case 2:
            // Calculate CRC-8 for the received data
            calculatedCRC = calculateCRC8(&myData, sizeof(myData));  // Mind how many data types are sent, //Board ID 1
            boardsStruct[myData.id-1].d = myData.d;
            // boardsStruct[myData.id-1].c = myData.c;
            Serial.printf("    Board ID %u: %u bytes\n", myData.id, len);
            Serial.printf("    Stick status %d\n", boardsStruct[myData.id-1].d);
            // Serial.printf("    Light: %.2f C\n", boardsStruct[myData.id-1].c);
            Serial.printf("    Calcultaed CRC: %d \n", calculatedCRC);
            Serial.printf("    Received CRC: %d \n", ReceivedCRC);
            if (calculatedCRC == ReceivedCRC){
              Serial.printf("    Data Tramission Right!");
              CRCCheckSum = 1;
            }
            else{
              Serial.printf("    Data Tramission Wrong!");
              CRCCheckSum = 0;
            }  
            Serial.println();
            microphoneStatus = myData.d;
            if (previousState != microphoneStatus) {
              previousState = microphoneStatus;
              stateChange = true;
            }else {
              stateChange = false;
            }
            // UART Data Transmission
            dataUARTSend.id=myData.id;
            dataUARTSend.d=boardsStruct[myData.id-1].d;
            // dataUARTSend.c=boardsStruct[myData.id-1].c; 
            dataUARTSend.CRC_checksum=CRCCheckSum;           
            byteArray = reinterpret_cast<byte*>(&dataUARTSend);
            SerialPort.write(byteArray,sizeof(dataUARTSend));            
            delay(1000);
            break;
    case 3:
            calculatedCRC = calculateCRC8(&myData, sizeof(myData));  // Mind how many data types are sent,//Board ID 2
            boardsStruct[myData.id-1].b = myData.b;
            boardsStruct[myData.id-1].c = myData.c;
            Serial.printf("    Board ID %u: %u bytes\n", myData.id, len);
            Serial.printf("    temp %f\n", boardsStruct[myData.id-1].b);
            Serial.printf("    lux %f\n", boardsStruct[myData.id-1].c);
            Serial.printf("    Calcultaed CRC: %d \n", calculatedCRC);
            Serial.printf("    Received CRC: %d \n", ReceivedCRC);
            if (calculatedCRC == ReceivedCRC){
              Serial.printf("    Data Tramission Right!");
              CRCCheckSum = 1;
            }
            else{
              Serial.printf("    Data Tramission Wrong!");
              CRCCheckSum = 0;
            }  
            Serial.println();
            // UART Data Transmission
            dataUARTSend.id=myData.id;
            dataUARTSend.b=boardsStruct[myData.id-1].b;
            dataUARTSend.c=boardsStruct[myData.id-1].c; 
            dataUARTSend.CRC_checksum=CRCCheckSum;       
            byteArray = reinterpret_cast<byte*>(&dataUARTSend);
            SerialPort.write(byteArray,sizeof(dataUARTSend));  
            delay(1000);
            break;
    
    case 4:
            calculatedCRC = calculateCRC8(&myData, sizeof(myData));  // Mind how many data types are sent,//Board ID 2
            boardsStruct[myData.id-1].b = myData.b;
            boardsStruct[myData.id-1].c = myData.c;
            Serial.printf("    Board ID %u: %u bytes\n", myData.id, len);
            Serial.printf("    temp %f\n", boardsStruct[myData.id-1].b);
            Serial.printf("    lux %f\n", boardsStruct[myData.id-1].c);
            Serial.printf("    Calcultaed CRC: %d \n", calculatedCRC);
            Serial.printf("    Received CRC: %d \n", ReceivedCRC);
            if (calculatedCRC == ReceivedCRC){
              Serial.printf("    Data Tramission Right!");
              CRCCheckSum = 1;
            }
            else{
              Serial.printf("    Data Tramission Wrong!");
              CRCCheckSum = 0;
            }  
            Serial.println();
            // UART Data Transmission
            dataUARTSend.id=myData.id;
            dataUARTSend.b=boardsStruct[myData.id-1].b;
            dataUARTSend.c=boardsStruct[myData.id-1].c; 
            dataUARTSend.CRC_checksum=CRCCheckSum;       
            byteArray = reinterpret_cast<byte*>(&dataUARTSend);
            SerialPort.write(byteArray,sizeof(dataUARTSend));  
            delay(1000);
            break;
    
    case 5:
            calculatedCRC = calculateCRC8(&myData, sizeof(myData));  // Mind how many data types are sent,//Board ID 2
            boardsStruct[myData.id-1].b = myData.b;
            boardsStruct[myData.id-1].c = myData.c;
            Serial.printf("    Board ID %u: %u bytes\n", myData.id, len);
            Serial.printf("    lat %f\n", boardsStruct[myData.id-1].b);
            Serial.printf("    lon %f\n", boardsStruct[myData.id-1].c);
            Serial.printf("    Calcultaed CRC: %d \n", calculatedCRC);
            Serial.printf("    Received CRC: %d \n", ReceivedCRC);
            if (calculatedCRC == ReceivedCRC){
              Serial.printf("    Data Tramission Right!");
              CRCCheckSum = 1;
            }
            else{
              Serial.printf("    Data Tramission Wrong!");
              CRCCheckSum = 0;
            }  
            Serial.println();
            // UART Data Transmission
            dataUARTSend.id=myData.id;
            dataUARTSend.b=boardsStruct[myData.id-1].b;
            dataUARTSend.c=boardsStruct[myData.id-1].c; 
            dataUARTSend.CRC_checksum=CRCCheckSum;       
            byteArray = reinterpret_cast<byte*>(&dataUARTSend);
            SerialPort.write(byteArray,sizeof(dataUARTSend));  
            delay(1000);
            break;

    default:
            calculatedCRC = calculateCRC8(&myData, sizeof(myData)-sizeof(myData.c));  // Mind how many data types are sent //Board ID 3;
            boardsStruct[myData.id-1].b = myData.b;
            Serial.printf("    Board ID %u: %u bytes\n", myData.id, len);
            Serial.printf("    TSL2591 Light Density: %.2f lux\n", boardsStruct[myData.id-1].b);
            Serial.printf("    Calcultaed CRC: %d \n", calculatedCRC);
            Serial.printf("    Received CRC: %d \n", ReceivedCRC);
            if (calculatedCRC == ReceivedCRC){
              Serial.printf("    Data Tramission Right!");
              CRCCheckSum = 1;
            }
            else{
              Serial.printf("    Data Tramission Wrong!");
              CRCCheckSum = 0;
            }  
            Serial.println();

            // UART Data Transmission
            dataUARTSend.id=myData.id;
            dataUARTSend.b=boardsStruct[myData.id-1].b;
            dataUARTSend.CRC_checksum=CRCCheckSum;           
            byteArray = reinterpret_cast<byte*>(&dataUARTSend);
            SerialPort.write(byteArray,sizeof(dataUARTSend));  
            delay(1000);
            break;         
  }
}

void sendToXiaoOne(int id, bool d) {
  xiao_one.id = id;
  xiao_one.d = d;

  uint8_t CRC =calculateCRC8(&xiao_one, sizeof(xiao_one));
  // Send data including CRC using ESP-NOW
  uint8_t dataToSend[sizeof(xiao_one)+1];
  memcpy(dataToSend,&xiao_one,sizeof(xiao_one));
  dataToSend[sizeof(xiao_one)] = CRC;

  // send message via esp_now
  esp_err_t result = esp_now_send(xiaoOne, dataToSend, sizeof(dataToSend));
  delay(500);

  if (result == ESP_OK) {
    Serial.print(F("  buzzer status sent "));
    Serial.println(dataToSend[sizeof(xiao_one)]);
  }
  else {
    Serial.println("Error sending the data");
  }
  // buzzerSignalSent = true;
}

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  Serial.begin(BAUD_RATE);
  SerialPort.begin(BAUD_RATE, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);  
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_send_cb(OnDataSent);
  // Register peer
  memcpy(xiaoOne_peerInfo.peer_addr, xiaoOne, 6);
  xiaoOne_peerInfo.channel = 0;
  xiaoOne_peerInfo.encrypt = false;
  
  // Add peer        
  if (esp_now_add_peer(&xiaoOne_peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  
  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info
  esp_now_register_recv_cb(OnDataRecv);
}
 
void loop() {
  // sendToXiaoOne(99, true);
  // delay(10000);
  // sendToXiaoOne(99, false);
  // delay(10000);
  if (stateChange) {
    if (microphoneStatus) {
      sendToXiaoOne(99, true);
    }else {
      sendToXiaoOne(99, false);
    }
  }
}
