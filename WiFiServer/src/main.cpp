#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

// WiFi credentials
const char *ssid = "VELOSO-LRV";
const char *password = "4536271845362718";

// Static IP configuration
IPAddress local_IP(192, 168, 25, 24);  // Desired static IP address
IPAddress gateway(192, 168, 25, 1);    // Gateway address
IPAddress subnet(255, 255, 255, 0);    // Subnet mask
IPAddress primaryDNS(192, 168, 25, 1); // Primary DNS (Local)
IPAddress secondaryDNS(8, 8, 8, 8);    // Secondary DNS (Google DNS)

// Set up server on port 80
AsyncWebServer server(80);

void setup()
{
  Serial.begin(115200);

  // Configure static IP
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS))
  {
    Serial.println("STA Failed to configure");
  }

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Root endpoint
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/plain", "Welcome to the ESP32 REST Server"); });

  // Endpoint to get device status
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request)
            {
        StaticJsonDocument<1024> jsonDoc;  // Using StaticJsonDocument instead of DynamicJsonDocument
        jsonDoc["status"] = "active";
        jsonDoc["uptime"] = millis();
        
        String response;
        serializeJson(jsonDoc, response);
        request->send(200, "application/json", response); });

  // Endpoint to toggle an LED (GPIO 2 in this example)
  const int ledPin = 2;
  pinMode(ledPin, OUTPUT);
  server.on("/toggleLED", HTTP_POST, [ledPin](AsyncWebServerRequest *request)
            {
        digitalWrite(ledPin, !digitalRead(ledPin)); // Toggle LED state
        StaticJsonDocument<256> jsonResponse;
        jsonResponse["status"] = "LED toggled";

        String response;
        serializeJson(jsonResponse, response);
        request->send(200, "application/json", response); });

  // Start server
  server.begin();
}

void loop()
{
  // Nothing to do here, async server handles requests
}
