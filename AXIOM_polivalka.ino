#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <arduino-timer.h>

#define MOSFET D8
#define LED D4                                     
#define SENSOR A0

const char *ssid = "AXIOM";
const char *password = "axiom123";

ESP8266WebServer server(80);
int moisture;
unsigned long sensorOnTime = 0;

auto timer = timer_create_default();
Timer<>::Task ledB;
bool ledBlinking = false;

void ledBlink(unsigned long time) {
  ledBlinking = true;
  ledB = timer.every(100, [](void*) -> bool { digitalWrite(LED, !digitalRead(LED)); return true; });
  timer.in(time, [](void*) -> bool {
    if (ledBlinking) {
      pumpOn(5000);
      timer.cancel(ledB);
      digitalWrite(LED, 0);
      ledBlinking = false;
    }
    return true;
  });
}

void pumpOn(unsigned long time) {
  digitalWrite(MOSFET, 1);
  timer.in(time, [](void*) -> bool { digitalWrite(MOSFET, 0); return true; });
}

void handleRoot() {
  char s[10000];
  
  server.send(200, "text/html", "\
  <h1>AXIOM</h1>\
  <p>Welcome onboard.</p>\
  <p>The current value of sensor is <span id=\"val\"></span>.</p>\
  <p><button onclick=\"fetch('/polit')\">Polit</button></p>\
  <script>\
    setInterval(() => {\
      fetch('/sensor')\
        .then(r => r.text())\
        .then(t => document.getElementById(\"val\").innerHTML = t)\
    }, 1000)\
  </script>\
  ");
}

void handleSensor() {
  server.send(200, "text/plain", String(moisture));
}

void handlePolit() {
  pumpOn(5000);
  server.send(200, "text/plain", "ok");
}

void setup() {
  delay(1000);

  pinMode(MOSFET, OUTPUT);
  pinMode(LED, OUTPUT);
  pinMode(SENSOR, INPUT);

  Serial.begin(115200);
  Serial.println();
  Serial.print("Configuring access point...");
  /* You can remove the password parameter if you want the AP to be open. */
  WiFi.softAP(ssid, password);

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.on("/", handleRoot);
  server.on("/sensor", handleSensor);
  server.on("/polit", handlePolit);

  server.begin();
  Serial.println("HTTP server started");

  // Read sensor every 1 s.
  timer.every(1000, [](void*) -> bool { 
    moisture = analogRead(SENSOR);
    //moisture < 700 && !ledBlinking
    //moisture > 700 && ledBlinking
    if (moisture > 900 && !ledBlinking) {
      ledBlink(5000);
    }
    if (moisture < 900 && ledBlinking) {
      timer.cancel(ledB); 
      digitalWrite(LED, 0); 
      ledBlinking = false;
    }    
    return true; 
  });
}

void loop() {
  server.handleClient();
  timer.tick<void>();
}
