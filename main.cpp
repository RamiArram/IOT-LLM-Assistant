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
#define N 2
#define TAUB 0
#define ALGO 1

String rag_data[N] = {"TAUBBB", "ALGOOOOO"};
bool rag_data_included[N] = {false};

void reset_rag(){
    for (int i = 0; i < N; ++i) {
        rag_data_included[i] = false;
    }
}

void toLowerCase(String& str) {
    for (int i=0;i<str.length();i++) {
        if (str[i] >= 'A' && str[i] <= 'Z') {
            str[i] = str[i] + ('a' - 'A');
        }
    }
}

String keyword_context(String word){
    if(word == ""){
        return "";
    }
    int index = -1;
    toLowerCase(word);
    if(word == "taub" ){
        index = TAUB;
    }
    else if((word == "algo" || word == "algorithms")){
        index = ALGO;
    }
    if(index != -1 && !rag_data_included[index]){
        rag_data_included[index] = true;
        return rag_data[index];
    }
    return "";
}

String find_context(String question){
    reset_rag();
    String context;
    String word;
    for(int i=0;i<question.length();i++){
        if(question[i] == ' ' || question[i] == '.' || question[i] == ',' || question[i] == '"'){
            context += keyword_context(word);
            word = "";
        } else{
            word += question[i];
            if(i == question.length()-1){
                context += keyword_context(word);
            }
        }
    }
    return context;
}

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
int start = 0;

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
  Serial.println("Question: " + Question);

  HTTPClient https;

  Serial.print("[HTTPS] begin...\n");
  if (https.begin("https://api.openai.com/v1/chat/completions")) {  // HTTPS

    https.addHeader("Content-Type", "application/json");
    String token_key = String("Bearer ") + chatgpt_token;
    https.addHeader("Authorization", token_key);

    // Add the new question to the conversation history    
    curr_convo = "{\"role\": \"user\", \"content\": \"" + Question + "\"} ";
    chatHistory[start % 5] = curr_convo;

    // Append all history to send to chatgpt and appends question last
    String context = find_context(Question);
    Serial.println("Context: " + context);

    history = "[{\"role\": \"system\", \"content\": \"" + context + "\"}, ";  // System message first
    for (int i = 0; i < 5; i++) {
      int index = (start + i + 1) % 5;
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

    // Prepare the payload with the entire conversation history
    String payload = "{\"model\": \"gpt-3.5-turbo\", \"messages\": " + history + String(", \"temperature\": ") + temperature + String(", \"max_tokens\": ") + max_tokens + "}";

    Serial.print("Payload: ");
    Serial.println(payload);

    Serial.print("[HTTPS] POST...\n");
    int httpCode = https.POST(payload);

    if (httpCode < 0) {
      Serial.printf("[HTTPS] POST... failed, error: %s\n", https.errorToString(httpCode).c_str());
    } else {
      Serial.printf("[HTTPS] POST... response code: %d\n", httpCode);
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        String response = https.getString();

        DynamicJsonDocument doc(2048);

        deserializeJson(doc, response);
        String Answer = doc["choices"][0]["message"]["content"];
        Serial.print("Answer: ");
        Serial.println(Answer);

        // Add the response to the conversation history
        curr_convo = "{\"role\": \"assistant\", \"content\": \"" + Answer + "\"}";
        start++;

        chatHistory[start % 5] = curr_convo;

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
