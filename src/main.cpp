#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <FS.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

#define SSID_LENGTH 32

#define FORMAT_SPIFFS_IF_FAILED true
#define WIFI_CONFIG_PATH "/wifi.json"

#define DEVICE_SSID "WIFI_SETUP"
#define DEVICE_DEFAULT_PASSPHRASE "WIFI_SETUP"

struct wifi_config {
  char SSID[128];
  char PSK[128];
  bool valid;
};

bool connectToWifi(fs::SPIFFSFS &fs);
wifi_config readWifiConfig(fs::SPIFFSFS &fs);
bool writeWifiConfig(fs::SPIFFSFS &fs, wifi_config myWifiConfig);
char** scanWifi(int* size);
void setupWifi(fs::SPIFFSFS &fs);

void setup() {
  //Initialize serial
  Serial.begin(115200);

  //Initialize SPIFFS and format if failed
  if(!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)){
    Serial.println("SPIFFS Mount Failed");
    return;
  }

}

void loop() {

  if(!connectToWifi(SPIFFS)){
    setupWifi(SPIFFS);
    return;
  }

  Serial.setDebugOutput(true); 
  Serial.println();
  Serial.println("----------   WiFi Info --------");
  WiFi.printDiag(Serial); 
  Serial.println("----------   WiFi Info --------");
  Serial.println();

  while(true){
    if (Serial.available() > 0) {
      SPIFFS.format();
      Serial.write("Formatted SPIFFS");
    }
  }

}

bool writeWifiConfig(fs::SPIFFSFS &fs, wifi_config myWifiConfig){
  File file = fs.open(WIFI_CONFIG_PATH, FILE_WRITE);
  if(!file){
    Serial.println("There was an error opening the file for writing");
    return false;
  }

  StaticJsonDocument<256> jsonWifiConfig;
  char output[256];
  jsonWifiConfig["ssid"] = myWifiConfig.SSID;
  jsonWifiConfig["psk"] = myWifiConfig.PSK;

  if (serializeJson(jsonWifiConfig, output) == 0) {
    Serial.println(F("Failed to serialize the JSON"));
    file.close();
    return false;
  }

  if(!file.print(output)){
    Serial.println("Failed to write the file");
    file.close();
    return false;
  }

  Serial.println(F("Wrote config successfully!"));
  file.close();
  return true;

}

wifi_config readWifiConfig(fs::SPIFFSFS &fs){
  File file = fs.open(WIFI_CONFIG_PATH);
  if(!file){
    Serial.println("There was an error opening the file for reading");
    return wifi_config{"", "", false};
  }

  StaticJsonDocument<256> jsonWifiConfig;
  DeserializationError error = deserializeJson(jsonWifiConfig, file);
  if (error){
    Serial.println(F("Failed to deserialize config file"));
    Serial.print(error.c_str());
    file.close();
    return wifi_config{"", "", false};
  }

  wifi_config myWifiConfig;
  strlcpy(myWifiConfig.SSID,
    jsonWifiConfig["ssid"] | "",
    sizeof(myWifiConfig.SSID));
  strlcpy(myWifiConfig.PSK,
    jsonWifiConfig["psk"] | "",
    sizeof(myWifiConfig.PSK));
  myWifiConfig.valid = true;

  // Close the file
  Serial.println(F("Read config successfully!"));
  file.close();
  return myWifiConfig;
}

bool connectToWifi(fs::SPIFFSFS &fs){
  if(!fs.exists(WIFI_CONFIG_PATH)){
    Serial.println("Config file not found");
    return false;
  }

  wifi_config myWifiConfig = readWifiConfig(fs);
  if(!myWifiConfig.valid){
    Serial.println("Stored Wifi config invalid");
    return false;
  }
  WiFi.mode(WIFI_MODE_STA);
  WiFi.begin(myWifiConfig.SSID, myWifiConfig.PSK);
  return true;
}

void setupWifi(fs::SPIFFSFS &fs){
  //write config file/APSTA mode
  WiFi.mode(WIFI_MODE_APSTA);
  WiFi.softAP(DEVICE_SSID, DEVICE_DEFAULT_PASSPHRASE);
  /* while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
  }
  Serial.println(WiFi.localIP());
  if (!MDNS.begin("esp32")) {
    Serial.println("Error setting up MDNS responder!");
    while(1) {
        delay(1000);
    }
  }
  Serial.println("mDNS responder started");

  // Start TCP (HTTP) server
  WiFiServer server(80);
  server.begin();
  Serial.println("TCP server started");

  // Add service to MDNS-SD
  MDNS.addService("http", "tcp", 80);

  //get list of networks
  int sizeOfNetworks;
  char** myNetworks = scanWifi(&sizeOfNetworks);
  String networks;
  //free memory from networks
  for( int i = 0; i < sizeOfNetworks; i++){
    networks += String(myNetworks[i]) + "\r\n";
    free(myNetworks[i]);
  }
  free(myNetworks);

  String res;
  WiFiClient client = server.available();
  while(!client) {}
  while(client.connected() && !client.available()){
    delay(1);
  }
  client.flush();
  res = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>Available APs: ";
  res += networks;
  res += "</html>\r\n\r\n";
  client.print(res); */

  //get list of networks
  int sizeOfNetworks;
  char** myNetworks = scanWifi(&sizeOfNetworks);
  String networks;
  //free memory from networks
  for( int i = 0; i < sizeOfNetworks; i++){
    networks += String(myNetworks[i]) + "\r\n";
    free(myNetworks[i]);
  }
  free(myNetworks);
  Serial.println(networks);

  return ;

  wifi_config myWifiConfig;
  strlcpy(myWifiConfig.SSID,
    "xyz",
    sizeof(myWifiConfig.SSID));
  strlcpy(myWifiConfig.PSK,
    "123456",
    sizeof(myWifiConfig.PSK));
  myWifiConfig.valid = true;

  writeWifiConfig(fs, myWifiConfig);
}

char** scanWifi(int* size){
  int n;
  n = WiFi.scanNetworks();
  if (n == 0) return {};
  char** myNetworks = (char**) malloc(sizeof(char*) * n);
  *size = n;
  for (int i = 0; i < n; ++i) {
    int length = WiFi.SSID(i).length() + 1;
    if(length > SSID_LENGTH) length = SSID_LENGTH;
    myNetworks[i] = (char*) malloc(SSID_LENGTH);
    WiFi.SSID(i).toCharArray(myNetworks[i], length, 0);
  }
  return myNetworks;
}
