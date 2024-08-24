
#include "CloudSpeechClient.h"
#include "WiFi.h"
#include "HardwareSerial.h"
#define RXp2 16
#define TXp2 17
#define led_1 15
#define led_2 2
#define button 23 //IR Sensor

#define led_3 4
bool recording = false;
bool do_nothing = false;
bool new_loop = true;
unsigned long startTime1 = 0;
unsigned long timeout1 = 0;
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
  
  if(new_loop){
    recording = false;
    do_nothing = false;
    new_loop = false;
     
    Serial.print("Hold sensor and wait for the blue light to speak");
  }
  //MOTION DETECTED
  if(digitalRead(button)==LOW ){
    


// Wait for the client to be available or until the timeout

    if(!recording && !do_nothing){
     CloudSpeechClient* cloudSpeechClient = new CloudSpeechClient(USE_APIKEY);
    // add checking.
    recording = true;
     
    Serial.println("\r\nRecord start!\r\n");
   
    String answer = cloudSpeechClient->Transcribe();
    Serial.println("Recording Completed. Now Processing...");
    // checking audio length first.
    
    MySerial.println(answer);
    delete cloudSpeechClient;
    }
    
    // check other edge cases.
    do_nothing = true;
    new_loop = true;
   
  }

  //NO MOTION DETECTED
  if(digitalRead(button)==HIGH){
    
  delay(500);
}
}