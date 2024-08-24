
#include "CloudSpeechClient.h"
#include "WiFi.h"
#include "HardwareSerial.h"
#include <WiFiManager.h>

#define RXp2 16
#define TXp2 17
#define led_1 15
#define led_2 2
#define button 23 //IR Sensor

#define led_3 4

//Uses external vars in the network_param.
const char* ssid;
const char* password;
WiFiManager wm;

String wifiname = "";
String wifipass = "";

bool recording = false;
bool do_nothing = false;
bool new_loop = true;
unsigned long startTime1 = 0;
unsigned long timeout1 = 0;
HardwareSerial MySerial(1);

String createWifiPasswordString(const char* wifiName, const char* password) {
    // Create the formatted string
    String result = "WIFI: ";
    result += wifiName;
    result += "PASSWORD: ";
    result += password;
    return result;
}

// Function to remove unwanted characters from a const char* string
String cleanString(const String& input) {
    String result;
    for (unsigned int i = 0; i < input.length(); ++i) {
        char c = input[i];
        if (!isSpace(c) && c != '\0') {
            result += c;
        }
    }
    return result;
}


void setup() {
  pinMode(button, INPUT);
  pinMode(led_1,OUTPUT);
  pinMode(led_2,OUTPUT);
  pinMode(led_3,OUTPUT);
  Serial.begin(115200);
  MySerial.begin(115200, SERIAL_8N1, RXp2,TXp2);
  
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  while (!Serial);

  // wait for WiFi connection
  //WiFi.begin(ssid, password);


  wm.resetSettings();

  bool res;

  res = wm.autoConnect("LLM Assistant" , "Password");

  if(!res){
    Serial.println("Failed to connect");
  }

  String ssid_s = wm.getWiFiSSID();
  String password_s = wm.getWiFiPass();

  password = password_s.c_str();

  String temp = String(password);

  temp = cleanString(temp);

  wifipass = temp;
  wifiname = ssid_s;

  password = temp.c_str();

  ssid = ssid_s.c_str();
  password = password_s.c_str();

  Serial.print("password");

  Serial.print(password);
  
  Serial.print("WIFI " + ssid_s + "PASS " + temp);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  String wifi_creds = createWifiPasswordString(ssid,password);

  Serial.println("WIFI CREDS " + wifi_creds);
  MySerial.println(wifi_creds);
  
  while (!MySerial.available())
  {
    Serial.println("Waiting for connection on the second ESP32");
  }

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
  if(digitalRead(button)==LOW){
    String ssid_s = wm.getWiFiSSID();
    String password_s = wm.getWiFiPass();

    password = password_s.c_str();

    String temp = String(password);

    temp = cleanString(temp);

    wifipass = temp;
    wifiname = ssid_s;

    Serial.print("WIFI " + String(ssid) + "PAS S + " + String(password));
    Serial.print("WIFI " + wifiname + "PASS " + wifipass);
    if(!recording && !do_nothing){
     CloudSpeechClient* cloudSpeechClient = new CloudSpeechClient(USE_APIKEY,wifiname.c_str(),wifipass.c_str());
    // add checking.
    recording = true;
     
    delay(1000);
    
    Serial.println("\r\nRecord start!\r\n");
    //Audio* audio = new Audio(M5STACKFIRE);
    String answer = cloudSpeechClient->Transcribe();
    Serial.println("Recording Completed. Now Processing...");

    Serial.println(answer);
    MySerial.println(answer);
    delete cloudSpeechClient;
  
    //Waits for the speaker to finish talking
    while(!MySerial.available()){
      delay(1);
    }
    // check other edge cases.
    do_nothing = true;
    new_loop = true;
    }
  }

  //NO MOTION DETECTED
  if(digitalRead(button)==HIGH){
    
  delay(500);
}
}
