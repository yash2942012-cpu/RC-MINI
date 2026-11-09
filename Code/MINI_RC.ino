#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "MicroRC-Car";
const char* password = "password123";

WebServer server(80);

const int IN1 = 0;
const int IN2 = 1; 
const int IN3 = 2;
const int IN4 = 3; 

const char HTML_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Micro RC Controller</title>
  <style>
    body { font-family: sans-serif; text-align: center; background: #222; color: #fff; margin-top: 50px; }
    button { width: 100px; height: 100px; font-size: 20px; margin: 10px; background: #00ADB5; color: white; border: none; border-radius: 15px; cursor: pointer; }
    button:active { background: #393E46; }
    .container { display: flex; flex-direction: column; align-items: center; }
    .row { display: flex; }
  </style>
</head>
<body>
  <h1>Micro RC Car</h1>
  <div class="container">
    <button onmousedown="fetch('/forward')" ontouchstart="fetch('/forward')" onmouseup="fetch('/stop')" ontouchend="fetch('/stop')">FORWARD</button>
    <div class="row">
      <button onmousedown="fetch('/left')" ontouchstart="fetch('/left')" onmouseup="fetch('/stop')" ontouchend="fetch('/stop')">LEFT</button>
      <button onmousedown="fetch('/stop')" ontouchstart="fetch('/stop')">STOP</button>
      <button onmousedown="fetch('/right')" ontouchstart="fetch('/right')" onmouseup="fetch('/stop')" ontouchend="fetch('/stop')">RIGHT</button>
    </div>
    <button onmousedown="fetch('/reverse')" ontouchstart="fetch('/reverse')" onmouseup="fetch('/stop')" ontouchend="fetch('/stop')">REVERSE</button>
  </div>
</body>
</html>
)rawliteral";

void handleRoot() {
  server.send(200, "text/html", HTML_PAGE);
}

void handleForward() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  server.send(200, "text/plain", "Moving Forward");
}

void handleReverse() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  server.send(200, "text/plain", "Moving Reverse");
}

void handleLeft() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  server.send(200, "text/plain", "Turning Left");
}

void handleRight() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  server.send(200, "text/plain", "Turning Right");
}

void handleStop() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  server.send(200, "text/plain", "Stopped");
}

void setup() {
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  handleStop();
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();

  server.on("/", handleRoot);
  server.on("/forward", handleForward);
  server.on("/reverse", handleReverse);
  server.on("/left", handleLeft);
  server.on("/right", handleRight);
  server.on("/stop", handleStop);
  
  server.begin();
}

void loop() {
  server.handleClient();
}
