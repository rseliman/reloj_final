#include "WifiCam.hpp"
#include <WiFi.h>


static const char* WIFI_SSID = "JoaoMagic";
static const char* WIFI_PASS = "oporto2016";
const int botonPin = D1;  // Pin donde está conectado el botón
int a = 0;
const int led = D9;
const int sensor = D7;
bool botonHabilitado = true;
int val;  // variable temporal sensor 

esp32cam::Resolution initialResolution;
WebServer server(80);



void
setup()
{
  Serial.begin(115200);
  Serial.println();
  delay(2000);
  pinMode(botonPin, INPUT_PULLUP);
  pinMode(sensor, INPUT);
  pinMode(led, OUTPUT);
  
  
 // cameraini();
 // wifini();   
}

void
loop()
{
  if (botonHabilitado) {
   if (digitalRead(botonPin) == LOW)
   {
    wifini();
    cameraini();
    botonHabilitado = false;
   }
  }

  if (botonHabilitado == false )
  { 
  server.handleClient();
  } 

 // val = digitalRead (sensor) ;

 // if (val == HIGH)
//  {
//  digitalWrite(led, HIGH);
//  } 
//  else
//  {
//  digitalWrite(led, LOW);
 // }
}

void cameraini ()
{
 
  using namespace esp32cam;

  initialResolution = Resolution::find(1024, 768);

  Config cfg;
  cfg.setPins(pins::xiaoesp32cam);
  cfg.setResolution(initialResolution);
  cfg.setJpeg(80);

  bool ok = Camera.begin(cfg);
  if (!ok) {
      Serial.println("camera initialize failure");
      delay(5000);
      ESP.restart();
    }
  Serial.println("camera initialize success");
  

  Serial.println("camera starting");
  Serial.print("http://");
  Serial.println(WiFi.localIP());
  addRequestHandlers();
  server.begin();

}

void wifini()
{
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("WiFi failure");
    delay(5000);
    ESP.restart();
  }
  Serial.println("WiFi connected");
}

