#include "esp32-hal-gpio.h"
#include "CloudSpeechClient.h"
#include <base64.h>
#include <ArduinoJson.h>
#include "WiFi.h"
#include <HTTPClient.h>
#include "network_param.h"
#define USE_SERIAL Serial
#include <Arduino.h>
#define button 23 
#define led_3 4
#define led_1 15
#define led_2 2
const char* chatgpt_token = "Your_ChatGPT_Token";
CloudSpeechClient::CloudSpeechClient(Authentication authentication, const char* ssid, const char* password) 
{
  this->authentication = authentication;
  wavData = new char*[wavDataSize/dividedWavDataSize];
  for (int i = 0; i < wavDataSize/dividedWavDataSize; ++i) wavData[i] = new char[dividedWavDataSize];
  i2s = new I2S(ICS43434);
  WiFi.begin(ssid, password);
  Serial.println("Connecting to wifi");
  while (WiFi.status() != WL_CONNECTED){ 
    Serial.println(".");
    delay(1000);
  }
  Serial.print("Connected to wifi");
  
  client.setCACert(root_ca); // Set your root CA certificate here
  client.setInsecure();      // Accept any SSL certificate, including self-signed

  if (!client.connect(server, 443)) Serial.println("Connection failed!");
}

String ans;

CloudSpeechClient::~CloudSpeechClient() {
  client.stop();
  for (int i = 0; i < wavDataSize/dividedWavDataSize; ++i) delete[] wavData[i];
  delete[] wavData;
  delete i2s;
  
}

bool CloudSpeechClient::PrintHttpBody2()
{
  bool neverAgain = false;
  int curr =0;
  for(int segment = 0; segment<segmentValue;segment++){
     for (int j = 0; j < wavDataSize/dividedWavDataSize; ++j) {
      if(digitalRead(button) == LOW && !neverAgain ){
      i2s->Read(i2sBuffer, i2sBufferSize);
      for (int i = 0; i < i2sBufferSize/8; ++i) {
        wavData[j][2*i] = i2sBuffer[8*i + 2];
        wavData[j][2*i + 1] = i2sBuffer[8*i + 3];
      }
    
      String enc = base64::encode((byte*)wavData[j], dividedWavDataSize);
      enc.replace("\n", "");// delete last "\n"
      client.print(enc);    
    }
    else if(digitalRead(button) == HIGH || neverAgain){
        if(segment ==0 && j< 15){
           Serial.print("recording is too short");
           return false;
        }
        if(!neverAgain){
        curr = j;
        memset(wavData[j], 0, dividedWavDataSize);
        neverAgain = true;
        digitalWrite(led_1,0);
        digitalWrite(led_3,0);
        digitalWrite(led_2,1);
        }
        String enc = base64::encode((byte*)wavData[curr], dividedWavDataSize);
        enc.replace("\n", "");// delete last "\n"
        client.print(enc);
    }
    }
    
}
return true;
}
  
  
  

void CloudSpeechClient::preTranscribe(){
  String HttpBody1 = "{\"config\":{\"encoding\":\"LINEAR16\",\"sampleRateHertz\":16000,\"languageCode\":\"en-US\"},\"audio\":{\"content\":\"";
  String HttpBody3 = "\"}}\r\n\r\n";
  int httpBody2Length = ((wavDataSize*segmentValue) + sizeof(paddedHeader)) * 4 / 3; // 4/3 is from base64 encoding
  String ContentLength = String(HttpBody1.length() + httpBody2Length + HttpBody3.length());
  String HttpHeader;

  HttpHeader = String("POST /v1/speech:recognize?key=") + ApiKey
               + String(" HTTP/1.1\r\nHost: speech.googleapis.com\r\nContent-Type: application/json\r\nContent-Length: ") + ContentLength + String("\r\n\r\n");

  client.print(HttpHeader);
  client.print(HttpBody1);
  CreateWavHeader(paddedHeader,wavDataSize*segmentValue);
  String enc = base64::encode(paddedHeader, 48);
  enc.replace("\n", "");
  client.print(enc);
}
String CloudSpeechClient::Transcribe() {

  CloudSpeechClient::preTranscribe();
  //recording time :
    digitalWrite(led_1, 1);
    digitalWrite(led_2, 0);
    digitalWrite(led_3, 0);
   bool state = PrintHttpBody2();
   if(!state){
    return("recording size is too small");
   }
  digitalWrite(led_1,0);
        digitalWrite(led_3,0);
        digitalWrite(led_2,1);
  Serial.print("finished printing body");
  // end of recording.
    String HttpBody3 = "\"}}\r\n\r\n";
  client.print(HttpBody3);

  String My_Answer = "";
  unsigned long startTime = millis();
unsigned long timeout = 10000; // 15 seconds

// Wait for the client to be available or until the timeout
while (!client.available()) {
    if (millis() - startTime > timeout) {
        Serial.println("Timeout waiting for the client to respond");
        return "Timeout waiting for the client to respond"; // or handle timeout as needed
    }
}

// Reset the start time to measure the timeout for the reading loop


// Read the response from the client
while (client.available()) {
    char temp = client.read();
    My_Answer = My_Answer + temp;
    
    // Break the loop if the timeout occurs
    
}

  Serial.print("My Answer - "); Serial.println(My_Answer);

int position = My_Answer.indexOf('{');
String jsonData = My_Answer.substring(position);
Serial.print("Json data--"); Serial.println(jsonData);

const size_t capacity = 4000; // Adjust capacity as needed
DynamicJsonDocument doc(capacity);

DeserializationError error = deserializeJson(doc, jsonData);
if (error) {
  Serial.print(F("deserializeJson() failed: "));
  Serial.println(error.f_str());
  return String("Low confidence level, please record again.");
}

String fullTranscript = "";
bool lowConfidence = false;

// Iterate over the results array and concatenate all transcripts
for (JsonObject result : doc["results"].as<JsonArray>()) {
  const char* transcript = result["alternatives"][0]["transcript"];
  float confidence = result["alternatives"][0]["confidence"];

  if (confidence < 0.75) {
    lowConfidence = true;
  }

  if (transcript) {
    fullTranscript += String(transcript);
    fullTranscript += " "; // Add a space between each transcript
  }
}

if (lowConfidence) {
  Serial.println("Low confidence level, please record again.");
  return String("Low confidence level, please record again.");
} else if (fullTranscript.length() > 0) {
  Serial.print("Full Transcript: ");
  Serial.println(fullTranscript);
  return fullTranscript;
} else {
  Serial.println("No transcripts found");
  return String("No transcripts found");
}
}
void CloudSpeechClient::CreateWavHeader(byte* header, int waveDataSize){
  header[0] = 'R';
  header[1] = 'I';
  header[2] = 'F';
  header[3] = 'F';
  unsigned int fileSizeMinus8 = waveDataSize + 44 - 8;
  header[4] = (byte)(fileSizeMinus8 & 0xFF);
  header[5] = (byte)((fileSizeMinus8 >> 8) & 0xFF);
  header[6] = (byte)((fileSizeMinus8 >> 16) & 0xFF);
  header[7] = (byte)((fileSizeMinus8 >> 24) & 0xFF);
  header[8] = 'W';
  header[9] = 'A';
  header[10] = 'V';
  header[11] = 'E';
  header[12] = 'f';
  header[13] = 'm';
  header[14] = 't';
  header[15] = ' ';
  header[16] = 0x10;  // linear PCM
  header[17] = 0x00;
  header[18] = 0x00;
  header[19] = 0x00;
  header[20] = 0x01;  // linear PCM
  header[21] = 0x00;
  header[22] = 0x01;  // monoral
  header[23] = 0x00;
  header[24] = 0x80;  // sampling rate 16000
  header[25] = 0x3E;
  header[26] = 0x00;
  header[27] = 0x00;
  header[28] = 0x00;  // Byte/sec = 16000x2x1 = 32000
  header[29] = 0x7D;
  header[30] = 0x00;
  header[31] = 0x00;
  header[32] = 0x02;  // 16bit monoral
  header[33] = 0x00;
  header[34] = 0x10;  // 16bit
  header[35] = 0x00;
  header[36] = 'd';
  header[37] = 'a';
  header[38] = 't';
  header[39] = 'a';
  header[40] = (byte)(waveDataSize & 0xFF);
  header[41] = (byte)((waveDataSize >> 8) & 0xFF);
  header[42] = (byte)((waveDataSize >> 16) & 0xFF);
  header[43] = (byte)((waveDataSize >> 24) & 0xFF);
}



///////////////////////////////////////////////////////////
/*

   
  */