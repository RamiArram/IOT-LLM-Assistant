#include <WiFi.h>
#include <HTTPClient.h>
#include <SPIFFS.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <mbedtls/base64.h>
#include <mbedtls/md.h>
#include <mbedtls/pk.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <ArduinoJson.h>  // Include ArduinoJson library


const char* ssid = "";
const char* password = "";
const char* tokenUrl = "https://oauth2.googleapis.com/token";
const char* privateKeyPem = "";
const char* serviceAccountEmail = "";
const char* bucketName = "storeaudios";  // Replace with your bucket name
const char* objectName = "recording.wav";  // The name of the file in the bucket
const char* spiffsFileName = "/recording.wav";  // The file path on the SD card

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000);

void setup() {
  Serial.begin(115200);


  
  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("Connected to Wi-Fi");

  // Initialize NTP client
  timeClient.begin();
  while (!timeClient.update()) {
    timeClient.forceUpdate();
  }

  // Get current time
  unsigned long currentTime = timeClient.getEpochTime();

  // Create JWT Header
  String header = "{\"alg\":\"RS256\",\"typ\":\"JWT\"}";
  String encodedHeader = base64UrlEncode(header);

  // Create JWT Payload
  String payload = String("{\"iss\":\"") + serviceAccountEmail + "\",\"scope\":\"https://www.googleapis.com/auth/cloud-platform\",\"aud\":\"" + tokenUrl + "\",\"iat\":" + String(currentTime) + ",\"exp\":" + String(currentTime + 3600) + "}";
  String encodedPayload = base64UrlEncode(payload);

  // Combine Header and Payload
  String message = encodedHeader + "." + encodedPayload;

  // Sign the JWT
  String signature = signRS256(message, privateKeyPem);
  String jwt = message + "." + signature;

  // Request Access Token
  HTTPClient http;
  http.begin(tokenUrl);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  // Formulate the POST data
  String postData = "grant_type=urn%3Aietf%3Aparams%3Aoauth%3Agrant-type%3Ajwt-bearer&assertion=" + jwt;

  // Make the POST request
  int httpResponseCode = http.POST(postData);

  String accessToken;
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("Access Token Response:");
    Serial.println(response);

    // Extract access token from response
    int startIndex = response.indexOf("access_token\":\"") + 15;
    int endIndex = response.indexOf("\"", startIndex);
    accessToken = response.substring(startIndex, endIndex);
  } else {
    Serial.println("Error: " + String(httpResponseCode));
    return;
  }

  http.end();

  // Upload file to Google Cloud Storage
  uploadFileToGCS(accessToken);

  // Send file to Google Speech-to-Text API
  sendToSpeechToTextAPI(accessToken);
}
void loop() {
  // Nothing to do here
}

void uploadFileToGCS(String accessToken) {
  File file = SPIFFS.open(spiffsFileName, "r");
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.println("File opened successfully. Uploading...");

  HTTPClient http;
  String url = String("https://storage.googleapis.com/upload/storage/v1/b/") + bucketName + "/o?uploadType=media&name=" + objectName;
  http.begin(url);
  http.addHeader("Authorization", "Bearer " + accessToken);
  http.addHeader("Content-Type", "application/octet-stream");  // Adjust content type based on your file type

  int httpResponseCode = http.sendRequest("POST", &file, file.size());

  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("File Upload Response:");
    Serial.println(response);

    // Check if the upload was successful
    if (httpResponseCode == 200) {
      Serial.println("File uploaded successfully.");
    } else {
      Serial.println("File upload failed.");
    }
  } else {
    Serial.println("Error: " + String(httpResponseCode));
  }

  file.close();
  http.end();
}

void sendToSpeechToTextAPI(String accessToken) {
  HTTPClient http;
  String url = "https://speech.googleapis.com/v1/speech:recognize";
  http.begin(url);
  http.addHeader("Authorization", "Bearer " + accessToken);
  http.addHeader("Content-Type", "application/json");

  // Prepare the JSON request body
  StaticJsonDocument<512> requestBody;
  requestBody["config"]["languageCode"] = "en-US";
  requestBody["config"]["encoding"] = "LINEAR16";
  requestBody["config"]["sampleRateHertz"] = 16000;
  requestBody["audio"]["uri"] = String("gs://") + bucketName + "/" + objectName;

  String jsonString;
  serializeJson(requestBody, jsonString);

  // Send the request
  int httpResponseCode = http.POST(jsonString);

  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("Speech-to-Text Response:");
    Serial.println(response);
  } else {
    Serial.println("Error: " + String(httpResponseCode));
  }

  http.end();
}

String base64UrlEncode(String input) {
  // Base64 encode and replace URL-unsafe characters
  size_t outputLength;
  unsigned char output[512];
  mbedtls_base64_encode(output, sizeof(output), &outputLength, (const unsigned char*)input.c_str(), input.length());
  String encoded = String((char*)output);
  encoded.replace("+", "-");
  encoded.replace("/", "_");
  encoded.replace("=", "");
  return encoded;
}

String signRS256(String message, const char* privateKeyPem) {
  mbedtls_pk_context pk;
  mbedtls_pk_init(&pk);

  mbedtls_entropy_context entropy;
  mbedtls_ctr_drbg_context ctr_drbg;
  mbedtls_entropy_init(&entropy);
  mbedtls_ctr_drbg_init(&ctr_drbg);
  const char* pers = "jwt_sign";
  mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char*)pers, strlen(pers));

  if (mbedtls_pk_parse_key(&pk, (const unsigned char*)privateKeyPem, strlen(privateKeyPem) + 1, NULL, 0, mbedtls_ctr_drbg_random, &ctr_drbg) != 0) {
    Serial.println("Failed to parse private key");
    return "";
  }

  unsigned char hash[32];
  mbedtls_md(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), (const unsigned char*)message.c_str(), message.length(), hash);

  unsigned char signature[512];
  size_t signatureLength;
  if (mbedtls_pk_sign(&pk, MBEDTLS_MD_SHA256, hash, sizeof(hash), signature, sizeof(signature), &signatureLength, mbedtls_ctr_drbg_random, &ctr_drbg) != 0) {
    Serial.println("Failed to sign message");
    return "";
  }

  mbedtls_pk_free(&pk);
  mbedtls_ctr_drbg_free(&ctr_drbg);
  mbedtls_entropy_free(&entropy);

  return base64UrlEncode(String((char*)signature, signatureLength));
}
