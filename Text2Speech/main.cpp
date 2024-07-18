#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <Wire.h>
#include <SD.h>
#include <SD_MMC.h>
#include <SPIFFS.h>
#include <FFat.h>
#include <ArduinoJson.h>
#include "Audio.h"

HardwareSerial MySerial(1);

const char* ssid = "WIFI";
const char* password = "PASS";
const char* chatgpt_token = "API";
const char* temperature = "0";
const char* max_tokens = "45";
String Question = "";

#define I2S_DOUT 25
#define I2S_BCLK 27
#define I2S_LRC 26

Audio audio;

void setup() {
  Serial.begin(115200);
  MySerial.begin(115200, SERIAL_8N1, 16, 17);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("Connected to WiFi");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(100);
}

void loop() {
  if (MySerial.available()) {
    Question = MySerial.readStringUntil('\n');
    Question.trim();  
    Serial.println("Received Question: " + Question);

    
    Question = "\"" + Question + "\"";
    HTTPClient https;

    Serial.println("[HTTPS] begin...");
    if (https.begin("https://api.openai.com/v1/chat/completions")) {  // HTTPS
      https.addHeader("Content-Type", "application/json");
      String token_key = String("Bearer ") + chatgpt_token;
      https.addHeader("Authorization", token_key);

      String payload = String("{\"model\": \"gpt-3.5-turbo\", \"messages\": [{\"role\": \"user\", \"content\": ") + Question + String("}], \"temperature\": ") + temperature + String(", \"max_tokens\": ") + max_tokens + String("}");

      Serial.println("[HTTPS] POST...");
      Serial.print("Payload: ");
      Serial.println(payload);

      int httpCode = https.POST(payload);

      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        String response = https.getString();

        DynamicJsonDocument doc(2048);  // Increase buffer size if needed
        deserializeJson(doc, response);

        String Answer = doc["choices"][0]["message"]["content"];
        Serial.print("Answer: ");
        Serial.println(Answer);

        audio.connecttospeech(Answer.c_str(), "en");
      } else {
        Serial.printf("[HTTPS] POST... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }

      https.end();
    } else {
      Serial.println("[HTTPS] Unable to connect");
    }

    Question = "";  // Reset the question after processing
  }

  audio.loop();
}

///void audio_info(const char *info) {
 // Serial.print("audio_info: ");
 // Serial.println(info);
//}
