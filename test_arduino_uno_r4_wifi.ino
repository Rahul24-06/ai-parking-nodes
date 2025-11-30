#include <WiFiS3.h>

const char* WIFI_SSID     = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

const char* SLOT_ID       = "A1";   // Change for each node: A2, A3, ...
const int   STATUS_LED_PIN = 7;     // Optional local LED

// Match this baud with what you configured in SenseCraft AI
const long  VISION_BAUD = 115200;   // Or 921600 if you set that in the module

WiFiServer server(80);

bool occupied = false;
unsigned long lastDetectionMillis = 0;
const unsigned long TIMEOUT_NO_CAR_MS = 8000;  // if no "car" for 8s -> empty

void connectToWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);

  int status = WL_IDLE_STATUS;
  while (status != WL_CONNECTED) {
    status = WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    delay(2000);
  }

  Serial.println("WiFi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void processVisionLine(String line) {
  line.trim();
  if (line.length() == 0) return;

  Serial.print("Vision data: ");
  Serial.println(line);

  if (line.indexOf("car") != -1 || line.indexOf("Car") != -1) {
    // Try to find a confidence value (naive parsing)
    float conf = 1.0; // default if not found
    int commaIndex = line.indexOf(',');
    if (commaIndex != -1 && commaIndex < line.length() - 1) {
      String confStr = line.substring(commaIndex + 1);
      confStr.trim();
      conf = confStr.toFloat();
    }

    if (conf >= 0.60) { // threshold
      occupied = true;
      lastDetectionMillis = millis();
    }
  }
}

void handleClient(WiFiClient &client) {
  // Read the request line
  String requestLine = client.readStringUntil('\r');
  client.read(); // read '\n'

  // Ignore remaining headers
  while (client.available()) {
    String headerLine = client.readStringUntil('\n');
    if (headerLine == "\r" || headerLine.length() == 1) {
      break; // end of headers
    }
  }

  bool isStatusEndpoint = requestLine.startsWith("GET /status");

  if (isStatusEndpoint) {
    // Build JSON response
    String json = "{";
    json += "\"slotId\":\"";
    json += SLOT_ID;
    json += "\",\"occupied\":";
    json += (occupied ? "true" : "false");
    json += "}";

    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.print("Content-Length: ");
    client.println(json.length());
    client.println("Connection: close");
    client.println();
    client.print(json);
  } else {
    // Simple text response for root or other paths
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/plain");
    client.println("Connection: close");
    client.println();
    client.print("Smart Parking Node ");
    client.print(SLOT_ID);
    client.print(" - Occupied: ");
    client.println(occupied ? "true" : "false");
  }
}

void setup() {
  pinMode(STATUS_LED_PIN, OUTPUT);

  Serial.begin(115200);   // Debug via USB
  while (!Serial) { ; }

  Serial1.begin(VISION_BAUD);  // Vision module UART
  delay(100);

  connectToWiFi();
  server.begin();
  Serial.println("HTTP server started on port 80");
}

void loop() {
  static String lineBuffer = "";

  while (Serial1.available() > 0) {
    char c = (char)Serial1.read();
    if (c == '\n') {
      processVisionLine(lineBuffer);
      lineBuffer = "";
    } else if (c != '\r') {
      lineBuffer += c;
    }
  }

  if (occupied && (millis() - lastDetectionMillis > TIMEOUT_NO_CAR_MS)) {
    occupied = false;
  }

  digitalWrite(STATUS_LED_PIN, occupied ? HIGH : LOW);

  WiFiClient client = server.available();
  if (client) {
    while (client.connected()) {
      if (client.available()) {
        handleClient(client);
        break;
      }
    }
    delay(1);
    client.stop();
  }
}
