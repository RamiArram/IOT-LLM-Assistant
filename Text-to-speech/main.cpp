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
#include <WiFiManager.h>

HardwareSerial MySerial(1);
HardwareSerial MySerial2(2);


const char* ssid = "";
const char* password = "";
const char* chatgpt_token = "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";
const char* temperature = "0.2";
const char* max_tokens = "4000";
String question = "";
String message_for_openai = "";

#define I2S_DOUT 25
#define I2S_BCLK 27
#define I2S_LRC 26

Audio audio;

#define N 4
#define GENERAL 0
#define ALGO 1
#define DS 2
#define IOT 3

#define CHAT_HISTORY_SIZE 5

String rag_data[N];
bool rag_data_included[N] = {false};

String chat_history[CHAT_HISTORY_SIZE] = {""};
bool history_rag_data_included[CHAT_HISTORY_SIZE][N] = {false};
int start = 0;



int countWords(const String& str) {
    int count = 0;
    bool inWord = false;
    
    for (char ch : str) {
        if (ch != ' ' && ch != '\t' && ch != '\n') {
            if (!inWord) {
                // Start of a new word
                inWord = true;
                ++count;
            }
        } else {
            // End of the current word
            inWord = false;
        }
    }
    
    return count;
}

void speakLongText(String longText) {
  int words = countWords(longText);
  const int wordsPerChunk = 10; // Maximum number of words per chunk
  std::vector<String> chunks;
  String currentChunk = "";
  int wordCount = 0;
  bool inWord = false;
  
  // Split the text into chunks
  for (int i = 0; i < longText.length(); i++) {
    char c = longText[i];
    
    if (c == ' ' || c == ',') {
      if (inWord) {
        wordCount++;
        inWord = false;
        if (wordCount >= wordsPerChunk) {
            currentChunk.trim();
          chunks.push_back(currentChunk);
          currentChunk = "";
          wordCount = 0;
        } else {
          currentChunk += ' '; // Add space between words
        }
      }
    } else {
      currentChunk += c;
      inWord = true;
    }
    
    // Handle end of text
    if (i == longText.length() - 1 && !currentChunk.isEmpty()) {
        currentChunk.trim();
      chunks.push_back(currentChunk);
    }
  }
  
  // Process each chunk for speech
  for (const auto& chunk : chunks) {
    Serial.println(chunk); // For debugging, print each chunk
    audio.connecttospeech(chunk.c_str(), "en");
    while (audio.isRunning()) {
      audio.loop();
    }
  }
    // signals that processing is done
  MySerial2.println(words);
}


void reset_history(){
    for(int i=0; i < CHAT_HISTORY_SIZE; i++){
        chat_history[i] = "";
        for(int j=0; j < N; j++){
            history_rag_data_included[i][j] = false;
        }
    }
    start = 0;
}

void initialize_rag_data() {
    rag_data[GENERAL] += "the data about our location: ";
    rag_data[GENERAL] += "we are in TAUB, the computer science faculty in Technion. ";
    rag_data[GENERAL] += "general data about TAUB: ";
    rag_data[GENERAL] += "the TAUB library is on floor one. ";
    rag_data[GENERAL] += "there is a pool table at floor -1. ";


    rag_data[ALGO] = "the data for the algorithms course: ";
    rag_data[ALGO] += "Lectures are on Monday 12:30, Wednesday 14:30. Lecture length is 2 hours. ";
    rag_data[ALGO] += "The course's number is 02340247. ";
    rag_data[ALGO] += "The teaching assistant in charge is Ms. Amit Rozenman Ganz. ";
    rag_data[ALGO] += "The lecturer in charge is Prof. Hadas Shachnai.";


    rag_data[DS] = "the data for the Data Structures course:";
    rag_data[DS] += "Lectures are held on Sunday 10:30, Tuesday 10:30. Lecture length is 2 hours. ";
    rag_data[DS] += "The course's number is 02340218. ";
    rag_data[DS] += "The teaching assistant in charge is Mr. Jonathan Gal. ";
    rag_data[DS] += "The lecturer in charge is Prof. Erez Petrank. ";

    rag_data[IOT] = "the data for the IOT (Internet of Things): ";
    rag_data[IOT] += "course number is 02360333. ";
    rag_data[IOT] += "The projects manager is Mr. Tom Sofer. ";
    rag_data[IOT] += "The lecturer in charge is Mr. Itai Dabran. ";
    rag_data[IOT] += "the IOT laboratories are on floor 2 rooms 236 and 231. ";
    rag_data[IOT] += "Nidal, Yousef and Rami created you in an IOT project with the help of their instructor Yaniv. ";
}

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

// returns context that is relevant to the previous conversation but *hasn't been included in context yet*
String extra_history_context(){
    String extra_context;
    for(int history_i=0; history_i < CHAT_HISTORY_SIZE; history_i++){
        for(int data_i=0; data_i < N; data_i++){
            // is relevant to a previous conversation but hasn't been included in context yet
            if(history_rag_data_included[history_i][data_i] && !rag_data_included[data_i]){
                extra_context += rag_data[data_i];
                rag_data_included[data_i] = true;
            }
        }
    }
    return extra_context;
}

String keyword_context(String word, String prev_word, String prev_prev_word){
    if(word == ""){
        return "";
    }
    int index = -1;
    toLowerCase(word);
    if(word == "algo" || word == "algorithms" || word == "algorithm's"){
        index = ALGO;
    }
    else if(prev_word == "data" && (word == "structures" || word == "structure's")){
        index = DS;
    }
    else if(word == "iot" || (prev_prev_word == "internet" && prev_word == "of" && (word == "things" || word == "thing's"))){
        index = IOT;
    }
    if(index != -1 && !rag_data_included[index]){
        rag_data_included[index] = true;
        return rag_data[index];
    }
    return "";
}

String find_question_context(String question){
    reset_rag();
    String context;
    String word, prev_word, prev_prev_word;
    for(int i=0;i<question.length();i++){
        if(question[i] == ' ' || question[i] == '.' || question[i] == ',' || question[i] == '"'){
            context += keyword_context(word, prev_word, prev_prev_word);
            prev_prev_word = prev_word;
            prev_word = word;
            word = "";
        } else{
            word += question[i];
            if(i == question.length()-1){
                context += keyword_context(word,prev_word,prev_prev_word);
            }
        }
    }
    return context;
}

bool extractWifiAndPassword(const char* inputString, const char*& wifiName, const char*& password) {
    const char* wifiPrefix = "WIFI: ";
    const char* passwordPrefix = "PASSWORD: ";

    // Find the positions of the prefixes
    const char* wifiPos = strstr(inputString, wifiPrefix);
    const char* passwordPos = strstr(inputString, passwordPrefix);

    // Check if both prefixes are found
    if (wifiPos && passwordPos) {
        // Calculate the start and end positions of WiFi name
        wifiPos += strlen(wifiPrefix);
        size_t wifiLength = passwordPos - wifiPos;
        
        // Allocate memory and copy the WiFi name
        char* wifiBuffer = new char[wifiLength + 1];
        strncpy(wifiBuffer, wifiPos, wifiLength);
        wifiBuffer[wifiLength] = '\0'; // Null-terminate the string
        wifiName = wifiBuffer;

        // Calculate the start position of the password
        const char* passwordStart = passwordPos + strlen(passwordPrefix);
        password = passwordStart;

        return true;
    }

    return false;
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


void setup()
{
    Serial.begin(115200);
    MySerial.begin(115200, SERIAL_8N1, 16, 17);
    MySerial2.begin(9600, SERIAL_8N1, 18, 19);

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();


    while (!Serial);

    String wifi_creds = "";

    //Wait for the other ESP to send wifi creds
    while (!MySerial.available()){

    }
    while (MySerial.available())
    {
      char add = MySerial.read();
      wifi_creds = wifi_creds + add;
      delay(1);
    }

    Serial.println("WIFI CREDS ARE " + wifi_creds);

    const char* wifi_creds_char = wifi_creds.c_str(); // Convert String to const char*

    if(!extractWifiAndPassword(wifi_creds_char ,ssid,password)){
      Serial.println("FAILED CONNECTING TO WIFI");
    }

    String temp = String(password);

    temp = cleanString(temp);

    password = temp.c_str();

    Serial.print(ssid);
    Serial.print(password);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        Serial.print(".");
    }

    Serial.println("connected");
    MySerial.println("connected");
    
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audio.setVolume(100);
    
    initialize_rag_data();
}

void loop()
{
    if (MySerial.available())
    {
        question = MySerial.readStringUntil('\n');
        question.trim();

        bool reset_history_command = false;
        bool is_error = false;

        toLowerCase(question);
        if(question == "delete history" || question =="reset history"){
            reset_history_command = true;
            reset_history();
            question = "say 'History has been deleted'";
        }

        if(question[0] == '$'){
            if(question[1] == '#'){
                // wifi credentials

                String wifi_creds = question.substring(2);
                const char* wifi_creds_char = wifi_creds.c_str(); // Convert String to const char*

                if(!extractWifiAndPassword(wifi_creds_char ,ssid,password)){
                Serial.println("FAILED CONNECTING TO WIFI");
                }

                String temp = String(password);

                temp = cleanString(temp);

                password = temp.c_str();

                Serial.print(ssid);
                Serial.print(password);
                WiFi.begin(ssid, password);

                int timeout = 15;
                while (WiFi.status() != WL_CONNECTED && timeout>0)
                {
                    delay(1000);
                    Serial.print(".");
                    timeout-- ;
                }
                if(timeout == 0){
                    MySerial2.print('#');
                }
            }else{
                // error message
                is_error = true;
                String error_message = question.substring(1);
                audio.connecttospeech(error_message.c_str(), "en");
                while (audio.isRunning()) {
                    audio.loop();
                }
                int words = countWords(error_message);
                MySerial2.println(words);
            }

        }else{
            // normal question ( not error/credentials)
        String context = find_question_context(question);
        bool question_rag_data_included[N];
        for(int i=0;i<N;i++){
            question_rag_data_included[i] = rag_data_included[i];
        }
        context += extra_history_context();
        Serial.println("Possibly useful data:\n" + context);

        String history;
        for(int i=0; i < CHAT_HISTORY_SIZE; i++) {
            int index = (start + i) % CHAT_HISTORY_SIZE;
            history += chat_history[index];
        }
        Serial.println("History:\n" + history);

        String message;

        String instructions;

        instructions += "Instructions: ";
        instructions += "you are an LLM assistant in TAUB and your name is Aura. ";
        instructions += "I will provide our conversation history, and some data that may be relevant to our conversation. ";
        instructions += "if the conversation is directly related to the data I provided, use it to answer. ";
        instructions += "keep your answer under 20 words(priority). ";
        instructions += "no need to add 'You:' before your answer. ";
        instructions += "be concise with you answer. ";
        instructions += "don't ask question that are unrelated to the current conversation. ";
        instructions += "instead of saying that the data provided doesn't specify something, say that you don't have enough information to answer. ";

        message += "Conversation History: " + history;

        message += "Data: " + rag_data[GENERAL] + context;

        message += "Current Conversation: Me: " + question + " ";

        Serial.println("Message for ChatGpt:\n" + message);


        message = "\"" + message + "\"";

        HTTPClient https;

        Serial.print("[HTTPS] begin...\n");
        if (https.begin("https://api.openai.com/v1/chat/completions")) {  // HTTPS

            https.addHeader("Content-Type", "application/json");
            String token_key = String("Bearer ") + chatgpt_token;
            https.addHeader("Authorization", token_key);

            String escapedMessage = message;
            escapedMessage.replace("\"", "\\\"");

            String payload = String("{\"model\": \"gpt-4\", \"messages\": [") +
                            "{\"role\": \"system\", \"content\": \"" + instructions + "\"}, " +
                            "{\"role\": \"user\", \"content\": \"" + escapedMessage + "\"}], " +
                            "\"temperature\": " + temperature + ", " +
                            "\"max_tokens\": " + max_tokens + "}";

            Serial.print("[HTTPS] GET...\n");
            Serial.print("[HTTPS] POST...\n");
            Serial.print("Payload: ");
            Serial.println(payload);
            // start connection and send HTTP header
            int httpCode = https.POST(payload);

            // httpCode will be negative on error
            // file found at server
            if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
                String payload = https.getString();
                //Serial.println(payload);

                DynamicJsonDocument doc(16384);

                deserializeJson(doc, payload);
                String answer = doc["choices"][0]["message"]["content"];
                Serial.print("Answer : "); Serial.println(answer);

                speakLongText(answer);

                String question_and_answer = "Me: " + question + " ";
                question_and_answer += "You: " + answer + " ";
                chat_history[start] = question_and_answer;
                for(int i=0;i<N;i++){
                    history_rag_data_included[start][i] = question_rag_data_included[i];
                }
                start++;
                start = start % CHAT_HISTORY_SIZE;
            }
            else {
                Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
            }
            https.end();
        }
        else {
            Serial.printf("[HTTPS] Unable to connect\n");
        }

        if(reset_history_command){
            reset_history();
        }
        question = "";
        message = "";
        }
        
    }
}
