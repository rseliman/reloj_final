#include "WiFi.h"
#include "WifiCam.hpp"
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <esp_camera.h>

const char *ssid = "JoaoM";
const char *password = "password";
bool botonHabilitado = true;
const int botonPin = D2;  // Pin donde está conectado el botón
int a = 0;
const int led = D9;
//const int sensor = D7;
//const int vibra = D0;
bool inicializado = false;

esp32cam::Resolution initialResolution;
WebServer server(80);

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("Configuring access point...");

  pinMode(led, OUTPUT);  // Configurar el pin del LED como salida
  pinMode(botonPin, INPUT_PULLUP);
  //pinMode(vibra, OUTPUT);

 // wifini(); 
  //cameraini();
  }

void loop() {


 if (botonHabilitado) {
    //if (digitalRead(botonPin) == LOW)
    {
     wifini(); 
     cameraini();
     
     botonHabilitado = false;
     delay (1000);
    }
 }
 
  delay(200);

 if (botonHabilitado == false )
  { 
   server.handleClient();
  } 

}

void cameraini ()
{
 
  using namespace esp32cam;

  initialResolution = Resolution::find(800, 600);

  Config cfg;
  cfg.setPins(pins::xiaoesp32cam);
  cfg.setResolution(initialResolution);
  cfg.setJpeg(70);

  bool ok = Camera.begin(cfg);
  if (!ok) {
      Serial.println("camera initialize failure");
      delay(5000);
      ESP.restart();
    }
  Serial.println("camera initialize success");
  

  Serial.println("camera starting");

  
  server.begin();
  addRequestHandlers();
 

}

void wifini()
{

  WiFi.softAP(ssid, password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.begin();

  Serial.println("Server started");
  
}  



