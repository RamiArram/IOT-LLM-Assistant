
#include "CloudSpeechClient.h"
#include "WiFi.h"
#include "HardwareSerial.h"
#define RXp2 16
#define TXp2 17
#define led_1 15
#define led_2 2
#define button 23 //IR Sensor

#define led_3 4

bool new_loop = true;

HardwareSerial MySerial(1);
void setup() {
  pinMode(button, INPUT);
  pinMode(led_1,OUTPUT);
  pinMode(led_2,OUTPUT);
  pinMode(led_3,OUTPUT);
  Serial.begin(115200);
  MySerial.begin(115200, SERIAL_8N1, RXp2,TXp2);
  

}

void loop() {
  digitalWrite(led_1, 0);
  digitalWrite(led_2, 0);
  digitalWrite(led_3, 0);
  if(new_loop==true){
    Serial.println("Press button");
    new_loop=false;
    
  }
  //MOTION DETECTED
  if(digitalRead(button)==LOW){
    CloudSpeechClient* cloudSpeechClient = new CloudSpeechClient(USE_APIKEY);
    Serial2.println("\r\nPlease Ask!\r\n");
    digitalWrite(led_1, 1);
    digitalWrite(led_2, 0);
    digitalWrite(led_3, 0);
    delay(1000);
    
    Serial.println("\r\nRecord start!\r\n");
    //Audio* audio = new Audio(M5STACKFIRE);
    String answer = cloudSpeechClient->Transcribe();
    Serial.println("Recording Completed. Now Processing...");
    digitalWrite(led_1,0);
    digitalWrite(led_3,0);
    digitalWrite(led_2,1);
    
    Serial.println(answer);
    MySerial.println(answer);
    delete cloudSpeechClient;
  
    new_loop=true;
  }

  //NO MOTION DETECTED
  if(digitalRead(button)==HIGH){
    delay(100);
  }
}
