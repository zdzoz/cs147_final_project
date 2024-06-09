#include <Arduino.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>

#define DOOR_SENSOR_PIN 2

volatile int currentDoorState;
volatile int lastDoorState;

#define LED 12

const char* ssid = "DoorCounter";
const char* password = "password123";

WebServer server(80);
Preferences preferences;
HTTPClient http;

void handleRoot()
{
    String html = R"=====(
  <!DOCTYPE html>
  <html lang="en">
  <head>
      <meta charset="UTF-8">
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
      <title>WiFi Setup</title>
      <style>
          body {
              font-family: Arial, sans-serif;
              display: flex;
              justify-content: center;
              align-items: center;
              height: 100vh;
              margin: 0;
              background-color: #f0f0f0;
          }
          .form-container {
              background-color: #ffffff;
              padding: 20px;
              border-radius: 8px;
              box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
          }
          .form-container h2 {
              margin-top: 0;
              margin-bottom: 20px;
          }
          .form-container label {
              display: block;
              margin-bottom: 5px;
          }
          .form-container input {
              width: 100%;
              padding: 8px;
              margin-bottom: 15px;
              border: 1px solid #ccc;
              border-radius: 4px;
          }
          .form-container button {
              width: 100%;
              padding: 10px;
              background-color: #007BFF;
              border: none;
              border-radius: 4px;
              color: #ffffff;
              font-size: 16px;
              cursor: pointer;
          }
          .form-container button:hover {
              background-color: #0056b3;
          }
      </style>
  </head>
  <body>

      <div class="form-container">
          <h2>WiFi Setup</h2>
          <form action="/submit_wifi_credentials" method="POST">
              <label for="ssid">SSID:</label>
              <input type="text" id="ssid" name="ssid" required>
              
              <label for="password">Password:</label>
              <input type="password" id="password" name="password" required>
              
              <button type="submit">Submit</button>
          </form>
      </div>

  </body>
  </html>
  )=====";

    server.send(200, "text/html", html);
}

void handleSubmit()
{
    if (server.hasArg("ssid") && server.hasArg("password")) {
        String ssid = server.arg("ssid");
        String password = server.arg("password");

        // Save the credentials to Preferences
        preferences.begin("wifi", false);
        preferences.putString("ssid", ssid);
        preferences.putString("password", password);
        preferences.end();

        // Respond to the client
        server.send(200, "text/plain", "Credentials saved. Rebooting...");

        // Give the user a moment to read the response
        delay(3000);

        // Restart the ESP32
        ESP.restart();
    } else {
        server.send(400, "text/plain", "Invalid request");
    }
}

volatile bool isConnected = false;

void setup()
{
    Serial.begin(9600);
    delay(1500);

    pinMode(DOOR_SENSOR_PIN, INPUT_PULLUP);

    // Initialize the built-in LED as an output
    pinMode(LED, OUTPUT);
    digitalWrite(LED, LOW); // Turn off the LED initially

    // Initialize Preferences
    preferences.begin("wifi", false);

    // Check if SSID and password are already stored
    String stored_ssid = preferences.getString("ssid", "");
    String stored_password = preferences.getString("password", "");

    if (stored_ssid != "" && stored_password != "") {
        // Try to connect to WiFi
        WiFi.begin(stored_ssid.c_str(), stored_password.c_str());

        Serial.print("Connecting to WiFi...");
        unsigned long startAttemptTime = millis();

        // Try to connect for 10 seconds
        while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
            delay(100);
            Serial.print(".");
        }

        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("Connected!");
            Serial.print("IP Address: ");
            Serial.println(WiFi.localIP());
            digitalWrite(LED, HIGH); // Turn on the LED to indicate success
            currentDoorState = digitalRead(DOOR_SENSOR_PIN); // read state
            isConnected = true;
            return; // Exit setup() as we are successfully connected to WiFi
        } else {
            Serial.println("Failed to connect.");
        }
    }

    // If we reached here, it means WiFi connection failed or no credentials stored
    // Start the ESP32 as an access point
    WiFi.softAP(ssid, password);

    Serial.println("Access Point started");
    Serial.print("IP Address: ");
    Serial.println(WiFi.softAPIP());

    // Set up web server routes
    server.on("/", handleRoot);
    server.on("/submit_wifi_credentials", HTTP_POST, handleSubmit);

    // Start the server
    server.begin();
    Serial.println("Web server started: ");
    Serial.print("SSID: ");
    Serial.println(ssid);
    Serial.print("PASS: ");
    Serial.println(password);
}

void updateCount()
{
    http.begin("18.117.85.193", 5000);
    http.addHeader("Content-Type", "text/plain");

    int httpResponseCode = http.POST("/");

    if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.println("HTTP Response code: " + String(httpResponseCode));
        Serial.println("Response: " + response);
    } else {
        Serial.println("Error on sending POST: " + String(httpResponseCode));
    }

    http.end();
}

volatile unsigned long blink_count = 0;
volatile int last_state = HIGH;

void loop()
{
    if (!isConnected) {
        if (millis() - blink_count > 1000) {
            if (last_state == HIGH) {
                last_state = LOW;
                digitalWrite(LED, LOW);
            } else {
                last_state = HIGH;
                digitalWrite(LED, HIGH);
            }
            blink_count = millis();
        }
        server.handleClient();
    } else {
        lastDoorState = currentDoorState; // save the last state
        currentDoorState = digitalRead(DOOR_SENSOR_PIN); // read new state

        if (lastDoorState == LOW && currentDoorState == HIGH) { // state change: LOW -> HIGH
            Serial.println("The door-opening event is detected");
            updateCount();
        } else if (lastDoorState == HIGH && currentDoorState == LOW) { // state change: HIGH -> LOW
            Serial.println("The door-closing event is detected");
        }
        delay(400);
    }
}
