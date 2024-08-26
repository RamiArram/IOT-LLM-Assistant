#include "CloudSpeechClient.h"
#include "WiFi.h"
#include "HardwareSerial.h"
#include <WiFiManager.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

#define RXp2 16
#define TXp2 17
#define led_1 15
#define led_2 2
#define button 23 //IR Sensor

#define led_3 4
#define SDA 27
#define SCL 25
#define i2c_Address 0x3c // Use the I2C address 0x3C
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1   // Not connected to a reset pin

Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Uses external vars in the network_param.
const char* ssid1;
const char* password1;
WiFiManager wm;

String wifiname = "";
String wifipass = "";
bool once = true;
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

void displayMonitor(String s){
    display.clearDisplay(); // Clear the display buffer
    display.setTextSize(1); // Set text size
    display.setTextColor(SH110X_WHITE); // Set text color
    display.setCursor(0, 0); // Set cursor position
    display.println(s); // Print text to the display
    display.display();
}

void setup() {
  pinMode(button, INPUT);
  pinMode(led_1, OUTPUT);
  pinMode(led_2, OUTPUT);
  pinMode(led_3, OUTPUT);
  Serial.begin(115200);
  MySerial.begin(115200, SERIAL_8N1, RXp2, TXp2);
  
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  Wire.begin(SDA, SCL);
  display.begin(i2c_Address, true); // Initialize the display with the I2C address
  display.clearDisplay(); // Clear the display buffer

  display.setTextSize(1); // Set text size
  display.setTextColor(SH110X_WHITE); // Set text color
  display.setCursor(0, 0); // Set cursor position
  display.println("Waiting to wifi credentials"); // Print text to the display
  display.display(); // Display the text on the screen
  digitalWrite(led_3, HIGH);
  while (!Serial);

  wm.resetSettings();

  // Start blinking LED
  

 
 
   bool res = wm.autoConnect("LLM Assistant" , "Password");

  if(!res){
    display.println("Failed to connect");
    displayMonitor("Failed to connect, restarting esp");
    ESP.restart();
  }
  
  

  // Stop blinking LED and turn it on solid once connected
  digitalWrite(led_1, HIGH);
 displayMonitor("Connected to Wifi");
  String ssid_s = wm.getWiFiSSID();
  String password_s = wm.getWiFiPass();

  password1 = password_s.c_str();

  String temp = String(password1);

  temp = cleanString(temp);

  wifipass = temp;
  wifiname = ssid_s;

  password1 = temp.c_str();

  ssid1 = ssid_s.c_str();
  password1 = password_s.c_str();

  Serial.print("password");
  Serial.print(password1);
  
  Serial.print("WIFI " + ssid_s + "PASS " + temp);

  Serial.println("connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  String wifi_creds = createWifiPasswordString(ssid1, password1);

  Serial.println("WIFI CREDS " + wifi_creds);
  MySerial.println(wifi_creds);
  
  while (!MySerial.available()) {
    display.println("Waiting for connection on the second ESP32");
  }
}

void loop() {
  digitalWrite(led_1, 0);
  digitalWrite(led_2, 0);
  digitalWrite(led_3, 0);
  if (once) {
    String hello = "$Hello I'm Aura, Your LLM Assistant";
    MySerial.println(hello);
    once = false;
  }
  if (new_loop) {
    recording = false;
    do_nothing = false;
    new_loop = false;

  
    Serial.print("Hold sensor and wait for the blue light to speak");
    displayMonitor("Ask me anything<3");
     if (WiFi.status() != WL_CONNECTED){ 
     displayMonitor("Connection to Wifi lost, Please try connecting again");
     ESP.restart();
      
  }
  }

  // MOTION DETECTED
  if (digitalRead(button) == LOW) {
    String ssid_s = wm.getWiFiSSID();
    String password_s = wm.getWiFiPass();

    password1 = password_s.c_str();

    String temp = String(password1);

    temp = cleanString(temp);

    wifipass = temp;
    wifiname = ssid_s;

    Serial.print("WIFI " + String(ssid1) + "PASS + " + String(password1));
    Serial.print("WIFI " + wifiname + "PASS " + wifipass);

    if (!recording && !do_nothing ) {
       
      CloudSpeechClient* cloudSpeechClient = new CloudSpeechClient(USE_APIKEY, wifiname.c_str(), wifipass.c_str());
      recording = true;
    
      Serial.println("\r\nRecord start!\r\n");
    
      String answer = cloudSpeechClient->Transcribe();
    
      Serial.println("Recording Completed. Now Processing...");
      String answerCut = answer;
      if (answer[0] == '$') {
        String error_message = answerCut.substring(1);
        displayMonitor(error_message);
      }
      Serial.println(answer);
      MySerial.println(answer);
     
      
      delete cloudSpeechClient;
      if (WiFi.status() != WL_CONNECTED){ 
     displayMonitor("Connection to Wifi lost, Please try connecting again");
     ESP.restart();
      }
      
      do_nothing = true;
      new_loop = true;
      
    }
  }

  // NO MOTION DETECTED
  if (digitalRead(button) == HIGH) {
    delay(500);
    
    
  }
}
