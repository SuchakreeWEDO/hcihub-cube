#include "ESP8266WiFi.h"
#include "ESP8266WebServer.h"
#include "ESP8266mDNS.h"


#include "WiFiClient.h"
#include "WiFiUdp.h"
#include "OSCMessage.h"
#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"
#include "Wire.h"

#include "iostream"
#include "string"
#include "vector"

#define SSID "HCI"
#define PASS "hciLAB##"

using std::vector;
using std::string;

char* name = "cube1";
const int led = 13;
int count = 0;
const unsigned int outPort = 9999;    // remote port to receive OSC
const unsigned int localPort = 8888;  // local port to listen for OSC packets (actually not used for sending)


vector<IPAddress> targets;
ESP8266WebServer server(80);
WiFiUDP Udp;
MPU6050 mpu;
int16_t ax, ay, az;
int16_t gx, gy, gz;
int valx, valy, valz;



void handleRoot() {
  digitalWrite(led, 1);
  server.send(200, "text/plain", "hello from esp8266!\r\n");
  digitalWrite(led, 0);
}

void handleNotFound() {
  digitalWrite(led, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) { message += " " + server.argName(i) + ": " + server.arg(i) + "\n"; }
  server.send(404, "text/plain", message);
  digitalWrite(led, 0);
}

void addIP(IPAddress ip) {
  for (int i = 0; i < targets.size(); i++) {
    if (ip == targets[i]) {
      return;
    }
  }
  targets.push_back(ip);
  Serial.print(ip.toString().c_str());
  Serial.println(" is in list.");
}


void removeIP(IPAddress ip) {
  for (int i = 0; i < targets.size(); i++) {
    if (ip == targets[i]) {
      targets.erase(targets.begin() + i);
      return;
    }
  }
}

void onEnable() {
  IPAddress clientIP = server.client().remoteIP();
  addIP(clientIP);
  server.send(200, "text/plain", "enable input!\r\n");
  Serial.print("enable: ");
  Serial.println(clientIP.toString());
}

void onDisable() {
  IPAddress clientIP = server.client().remoteIP();
  removeIP(clientIP);
  server.send(200, "text/plain", "disable input!\r\n");
  Serial.print("enable: ");
  Serial.println(clientIP.toString());
}

void sendAccelerometer(IPAddress ip) {
  OSCMessage msg("/cube1/angles");
  msg.add(ax);
  msg.add(ay);
  msg.add(az);
  msg.add(gx);
  msg.add(gy);
  msg.add(gz);
  Udp.beginPacket(ip, outPort);
  msg.send(Udp);
  Udp.endPacket();
  msg.empty();
}

void sendGyro(IPAddress ip) {
  OSCMessage msg("/cube1/gyro");
  msg.add(gx);
  msg.add(gy);
  msg.add(gz);
  Udp.beginPacket(ip, outPort);
  msg.send(Udp);
  Udp.endPacket();
  msg.empty();
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Wire.begin();



  // WiFi
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  server.on("/enable", onEnable);
  server.on("/disable", onDisable);
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(SSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Gyro
  Serial.println("Initialize MPU");
  mpu.initialize();
  Serial.println(mpu.testConnection() ? "Connected" : "Connection failed");

  // Start mDNS
  if (MDNS.begin(name)) { Serial.println("MDNS responder started"); }

  // Start Udp
  Serial.println("Begin UDP");
  Udp.begin(localPort);



  server.on("/", handleRoot);
  server.on("/enable", onEnable);
  server.on("/disable", onDisable);

  server.onNotFound(handleNotFound);

  // Start server
  server.begin();
}

void loop() {
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  // put your main code here, to run repeatedly:
  server.handleClient();
  MDNS.update();

  // valx = map(ax, -17000, 17000, 0, 179);
  // valy = map(ay, -17000, 17000, 0, 179);
  // valz = map(az, -17000, 17000, 0, 179);

  for (const auto& ip : targets) {
    sendAccelerometer(ip);

    Serial.print("axis x = ");
    Serial.print(ax);
    Serial.print(" axis y = ");
    Serial.print(ay);
    Serial.print(" axis z = ");
    Serial.print(az);
    Serial.print(" gx = ");
    Serial.print(gx);
    Serial.print(" gy = ");
    Serial.print(gy);
    Serial.print(" gz = ");
    Serial.print(gz);
    Serial.print(" -> ");
    Serial.print(ip.toString().c_str());
    Serial.println();
    if(!mpu.testConnection()) {
      mpu.initialize();
      Serial.println("reconnect");      
    }
  }
}
