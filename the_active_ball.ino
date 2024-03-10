
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_LIS3DH.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_NeoPixel.h>
#include <FastLED.h>
#include <esp_now.h>
#include "WiFi.h"

// Used for software SPI
#define LIS3DH_CLK 32
#define LIS3DH_MISO 15
#define LIS3DH_MOSI 33
// Used for hardware & software SPI
#define LIS3DH_CS 27
#define PIN            14
#define NUMPIXELS      60

float x = 0; float y = 0.0; float z = 0; 

bool ping = 0;
bool global = 0;
int address = 0;
int value = 0;
int sender = 2;

//software SPI
//Adafruit_LIS3DH lis = Adafruit_LIS3DH(LIS3DH_CS, LIS3DH_MOSI, LIS3DH_MISO, LIS3DH_CLK);
// hardware SPI
//Adafruit_LIS3DH lis = Adafruit_LIS3DH(LIS3DH_CS);
// I2C
Adafruit_LIS3DH lis = Adafruit_LIS3DH();

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

CRGB leds[NUMPIXELS];

uint8_t receiverAddress[][6] = {{0xf4,0xcf,0xa2,0xeb,0xfc,0x18},{0xec,0xfa,0xbc,0x58,0xad,0x58},{0x24,0x6f,0x28,0x02,0xa2,0x74},{0xf4,0xcf,0xa2,0xeb,0xfc,0xa9},{0xf4,0xcf,0xa2,0xec,0x04,0xb5},{0xcc,0x50,0xe3,0xe8,0x43,0xda},{0xf4,0xcf,0xa2,0xeb,0xfc,0xd6},{0x5c,0xcf,0x7f,0x6c,0xd8,0x15}};
/*
0 empty(varvara test device?)
1 Sushma
2 Varvara
3 Nasim
4 Supriya 
5 Zaq (not yet picked up) 43:da 
6 Razoul (not yet picked up) fc:d6
7 test device
*/

esp_now_peer_info_t peerInfo[8];

typedef struct customMessage {
    bool isGlobal;  //anotates if message is directed at some other device specifically or directly to a specific device
    int address;    //if not global, should have address 0-6
    bool isPing;    //true of message is ping, if not check value(payload)
    int value;      //value of payload 0-255
    int sender;     //our device ID 0-6
    
} customMessage;

customMessage myCustomMessage;  //receive message
customMessage myCustomToSend;   //send message

bool deviceChange = false;
bool globalPing = false;
bool directPing = false;
int globalMessage = 0;
int directMessage = 0;
int directMessageSender = 0;
bool state = false;

void lights() {
  for(int i=0;i<4;i++){   
    pixels.setPixelColor(i, pixels.Color(255,0,0)); // Red
  }
  for(int i=4;i<7;i++){   
    pixels.setPixelColor(i, pixels.Color(255,128,0)); // Orange
  }
  for(int i=7;i<11;i++){ 
    pixels.setPixelColor(i, pixels.Color(255,255,0)); // Yellow
  }
  for(int i=11;i<14;i++){
    pixels.setPixelColor(i, pixels.Color(0,255,0)); // Green
  }
  for(int i=14;i<18;i++){
    pixels.setPixelColor(i, pixels.Color(0,255,255)); // Turqoise
  }
  for(int i=18;i<21;i++){   
    pixels.setPixelColor(i, pixels.Color(0,0,255)); // Blue
  }
  for(int i=21;i<25;i++){
    pixels.setPixelColor(i, pixels.Color(102,0,204)); // Purple
  }
  for(int i=25;i<28;i++){
    pixels.setPixelColor(i, pixels.Color(255,0,255)); // Pink
  }
  for(int i=28;i<31;i++){
    pixels.setPixelColor(i, pixels.Color(255,0,0)); // Red
  }
  for(int i=31;i<34;i++){
    pixels.setPixelColor(i, pixels.Color(255,128,0)); // Orange
  }
  for(int i=34;i<38;i++){
    pixels.setPixelColor(i, pixels.Color(255,255,0)); // Yellow
  }
  for(int i=38;i<41;i++){
    pixels.setPixelColor(i, pixels.Color(0,255,0)); // Green
  }
  for(int i=41;i<45;i++){
    pixels.setPixelColor(i, pixels.Color(0,255,255)); // Turqoise
  }
  for(int i=45;i<48;i++){
     pixels.setPixelColor(i, pixels.Color(0,0,255)); // Blue
  }
  for(int i=48;i<52;i++){
    pixels.setPixelColor(i, pixels.Color(102,0,204)); // Purple
  }
  for(int i=52;i<55;i++){
    pixels.setPixelColor(i, pixels.Color(255,0,255)); // Pink
  }
  for(int i=55;i<60;i++){
    pixels.setPixelColor(i, pixels.Color(0,0,0));
  }
  pixels.show();
  delay(1000);

}

void messageSent(const uint8_t *macAddr, esp_now_send_status_t status) {
    Serial.printf("MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
    Serial.print("Send status: ");
    if(status == 0){
        Serial.println("Success");
    }
    else{
        Serial.println("Error");
    }
}

/**
 * @brief Callback function for receiving messages.
 * 
 * This function is called when a message is received.
 * It takes the MAC address of the sender, the incoming data, and the length of the data as parameters.
 * 
 * @param macAddr The MAC address of the sender.
 * @param incomingData The incoming data.
 * @param len The length of the incoming data.
 */
void messageReceived(const uint8_t* macAddr, const uint8_t* incomingData, int len){
    //triggered if message is received from esp
    memcpy(&myCustomMessage, incomingData, sizeof(myCustomMessage));
    Serial.printf("Incoming Message from: %02X:%02X:%02X:%02X:%02X:%02X \n\r", macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
    

}

/**
 * @brief Handles the message received from Arduino.
 * 
 * This function is responsible for processing the message received from Arduino.
 * 
 * @param message The message received from Arduino.
 */


void createAndSendMsgFromString(String message){ //handleMessageFromArduino
      
      //triggered if message is received from arduino
      //message is in format "01x1230"

      //first char isPing 0 or 1
      //second char isGlobal 0 or 1
      //third char if global insert any char (wont be read later anyway) else int 0-6
      //4th 5th and 6th if Ping insert any chars (wont be read later anyway) else int 000-255
      //7th int of my device 0-6

      //EXAMPLE "0031232" private msg
      // Ping   Glob  addr  val   sender
      // 0      0     3     123   2
      
      //EXAMPLE "11xyyy2" global ping 
      // Ping   Glob  addr  val   sender
      // 1      1     x     yyy   2

  bool isPing = (message[0] == '0') ? false : true;
  bool isGlobal = (message[1] == '0') ? false : true;
  int address;
  if(isGlobal){
    address = -1;
  }else{
    address = (message[2]- '0');
  }
  int value;
  if(isPing){
    value = 0;
  }else{
    value = 100*(message[3]- '0') + 10*(message[4]- '0') + (message[5]- '0');
  }
  int sender = message[6] - '0';


  myCustomToSend.isPing = isPing;
  myCustomToSend.isGlobal = isGlobal;
  myCustomToSend.address = address;
  myCustomToSend.value = value;
  myCustomToSend.sender = sender;
  //debugging
  Serial.print("\nisPing: ");
  Serial.println(myCustomToSend.isPing);
  Serial.print("isGlobal: ");
  Serial.println(myCustomToSend.isGlobal);
  Serial.print("address: ");
  Serial.println(myCustomToSend.address);
  Serial.print("value: ");
  Serial.println(myCustomToSend.value);
  Serial.print("sender: ");
  Serial.println(myCustomToSend.sender);
 
//SEND MESSAGE 
  if (address>=0){
    esp_now_send(receiverAddress[address], (uint8_t *) &myCustomToSend, sizeof(myCustomToSend));
  } else {
    for (int i=0; i<sizeof(receiverAddress) / sizeof(receiverAddress[0]);i++){
      esp_now_send(receiverAddress[i], (uint8_t *) &myCustomToSend, sizeof(myCustomToSend));
    }
  }
}

void handleMessageFromNetwork(String message){
  
  //ReadingMessage from network as int variables
  //------------------------------------------------------------
  bool isPing = (!myCustomMessage.isPing) ? false : true;
  bool isGlobal = (!myCustomMessage.isGlobal) ? false : true;
  int address;

  if(isGlobal){
    address = -1;
  }else{
    address = (myCustomMessage.address);
  }
  int value;

  if(isPing){
    value = 0;
  }else{
    value = myCustomMessage.value;
  }
  int sender = myCustomMessage.sender;

// Serial.println(message); 
// for (int i=0; i<7;i++){
//  Serial.print(message[i]); 
// }
  Serial.println(); 
  //handling message 
  //------------------------------------------------------------

  if(isPing && isGlobal){
    handleGlobalPing();
  }else if(isPing){
    handleDirectPing(sender);
  }else if(isGlobal){
    handleGlobalMessage(sender, value);
  }else{
    handleDirectMessage(sender, value);
  }
  deviceChange = true;

}

double accelometer_x(){
  lis.read();      // get X Y and Z data at once
  // Then print out the raw data
  //Serial.print("X:  "); Serial.print(lis.x);
  //Serial.print("  \tY:  "); Serial.print(lis.y);
  //Serial.print("  \tZ:  "); Serial.print(lis.z);
  /* Or....get a new sensor event, normalized */
  sensors_event_t event;
  lis.getEvent(&event);

  return event.acceleration.x;
}

double accelometer_y(){
  lis.read();      // get X Y and Z data at once
  // Then print out the raw data
  //Serial.print("X:  "); Serial.print(lis.x);
  //Serial.print("  \tY:  "); Serial.print(lis.y);
  //Serial.print("  \tZ:  "); Serial.print(lis.z);
  /* Or....get a new sensor event, normalized */
  sensors_event_t event;
  lis.getEvent(&event);

  return event.acceleration.y;
}

double accelometer_z(){
  lis.read();      // get X Y and Z data at once
  // Then print out the raw data
  //Serial.print("X:  "); Serial.print(lis.x);
  //Serial.print("  \tY:  "); Serial.print(lis.y);
  //Serial.print("  \tZ:  "); Serial.print(lis.z);
  /* Or....get a new sensor event, normalized */
  sensors_event_t event;
  lis.getEvent(&event);

  return event.acceleration.z;
}


//use this functions to trigger your actions

void handleGlobalPing(){
  globalPing = true;
  //pixels.begin();
  for(int i=0;i<NUMPIXELS;i++){
      pixels.setPixelColor(i, pixels.Color(255,0,0)); 
    }
  pixels.show();
  delay (1000);

}

void handleDirectPing(int sender){
  directPing = true;

  if (sender == 0) {
    for(int i=0;i<NUMPIXELS;i++){
      pixels.setPixelColor(i, pixels.Color(255,128,0)); 
    }
    pixels.show();
    delay (1000);
  }

  if (sender == 1) {
    for(int i=0;i<NUMPIXELS;i++){
      pixels.setPixelColor(i, pixels.Color(255,255,0)); 
    }
    pixels.show();
    delay (1000);
  }

  if (sender == 3) {
    for(int i=0;i<NUMPIXELS;i++){
      pixels.setPixelColor(i, pixels.Color(0,255,0)); 
    }
    pixels.show();
    delay (1000);
  }

  if (sender == 4) {
    for(int i=0;i<NUMPIXELS;i++){
      pixels.setPixelColor(i, pixels.Color(0,0,255)); 
    }
    pixels.show();
    delay (1000);
  }

  if (sender == 5) {
    for(int i=0;i<NUMPIXELS;i++){
      pixels.setPixelColor(i, pixels.Color(102,0,204)); 
    }
    pixels.show();
    delay (1000);
  }

  if (sender == 6) {
    for(int i=0;i<NUMPIXELS;i++){
      pixels.setPixelColor(i, pixels.Color(255,0,255)); 
    }
    pixels.show();
    delay (1000);
  }

}

void handleGlobalMessage(int sender, int value){
  
  for(int i=0;i<NUMPIXELS;i++){
    pixels.setPixelColor(i, pixels.Color(0,0,0)); 
  }
  pixels.show();
  globalMessage = value;  
  directMessageSender = sender; 
  Serial.print("Incoming global Message from: "); 
  Serial.print(sender); 
  Serial.print(" value: "); 
  Serial.println(value); 
  for(int i=0;i<round(value/9);i++){
    pixels.setPixelColor(i, pixels.Color(255,0,0)); 
  }
  for(int i=round(value/9);i<28;i++){
    pixels.setPixelColor(i, pixels.Color(0,0,0)); 
  }
  for(int i=28;i<(round(value/9) + 28);i++){
    pixels.setPixelColor(i, pixels.Color(255,0,0)); 
  }
  for(int i=(round(value/9) + 28);i<NUMPIXELS;i++){
    pixels.setPixelColor(i, pixels.Color(0,0,0)); 
  }
  pixels.show();
  delay (1000);
}

void handleDirectMessage(int sender, int value){
  for(int i=0;i<NUMPIXELS;i++){
    pixels.setPixelColor(i, pixels.Color(0,0,0)); 
  }
  pixels.show();

  directMessage = value;
  directMessageSender = sender; 
  Serial.print("Incoming direct Message from: "); 
  Serial.print(sender); 
  Serial.print(" value: "); 
  Serial.println(value); 

  if (sender == 0) {
    for(int i=0;i<round(value/9);i++){
      pixels.setPixelColor(i, pixels.Color(255,128,0)); 
    }
    for(int i=round(value/9);i<28;i++){
      pixels.setPixelColor(i, pixels.Color(0,0,0)); 
    }
    for(int i=28;i<(round(value/9) + 28);i++){
      pixels.setPixelColor(i, pixels.Color(255,128,0)); 
    }
    for(int i=(round(value/9) + 28);i<NUMPIXELS;i++){
      pixels.setPixelColor(i, pixels.Color(0,0,0)); 
    }
    pixels.show(); 
    delay (1000);
  }

  if (sender == 1) {
    for(int i=0;i<round(value/9);i++){
      pixels.setPixelColor(i, pixels.Color(255,255,0)); 
    }
    for(int i=round(value/9);i<28;i++){
      pixels.setPixelColor(i, pixels.Color(0,0,0)); 
    }
    for(int i=28;i<(round(value/9) + 28);i++){
      pixels.setPixelColor(i, pixels.Color(255,255,0)); 
    }
    for(int i=(round(value/9) + 28);i<NUMPIXELS;i++){
      pixels.setPixelColor(i, pixels.Color(0,0,0)); 
    }
    pixels.show();
    delay (1000);
  }

  if (sender == 3) {
    for(int i=0;i<round(value/9);i++){
      pixels.setPixelColor(i, pixels.Color(0,255,0)); 
    }
    for(int i=round(value/9);i<28;i++){
      pixels.setPixelColor(i, pixels.Color(0,0,0)); 
    }
    for(int i=28;i<(round(value/9) + 28);i++){
      pixels.setPixelColor(i, pixels.Color(0,255,0)); 
    }
    for(int i=(round(value/9) + 28);i<NUMPIXELS;i++){
      pixels.setPixelColor(i, pixels.Color(0,0,0)); 
    }
    pixels.show();
    delay (1000);
  }

  if (sender == 4) {
    for(int i=0;i<round(value/9);i++){
      pixels.setPixelColor(i, pixels.Color(0,0,255)); 
    }
    for(int i=round(value/9);i<28;i++){
      pixels.setPixelColor(i, pixels.Color(0,0,0)); 
    }
    for(int i=28;i<(round(value/9) + 28);i++){
      pixels.setPixelColor(i, pixels.Color(0,0,255)); 
    }
    for(int i=(round(value/9) + 28);i<NUMPIXELS;i++){
      pixels.setPixelColor(i, pixels.Color(0,0,0)); 
    }
    pixels.show();
    delay (1000);
  }

  if (sender == 5) {
    for(int i=0;i<round(value/9);i++){
      pixels.setPixelColor(i, pixels.Color(102,0,204)); 
    }
    for(int i=round(value/9);i<28;i++){
      pixels.setPixelColor(i, pixels.Color(0,0,0)); 
    }
    for(int i=28;i<(round(value/9) + 28);i++){
      pixels.setPixelColor(i, pixels.Color(102,0,204)); 
    }
    for(int i=(round(value/9) + 28);i<NUMPIXELS;i++){
      pixels.setPixelColor(i, pixels.Color(0,0,0)); 
    }
    pixels.show();
    delay (1000);
  }

  if (sender == 6) {
    for(int i=0;i<round(value/9);i++){
      pixels.setPixelColor(i, pixels.Color(255,0,255)); 
    }
    for(int i=round(value/9);i<28;i++){
      pixels.setPixelColor(i, pixels.Color(0,0,0)); 
    }
    for(int i=28;i<(round(value/9) + 28);i++){
      pixels.setPixelColor(i, pixels.Color(255,0,255)); 
    }
    for(int i=(round(value/9) + 28);i<NUMPIXELS;i++){
      pixels.setPixelColor(i, pixels.Color(0,0,0)); 
    }
    pixels.show();
    delay (1000);
  }
}

void setup(void) {
  pixels.begin();
  Serial.begin(9600);
  while (!Serial) delay(10);     

  Serial.println("LIS3DH test!");

  if (! lis.begin(0x18)) {   // change this to 0x19 for alternative i2c address
    Serial.println("Couldnt start");
    while (1) yield();
  }
  Serial.println("LIS3DH found!");

  // lis.setRange(LIS3DH_RANGE_4_G);   // 2, 4, 8 or 16 G!

  Serial.print("Range = "); Serial.print(2 << lis.getRange());
  Serial.println("G");

  // lis.setDataRate(LIS3DH_DATARATE_50_HZ);
  Serial.print("Data rate set to: ");
  switch (lis.getDataRate()) {
    case LIS3DH_DATARATE_1_HZ: Serial.println("1 Hz"); break;
    case LIS3DH_DATARATE_10_HZ: Serial.println("10 Hz"); break;
    case LIS3DH_DATARATE_25_HZ: Serial.println("25 Hz"); break;
    case LIS3DH_DATARATE_50_HZ: Serial.println("50 Hz"); break;
    case LIS3DH_DATARATE_100_HZ: Serial.println("100 Hz"); break;
    case LIS3DH_DATARATE_200_HZ: Serial.println("200 Hz"); break;
    case LIS3DH_DATARATE_400_HZ: Serial.println("400 Hz"); break;

    case LIS3DH_DATARATE_POWERDOWN: Serial.println("Powered Down"); break;
    case LIS3DH_DATARATE_LOWPOWER_5KHZ: Serial.println("5 Khz Low Power"); break;
    case LIS3DH_DATARATE_LOWPOWER_1K6HZ: Serial.println("16 Khz Low Power"); break;
  }

  //serial connection to computer
  //Serial.begin(9600);
    // delay(1000); // uncomment if your serial monitor is empty
    //handles serial connection to arduino via wire
    //NodeMcu_SoftSerial.begin(9600);

  WiFi.mode(WIFI_STA);
    
  if (esp_now_init() == 0) {
      Serial.println("ESPNow Init success");
  }
  else {
      Serial.println("ESPNow Init fail");
      return;
  }


  //esp_now_set_self_role(ESP_NOW_ROLE_COMBO);

  // check length of receiverAddress structure
  //iterate through all the possible addresses and add them as peers


  for (int i=0;i<sizeof(receiverAddress) / sizeof(receiverAddress[0]);i++) {
    memcpy(peerInfo[i].peer_addr, receiverAddress[i], 6);
      peerInfo[i].channel = 0;
      peerInfo[i].encrypt = false;
      if (esp_now_add_peer(&peerInfo[i]) != ESP_OK) {
          Serial.println("Failed to add peer");
          return;
      }
  }
    //call back functions for sending and receiving messages
  esp_now_register_send_cb(messageSent);  
  esp_now_register_recv_cb(messageReceived); 

}

void loop() {
 
  String arduino_recivedString;

  lis.read();      // get X Y and Z data at once
  // Then print out the raw data
  //Serial.print("X:  "); Serial.print(lis.x);
  //Serial.print("  \tY:  "); Serial.print(lis.y);
  //Serial.print("  \tZ:  "); Serial.print(lis.z);

  /* Or....get a new sensor event, normalized */
  sensors_event_t event;
  lis.getEvent(&event);

  
  /* Display the results (acceleration is measured in m/s^2) */
  Serial.print("\t\tXr:"); Serial.print(event.acceleration.x); Serial.print(" ");
  Serial.print(" \tYr:"); Serial.print(event.acceleration.y); Serial.print(" ");
  Serial.print(" \tZr:"); Serial.print(event.acceleration.z); Serial.print(" ");
  //Serial.println(" m/s^2 ");

  if ((event.acceleration.x < 0.0) or (event.acceleration.x > 1.3) or (event.acceleration.y < -0.2) or (event.acceleration.y > 1.1)){ 

    if (event.acceleration.x > 7.0){
      ping = 1;
      value = 255;
      for(int i=0;i<NUMPIXELS;i++){
          pixels.setPixelColor(i, pixels.Color(0,0,0)); 
      }
      for(int i=0;i<NUMPIXELS;i++){
        pixels.setPixelColor(i, pixels.Color(0,255,255)); // Turqoise
      }
      pixels.show();
      delay(3000);
    }
    else{
      ping = 0;
    }

    lis.getEvent(&event);

    if (event.acceleration.x < -5.0) {
      global = 1;   
      for(int i=0;i<NUMPIXELS;i++){
        pixels.setPixelColor(i, pixels.Color(0,0,0)); 
      }
      pixels.show();
      value = round (abs((event.acceleration.x - x) / 0.04));
      for(int i=0;i<round(value/9);i++){
        pixels.setPixelColor(i, pixels.Color(255,0,0)); // Red
      }
      for(int i=28;i<(round(value/9) + 28);i++){
        pixels.setPixelColor(i, pixels.Color(255,0,0)); // Red
      }
      pixels.show();
      delay(3000);
    }

    else {
      global = 0;
    }

    if ((event.acceleration.x < -1.0)  and (event.acceleration.y > 2.0)) {
      address = 0;
      for(int i=0;i<NUMPIXELS;i++){
        pixels.setPixelColor(i, pixels.Color(0,0,0)); 
      }
      pixels.show();
      value = round((abs(event.acceleration.x - x) + abs(event.acceleration.y - y)) / 0.04);
      for(int i=0;i<round(value/9);i++){
        pixels.setPixelColor(i, pixels.Color(255,128,0)); // Orange
      }
      for(int i=28;i<(round(value/9) + 28);i++){
        pixels.setPixelColor(i, pixels.Color(255,128,0)); // Orange
      }
      pixels.show();
      delay(3000);
    }

    else if (event.acceleration.y > 5.5) {
      address = 1;
      for(int i=0;i<NUMPIXELS;i++){
        pixels.setPixelColor(i, pixels.Color(0,0,0)); 
      }
      pixels.show();
      value = abs((event.acceleration.y - y) / 0.04);
      for(int i=0;i<round(value/9);i++){
        pixels.setPixelColor(i, pixels.Color(255,255,0)); // Yellow
      }
      for(int i=28;i<(round(value/9) + 28);i++){
        pixels.setPixelColor(i, pixels.Color(255,255,0)); // Yellow
      }
      pixels.show();
      delay(3000);
    }

    else if ((event.acceleration.x > 2.0) and (event.acceleration.y > 2.0)) {
      address = 3;  
      for(int i=0;i<NUMPIXELS;i++){
        pixels.setPixelColor(i, pixels.Color(0,0,0)); 
      }
      pixels.show();
      value = round ((abs(event.acceleration.x - x) + abs(event.acceleration.y - y)) / 0.04);
      for(int i=0;i<round(value/9);i++){
        pixels.setPixelColor(i, pixels.Color(0,255,0)); // Green
      }
      for(int i=28;i<(round(value/9) + 28);i++){
        pixels.setPixelColor(i, pixels.Color(0,255,0)); // Green
      }
      pixels.show();
      delay(3000);
    }

    else if ((event.acceleration.x > 2.0) and (event.acceleration.y < -2.0)) {
      address = 4;
      for(int i=0;i<NUMPIXELS;i++){
        pixels.setPixelColor(i, pixels.Color(0,0,0)); 
      }
      pixels.show();
      value = round((abs(event.acceleration.x - x) + abs(event.acceleration.y - y)) / 0.04);
      for(int i=0;i<round(value/9);i++){
        pixels.setPixelColor(i, pixels.Color(0,0,255)); // Blue
      }
      for(int i=28;i<(round(value/9) + 28);i++){
        pixels.setPixelColor(i, pixels.Color(0,0,255)); // Blue
      }
      pixels.show();
      delay(3000);
    }

    else if (event.acceleration.y < -5.0) {
      address = 5; 
      for(int i=0;i<NUMPIXELS;i++){
          pixels.setPixelColor(i, pixels.Color(0,0,0)); 
        }
        pixels.show();
      value = round(abs((event.acceleration.y - y) / 0.04));
      for(int i=0;i<round(value/9);i++){
        pixels.setPixelColor(i, pixels.Color(102,0,204)); // Purple
      }
      for(int i=28;i<(round(value/9) + 28);i++){
        pixels.setPixelColor(i, pixels.Color(102,0,204)); // Purple
      }
      pixels.show();
      delay(3000);
    }

    else if ((event.acceleration.x < -1.0) and (event.acceleration.y < -2.0)) {
      address = 6;
    
      for(int i=0;i<NUMPIXELS;i++){
        pixels.setPixelColor(i, pixels.Color(0,0,0)); 
      }
      pixels.show();
      value = round((abs(event.acceleration.x - x) + abs(event.acceleration.y - y)) / 0.04);
      for(int i=0;i<round(value/9);i++){
        pixels.setPixelColor(i, pixels.Color(255,0,255)); // Pink
      }
      for(int i=28;i<(round(value/9) + 28);i++){
        pixels.setPixelColor(i, pixels.Color(255,0,255)); // Pink
      }
      pixels.show();
      delay(3000);
    }
  
    arduino_recivedString += String(ping);
    arduino_recivedString += String(global);

      //adding address if necessary
    if(global){
      arduino_recivedString += "-";
    }else{
      arduino_recivedString += String(address);
    }

      //adding value if necessary
    if(ping){
      arduino_recivedString += "000";
    }else{
      arduino_recivedString += String(value / 100 % 10);
      arduino_recivedString += String(value / 10 % 10);
      arduino_recivedString += String(value % 10);
    }

      //adding my sender id
    arduino_recivedString += String(sender);

    
    Serial.println(arduino_recivedString);

    while (value > 0){
      lis.getEvent(&event);
      if (event.acceleration.z > 12.0) {
        Serial.println (event.acceleration.z);
        createAndSendMsgFromString(arduino_recivedString);
        for(int i=0;i<NUMPIXELS;i++){
          pixels.setPixelColor(i, pixels.Color(153,204,255)); 
        }
        pixels.show();
        delay(3000);
        value = 0;
      }
      else{
        value = value;
      }
    }


    

    String received;
    handleMessageFromNetwork(received);
    
    lights();
  }

  else {
    for(int i=0;i<NUMPIXELS;i++){
      pixels.setPixelColor(i, pixels.Color(0,0,0)); 
    }
    pixels.show();
    delay(500);
  }

  x = event.acceleration.x;
  y = event.acceleration.y;
  z = event.acceleration.z;
  address = 0;
  value = 0;
  Serial.println();

  delay(500);

  }
