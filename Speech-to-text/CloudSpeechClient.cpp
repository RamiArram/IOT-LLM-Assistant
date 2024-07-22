#include "CloudSpeechClient.h"
#include "network_param.h"
#include <base64.h>
#include <ArduinoJson.h>
#include "WiFi.h"
#include <HTTPClient.h>
#define USE_SERIAL Serial
#include <Arduino.h>
const char* chatgpt_token = "Your_ChatGPT_Token";
CloudSpeechClient::CloudSpeechClient(Authentication authentication) 
{
  this->authentication = authentication;
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(1000);
  client.setCACert(root_ca); // Set your root CA certificate here
  client.setInsecure();      // Accept any SSL certificate, including self-signed

  if (!client.connect(server, 443)) Serial.println("Connection failed!");
}

String ans;

CloudSpeechClient::~CloudSpeechClient() {
  client.stop();
  WiFi.disconnect();
}

void CloudSpeechClient::PrintHttpBody2(Audio* audio)
{
  String enc = base64::encode(audio->paddedHeader, sizeof(audio->paddedHeader));
  enc.replace("\n", "");  // delete last "\n"
  client.print(enc);      // HttpBody2
  char** wavData = audio->wavData;
  for (int j = 0; j < audio->wavDataSize / audio->dividedWavDataSize; ++j) {
    enc = base64::encode((byte*)wavData[j], audio->dividedWavDataSize);
    enc.replace("\n", "");// delete last "\n"
    client.print(enc);    // HttpBody2
  }
}

String CloudSpeechClient::Transcribe(Audio* audio) {
  String HttpBody1 = "{\"config\":{\"encoding\":\"LINEAR16\",\"sampleRateHertz\":16000,\"languageCode\":\"en-US\"},\"audio\":{\"content\":\"";
  String HttpBody3 = "\"}}\r\n\r\n";
  int httpBody2Length = (audio->wavDataSize + sizeof(audio->paddedHeader)) * 4 / 3; // 4/3 is from base64 encoding
  String ContentLength = String(HttpBody1.length() + httpBody2Length + HttpBody3.length());
  String HttpHeader;

  HttpHeader = String("POST /v1/speech:recognize?key=") + ApiKey
               + String(" HTTP/1.1\r\nHost: speech.googleapis.com\r\nContent-Type: application/json\r\nContent-Length: ") + ContentLength + String("\r\n\r\n");

  client.print(HttpHeader);
  client.print(HttpBody1);
  PrintHttpBody2(audio);
  client.print(HttpBody3);

  String My_Answer = "";
  while (!client.available());

  while (client.available()) {
    char temp = client.read();
    My_Answer = My_Answer + temp;
  }

  Serial.print("My Answer - "); Serial.println(My_Answer);

  int position = My_Answer.indexOf('{');
  String jsonData = My_Answer.substring(position);
  Serial.print("Json data--"); Serial.println(jsonData);

  
  const size_t capacity = 1024; // Adjust capacity as needed
  DynamicJsonDocument doc(capacity);

  
  DeserializationError error = deserializeJson(doc, jsonData);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return String("");
  }

  const char* transcript = doc["results"][0]["alternatives"][0]["transcript"];

  if (transcript) {
    Serial.print("Transcript: ");
    Serial.println(transcript);
    return String(transcript);
  } else {
    Serial.println("Transcript not found");
    return String("");
  }
}



///////////////////////////////////////////////////////////
/*

   
  */