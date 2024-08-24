#ifndef _CLOUDSPEECHCLIENT_H
#define _CLOUDSPEECHCLIENT_H
#include <WiFiClientSecure.h>
#include "I2S.h"

enum Authentication {
  USE_ACCESSTOKEN,
  USE_APIKEY
};

class CloudSpeechClient {
  WiFiClientSecure client;
  bool PrintHttpBody2();
  Authentication authentication;
  I2S* i2s;
  static const int headerSize = 44;
  static const int i2sBufferSize = 12000;
  char i2sBuffer[i2sBufferSize];
  void CreateWavHeader(byte* header, int waveDataSize);

public:
  CloudSpeechClient(Authentication authentication);
  ~CloudSpeechClient();
  String Transcribe();
  void preTranscribe();
  static const int segmentValue = 8; // how many iterations
  static const int wavDataSize = 90000;                   // It must be multiple of dividedWavDataSize. Recording time is about 1.9 second.
  static const int dividedWavDataSize = i2sBufferSize/4;
  char** wavData;                                         // It's divided. Because large continuous memory area can't be allocated in esp32.
  byte paddedHeader[headerSize + 4] = {0};  
   
};

#endif // _CLOUDSPEECHCLIENT_H
