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


const char* ssid = "DOGGO ";
const char* password = "ramiharami";
const char* tokenUrl = "https://oauth2.googleapis.com/token";
const char* privateKeyPem = "-----BEGIN PRIVATE KEY-----\nMIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQCSDCbrUjxG/LKt\n9A/ColKzHPYhI37ASSzv+I8n4UALaV4bnOj5fcPRqFFX2H2/t5U1Zom9ElVFRfCr\nxNCHBNqpWq1SZV+ZuPXQslNxH70VPk87/+3xPZI/4CtzRmdBiayUsNNDHINFfiWJ\nuxGn+u2nTJjyk9TnMHmzr9Fn+Vv650lcXEjzPzc5gjxzpudDM9IjXYh/efNUi5vE\nYbPvh6oM5Qw8wEK2WDNs9L3FbV+SJP3kTOkBdVWEqStHy2lEge2z3XDgzyTRWR3L\nsIWBh05mjywG/Mx5/dDOilHibp+e2v3uuybQjZv+Q1NqTY+cu9t2njwLy2fmsVdR\n5P3nWOClAgMBAAECggEAHvbynXv/CbH0FEult8eArzcZvCbxbcVWAL77YyIv2PuL\nF2GqR5NxnBcKwAxHhQlgWkUWOP8VEy3j/BcreyT9U2DFVK3nmgkMHSZh6J8pEML4\nO6lkgnxicKaPr84igFO3QoNHF3iJIIfBX7VI5guerZTg7LWJM0ck1oMVa+0Domtr\nNBgEkQ64LU/NYZhn1WQaOyDifCybGmUi7I6J+BvCDjf6iZP95RrcQrYqb4WF/PGv\nag54LkpZ7PP7E3i3bT9JGhdQb+OqWf4Uwmmziq498ZIJ9WNZvvbzPE9M1Te8XwY0\n2/NXpYa9RogJBu38Pk2ijDZetPt88Le1rvXLlCI5gQKBgQDDaD+qJEbcoPTo7TuC\nZBGXl/rkrGJgNWpKQULNKl947Ajt4jlccdIHq8PIvYoDPYWNIUh9ivWuT36m5H3k\nQQUmwnggnhru8/3ka8T8GEGw5DtAvLh4gSl76zERp8iSdyyJ9/zcBWOFOKcrpFug\neIxbGdkVIUiu9z6HgBKkFRvCoQKBgQC/VaTQ5tLkz7as1jeiJzpCqkexYiudYjm+\nG7egNo+jmLAunJXVIbUbtx+6ZLCmEqJrJbKhucXc1YLNzH0Xih24yVWkuvkl7y7F\nlv+/pOVInYDdGIj9zDGlGdYuQZo2X6k5Bd2aDj0bJt0ucX1c7Iwx3GmRfeXvvh2h\neJ9vi1jjhQKBgCBy+FeFrKrIo8LmWkJ94Zn95/D/W1rEpADus2wkhDQtZhIyxfm/\nAPS7Jkj4iKmfFsVn6ITIhaHLm6mVOf6keXmFQ7OO7cQOinPbHZwLXyVD0T8f17ZT\nJJNqa/yVky4XUjMbFkdnMa6WyNPreDe+rPgX47+phktfzWds8iuDQdfhAoGBAKa8\nJIejTs70r9VITGzzUFL9/sH2sdlR2s3va4KjaWMAUTAdZni4ChJf77dHvaTLglxr\nGpLSyDlcmsNq2uvjgWkhko+eHDZRi+nGX5KLSP+RudVyZxb8lZqxvDLNGm0dyeMW\nWHnCxL9fi8nIPp32yWk+EYUnOHRGsZ2f9xiGT2zdAoGAeYx7oSh/X+de0/2Y3TOj\noQcSpF3+NEZJZlrflSnXxvFVItvvnFF8K+ThLpoN28t/WRwqyv0/WUEqbFFH2O7p\nHeBjwB26vsQGjBtOpypRs+AEDpcFr2R785GEtPSKzzUsjBF7tWUS0TMfs6F5NLJ+\nLeHtxosivhTgLo7+BDR+lu0=\n-----END PRIVATE KEY-----\n";
const char* serviceAccountEmail = "storage-bucket-view@centered-cable-427011-t5.iam.gserviceaccount.com";
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