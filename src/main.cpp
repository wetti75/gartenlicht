#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <Arduino.h>

//Für Temperatursensor
#include <OneWire.h>


//Webserver
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
ESP8266WebServer server(8080);


//Wifi
#define STASSID "FRITZ!Box 7560 TZ"
#define STAPSK  "95172809322535102456"
const char* ssid = STASSID;
const char* password = STAPSK;


//NTP Timeserver
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org");


//Wemo Alexa
#include <fauxmoESP.h>
fauxmoESP fauxmo;

//OTA Updater
const char* host = "Pool-Controller";

bool lightIsOn = true;

void handleStats() {
  Serial.println("handle on web server");
  String message;
  message.reserve(2600);

  message = "<html><head><title>Poolcontroller Stats</title></head><body>";
  message += "<h2>Hello Gartenlicht ...</h2>";
  if(lightIsOn) {
    message += "<div>Status: On</div>";
  }else {
    message += "<div>Status: Off</div>";
  }
  message += "</body></html>";

  server.send(200, "text/html", message);
  delay(500);
}

void handleOn() {
  lightIsOn = true;
  server.send(200, "application/json", "true");
  delay(500);
}
void handleOff() {
  lightIsOn = false;
  server.send(200, "application/json", "false");
  delay(500);
}

void setup() {
  Serial.begin(9600);
  Serial.println("setup ...");

  Serial.println("ini wifi");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(F("."));
    Serial.println("connecting wifi");
  }
  Serial.println(WiFi.localIP().toString());

  
  Serial.println("start NTP");
  timeClient.begin();
  timeClient.setTimeOffset(60 * 60 * 2);
  timeClient.update();
  delay(500);
  
  Serial.println("start webserver");
  server.on("/on/", handleOn);
  server.on("/off/", handleOff);
  server.on("/", handleStats);
  server.begin();
  delay(500);

  Serial.println("setup finish");

  pinMode(12, OUTPUT);
  digitalWrite(12, HIGH);
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);


  Serial.println("setup fauxmo");  
  fauxmo.createServer(true);
  fauxmo.setPort(80);
  fauxmo.enable(true);

  fauxmo.addDevice("Gartenlicht");
  fauxmo.setState("Gartenlicht", lightIsOn, 254);

  fauxmo.onSetState([](unsigned char device_id, const char * device_name, bool state, unsigned char value) {
        Serial.print("Device "); Serial.print(device_name); 
        Serial.print(" state: "); Serial.print(state); 
        Serial.print(" value: "); Serial.print(value);
        if(String(device_name) == String("Gartenlicht")) {
          lightIsOn = state;
        }
    });

  //pinMode(13, OUTPUT);


  ArduinoOTA.setHostname(host);
  ArduinoOTA.onStart([]() { // switch off all the PWMs during upgrade
    Serial.print("Systemupdate ...");
  });

  ArduinoOTA.onEnd([]() { // do a fancy thing with our board led at end
    Serial.print("Update finish ...");
    delay(1000);
    Serial.print("restart ESP ...");
    delay(1000);
    ESP.restart();
  });

  ArduinoOTA.onError([](ota_error_t error) {
    (void)error;
    Serial.print("Update error ...");
    delay(1000);
    ESP.restart();
  });

  /* setup the OTA server */
  ArduinoOTA.begin();
  
}


void loop() {
  ArduinoOTA.handle();
  fauxmo.handle();

  String formattedTime = timeClient.getFormattedTime();
  server.handleClient();

  if(lightIsOn) {
    //Serial.print("Light is LOW");
    digitalWrite(13, LOW);
    fauxmo.setState("Gartenlicht", false, 254);
  }else {
    //Serial.print("Light is HIGH");
    digitalWrite(13, HIGH);
    fauxmo.setState("Gartenlicht", true, 254);
  }

  delay(500);


  //Serial.println("Going to deep sleep...");
  //ESP.deepSleep(60 * 1000000);

}