#include <SPI.h>
#include "RF24.h"
#include "RF24SN.h"
#include <Wire.h>
#include <Adafruit_BMP085.h>
#include <EEPROM.h>
#include <RBD_Timer.h>

#define DEF_INTERVAL 10 //publish interval
#define EPROM_WAITINTADDRESS 0 //eeprom address

//RF24SN settings
#define RF24SN_NODE_ID 2 //NRF24 module nom
#define RF24SN_SERVER_ADDR 0xF0F0F0F000LL //NRF24 srv.address /raspberry-pi/ 

//sesors address
#define VOLTAGE_SENSOR   1   //pub."battery"
#define TEMP_SENSOR      2   //pub."temp"
#define PRESS_SENSOR     3   //pub."presure"
#define ALT_SENSOR       4   //pub."altitude"
#define PRESSSEA_SENSOR  5   //pub."presure sea level"
#define CFG_INTERVAL     6   //request for settings 
#define REQ_INTERVAL     100 //нов интервал рекуест

//радио модул - пинове 
#define RF24_CE_PIN 9
#define RF24_CS_PIN 10

//timer
RBD::Timer timer;

//init radio
RF24 radio(RF24_CE_PIN, RF24_CS_PIN);
RF24SN rf24sn( &radio, RF24SN_SERVER_ADDR, RF24SN_NODE_ID);

//init sensors
Adafruit_BMP085 bmp;

//publish interval 
long  pub_interval = DEF_INTERVAL ; //seconds

long publishTimer, requestTimer; 

void setup() {   
  //com port init
  Serial.begin(9600); 
  
  //init sensors
  if (!bmp.begin()) {
    Serial.println("Could not find a valid BMP085 sensor, check wiring!");
  while (1) {}
  }
  
  //init radio
  rf24sn.begin(); 
  pub_interval = DEF_INTERVAL; 
  //read settings
  int value = EEPROM.read(EPROM_WAITINTADDRESS);
  if(value >= DEF_INTERVAL ){
      pub_interval = value; 
      Serial.println("EPROM interval:");
      Serial.println(value );
    }
 
  //инит на таймера
  timer.setTimeout(pub_interval * 1000); //convert to mils
  timer.restart();
}

void loop() { 
  //publish values by timer
  if(timer.onRestart()) { 
   //power on NRF
    radio.powerUp();
    delay(50);
    //and publish  
    publish();
     
    //request for new settings
    check_for_new_interval();  
    //off NRF
    radio.powerDown();
    }  

}


//publish sesor values
void publish() { 
  Serial.println("Publishing values ");  
  bool publishSuccess = rf24sn.publish(TEMP_SENSOR, bmp.readTemperature());
  if (!publishSuccess) { Serial.println("Publish failed"); }
   publishSuccess = rf24sn.publish(PRESSSEA_SENSOR, bmp.readSealevelPressure(554) );
  if (!publishSuccess) { Serial.println("Publish failed");}

   publishSuccess = rf24sn.publish(ALT_SENSOR, bmp.readAltitude());
  if (!publishSuccess) { Serial.println("Publish failed"); }
  int sensorValue = analogRead(A0);
  float voltage = sensorValue * (5.0 / 1023.0)*2; //10kOM
 
  publishSuccess = rf24sn.publish(VOLTAGE_SENSOR, voltage);
  if (!publishSuccess) {   Serial.println("Publish failed");  }  

   publishSuccess = rf24sn.publish(PRESS_SENSOR, bmp.readPressure());
  if (!publishSuccess) { Serial.println("Publish failed");}      

   publishSuccess = rf24sn.publish( CFG_INTERVAL, pub_interval );
   if (!publishSuccess) { Serial.println("Publish failed");} 
}

//set new interval and write it to eeprom
void set_publish_interval(int new_interval){  
    if (new_interval>0 && new_interval>=DEF_INTERVAL && new_interval!=pub_interval){
        EEPROM.put(EPROM_WAITINTADDRESS, new_interval);
        pub_interval = new_interval;  
        Serial.print("New publish interval: ");
        Serial.println(new_interval);
        //инит на таймера
        timer.setTimeout(pub_interval * 1000); //преобразувам в милисекунди
        timer.restart();        
  }
}

//request for new settings
void check_for_new_interval(){
    float q=0;
    Serial.println("Requesting settings values");  
    bool requestSuccess = rf24sn.request(REQ_INTERVAL, &q); 
    if (!requestSuccess) { Serial.println("Request failed"); }
    if(requestSuccess && q>0){
      set_publish_interval( q );
    }
}
