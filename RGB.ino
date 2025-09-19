#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// ====== WiFi Settings ======
const char* ssid = "WI FI";       // 🔹 replace with your WiFi SSID
const char* password = "PASSWORD";   // 🔹 replace with your WiFi Password
const char* ap_ssid = "ESP8266";     // DO NOT CHANGE
const char* ap_password = "";            // DO NOT CHANGE

// ====== RGB LED Pins ======
#define RED_PIN   D4
#define GREEN_PIN D5
#define BLUE_PIN  D6

ESP8266WebServer server(80);

// Current RGB values
int redVal = 0, greenVal = 0, blueVal = 0;
bool effectRunning = false;
String currentEffect = "";

// ====== Helper: Set LED color ======
void setColor(int r, int g, int b) {
  // ✅ Common Cathode → direct values
  analogWrite(RED_PIN,   r);
  analogWrite(GREEN_PIN, g);
  analogWrite(BLUE_PIN,  b);
}

// ====== Effects ======
void runEffect() {
  if (!effectRunning) return;

  if (currentEffect == "fade") {
    for (int i = 0; i < 255 && effectRunning; i++) {
      setColor(i, 0, 255 - i);
      delay(10);
    }
    for (int i = 0; i < 255 && effectRunning; i++) {
      setColor(255 - i, i, 0);
      delay(10);
    }
  }
  else if (currentEffect == "blink") {
    setColor(255, 255, 255);
    delay(500);
    setColor(0, 0, 0);
    delay(500);
  }
  else if (currentEffect == "breathing") {
    for (int i = 0; i < 255 && effectRunning; i++) {
      setColor(i, i, i);
      delay(10);
    }
    for (int i = 255; i > 0 && effectRunning; i--) {
      setColor(i, i, i);
      delay(10);
    }
  }
  else if (currentEffect == "rainbow") {
    for (int i = 0; i < 255 && effectRunning; i++) {
      setColor(i, 255 - i, (i * 2) % 255);
      delay(15);
    }
  }
  else if (currentEffect == "wave") {
    for (int i = 0; i < 255 && effectRunning; i++) {
      setColor(sin(i * 0.1) * 127 + 128, cos(i * 0.1) * 127 + 128, sin(i * 0.1 + 2) * 127 + 128);
      delay(20);
    }
  }
  else if (currentEffect == "random") {
    setColor(random(0, 255), random(0, 255), random(0, 255));
    delay(500);
  }
}

// ====== Web Pages ======
void handleRoot() {
  String html = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
    <title>ESP8266 RGB Controller</title>
    <style>
      body { background:#111; color:white; text-align:center; font-family:Arial; }
      button { margin:5px; padding:10px; font-size:18px; border-radius:10px; background:#333; color:white; border:none; }
      input[type=color]{ width:120px; height:60px; border:none; margin:15px; }
    </style>
  </head>
  <body>
    <h2>ESP8266 RGB LED Controller</h2>
    <input type="color" id="colorPicker" value="#ff00ff">
    <br>
    <button onclick="setColor()">Set Solid Color</button>
    <br><br>
    <button onclick="sendEffect('fade')">Fade</button>
    <button onclick="sendEffect('blink')">Blink</button>
    <button onclick="sendEffect('breathing')">Breathing</button>
    <button onclick="sendEffect('rainbow')">Rainbow</button>
    <button onclick="sendEffect('wave')">Wave</button>
    <button onclick="sendEffect('random')">Random Colors</button>
    <button onclick="sendEffect('stop')">Stop Effects</button>

    <script>
      function setColor(){
        let color = document.getElementById("colorPicker").value.substring(1);
        fetch("/set?color=" + color);
      }
      function sendEffect(effect){
        fetch("/effect?name=" + effect);
      }
    </script>
  </body>
  </html>
  )rawliteral";

  server.send(200, "text/html", html);
}

void handleSetColor() {
  effectRunning = false;
  if (server.hasArg("color")) {
    String hex = server.arg("color");
    long number = strtol(hex.c_str(), NULL, 16);

    redVal   = (number >> 16) & 0xFF;
    greenVal = (number >> 8) & 0xFF;
    blueVal  = number & 0xFF;

    setColor(redVal, greenVal, blueVal);

    server.send(200, "text/plain", "Color Set: " + hex);
  }
}

void handleEffect() {
  if (server.hasArg("name")) {
    String name = server.arg("name");
    if (name == "stop") {
      effectRunning = false;
      setColor(redVal, greenVal, blueVal);
    } else {
      effectRunning = true;
      currentEffect = name;
    }
    server.send(200, "text/plain", "Effect: " + name);
  }
}

// ====== Setup ======
void setup() {
  Serial.begin(115200);

  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  analogWriteRange(255);

  // Try to connect to home WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  int timeout = 20; // ~10s
  while (WiFi.status() != WL_CONNECTED && timeout > 0) {
    delay(500);
    Serial.print(".");
    timeout--;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to WiFi!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi Failed! Starting Access Point...");
    WiFi.softAP(ap_ssid, ap_password);
    Serial.print("Connect to AP, open: http://");
    Serial.println(WiFi.softAPIP());
  }

  server.on("/", handleRoot);
  server.on("/set", handleSetColor);
  server.on("/effect", handleEffect);

  server.begin();
  Serial.println("ESP8266 RGB Controller Ready!");
}

// ====== Loop ======
void loop() {
  server.handleClient();
  runEffect();
}
