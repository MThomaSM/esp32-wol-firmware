#include "WiFi.h"
#include "WiFiUdp.h"
#include <Arduino_JSON.h>
#include <WakeOnLan.h>
#include <HTTPClient.h>
#include "SPIFFS.h"

WiFiUDP UDP;
WakeOnLan WOL(UDP);

const char* ssid = "nothing";
const char* password =  "nothing";
String uuid =  "nothing";
String base_api_url = "https://pcmanager-backend.tomasmagnes.dev/api"; //base url to backend

String readGETWebResponse(String url){
  HTTPClient https;
  https.begin(url);
  int httpCode = https.GET();
  if (httpCode != HTTP_CODE_OK) return "Wrong response code. Expected 200";
  String output = https.getString();
  return output;
}

String readFile(String infile){
  if (!SPIFFS.begin(true)) return "An Error has occurred while mounting SPIFFS";
  File file = SPIFFS.open(infile, "r");
  if(!file) return "File doesnt exist";
  String output = file.readString();
  file.close();
  return output;
}

void appendToFile(String filename, String content){
  if (!SPIFFS.begin(true)) return;
  File fileToAppend = SPIFFS.open(filename, FILE_APPEND);
  if(!fileToAppend){
      Serial.println("There was an error opening the file for appending");
      return;
  }
  fileToAppend.println(content);
  fileToAppend.close();
}

void setup(){
  //Main config
  Serial.begin(115200); //nastavenie komunikacie na "frekvencii" 115200 na ktoréj ESP32 komunikuje
  WOL.setRepeat(3, 100); //3 - vždy 3 woll packety, 100 - s rozostupom 100 milisekund
  WiFi.mode(WIFI_STA); //nastavenie wifi
  WiFi.disconnect(); //odpojenie z wifi ak na nejakej bol predtým pripojený
  delay(100); //delay 100ms
  JSONVar config = JSON.parse(readFile("/config.json"));
  ssid = config["wifi_name"];
  password = config["wifi_password"]; 
  uuid = config["uuid"]; 
  Serial.println("==================================="); //vypisi preistotu
  Serial.println("Configuration acording config.json:");
  Serial.print("System uuid:");
  Serial.println(uuid);
  Serial.print("Wifi name:");
  Serial.println(ssid);
  Serial.print("Wifi password:");
  Serial.println(password);
  Serial.println("===================================");
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { //pripajanie k wifi
    delay(1000);
    Serial.println("Connecting..");
  }
  Serial.println("Connected successfully");
  Serial.println("===================================");
  Serial.println("Recieved network configuration:"); //konfiguracia ktorú dostal od wifi routru
  Serial.print("IP:");
  Serial.println(WiFi.localIP());
  Serial.print("Subnet mask:");
  Serial.println(WiFi.subnetMask());
  Serial.println("===================================");
  WOL.calculateBroadcastAddress(WiFi.localIP(), WiFi.subnetMask()); //nastavenie wol podľa sieťovej konfiguracie
  Serial.println("Setup was successfull");
  Serial.println("===================================");
    
}

void loop() { 
  if(millis() >= 86400000) return ESP.restart();
  if(WiFi.status() != WL_CONNECTED) return setup();
  String response = readGETWebResponse(base_api_url + "/startlist/" + uuid + "/maclist");
  JSONVar apiResponseObj = JSON.parse(response);
  
  if (JSON.typeof(apiResponseObj) == "undefined") {
    Serial.println("Parsing JSON failed!");
    return;
  }

  for(int i = 0; i < apiResponseObj.length(); i++){
      String macAddress = (const char*)apiResponseObj[i];
      Serial.println("Sending magic packets to " + macAddress);
      WOL.sendMagicPacket(macAddress);
      delay(500);
      readGETWebResponse(base_api_url + "/startlist/" + uuid + "/mac/" + macAddress + "/remove");
      Serial.println(macAddress + " was removed from startlist");
  }
  delay(8000);
}
