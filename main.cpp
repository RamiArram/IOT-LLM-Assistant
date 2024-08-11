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

const char* ssid = "";
const char* password = "";
const char* chatgpt_token = "";
const char* temperature = "0";
const char* max_tokens = "45";
String Question = "";
String curr_convo;
String history;

#define I2S_DOUT      25
#define I2S_BCLK      27
#define I2S_LRC       26

Audio audio;

void setup()
{
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  while (!Serial);

  // wait for WiFi connection
  WiFi.begin(ssid, password);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(100);
}

String chatHistory[5] = {"","","","",""};
int start=0;

void loop()
{
  Serial.print("Ask your Question : ");
  while (!Serial.available())
  {
    audio.loop();
  }
  while (Serial.available())
  {
    char add = Serial.read();
    Question = Question + add;
    delay(1);
  }
  int len = Question.length();
  Question = Question.substring(0, (len - 1)); // Remove the last character (usually newline or carriage return)
  Question = "\"" + Question + "\"";
  Serial.println(Question);

  HTTPClient https;

  Serial.print("[HTTPS] begin...\n");
  if (https.begin("https://api.openai.com/v1/chat/completions")) {  // HTTPS

    https.addHeader("Content-Type", "application/json");
    String token_key = String("Bearer ") + chatgpt_token;
    https.addHeader("Authorization", token_key);

    // Add the new question to the conversation history    
    curr_convo = "{\"role\": \"user\", \"content\": " + Question + "} ";
    chatHistory[start%5] = curr_convo;
    Serial.println(chatHistory[0]);

    //append all history, and append the new question last, to format the string correctly for API
    history="";
    int index;
    history = "[";
    for (int i = 0; i < 5; i++) {
      int index = (start+i+1) % 5;
      if (chatHistory[index] != "") {
        history += chatHistory[index];
        if (i != 4) history += ",";
      }
    }

    Serial.println("history: " + history);

    if (history.endsWith(",")) {
      history.remove(history.length() - 1);
    }
    history += "]";


    // Remove the trailing comma and close the JSON array
    String trimmedHistory = history;
    trimmedHistory.remove(trimmedHistory.length() - 2); // Remove the last comma and space
    trimmedHistory += "]";

    // Prepare the payload with the entire conversation history
    String payload = "{\"model\": \"gpt-3.5-turbo\", \"messages\": " + trimmedHistory + String(", \"temperature\": ") + temperature + String(", \"max_tokens\": ") + max_tokens + String("}");

    Serial.print("Payload: ");
    Serial.println(payload);

    Serial.print("[HTTPS] POST...\n");
    int httpCode = https.POST(payload);

    if (httpCode < 0) {
      Serial.printf("[HTTPS] POST... failed, error: %s\n", https.errorToString(httpCode).c_str());
    } else {
      Serial.printf("[HTTPS] POST... response code: %d\n", httpCode);
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        String payload = https.getString();

        DynamicJsonDocument doc(2048);  // Increase size if necessary

        deserializeJson(doc, payload);
        String Answer = doc["choices"][0]["message"]["content"];
        Serial.print("Answer : "); Serial.println(Answer);

        // Add the response to the conversation history
        curr_convo += ", {\"role\": \"assistant\", \"content\": \"" + Answer + "\"} ";

        //update chatHistory
        chatHistory[start%5] = curr_convo;

        start++;

        // Play the response
        audio.connecttospeech(Answer.c_str(), "en");

      } else {
        Serial.printf("[HTTPS] POST... failed with HTTP code: %d\n", httpCode);
      }
    }
    https.end();
  } else {
    Serial.printf("[HTTPS] Unable to connect\n");
  }

  Question = "";
}
