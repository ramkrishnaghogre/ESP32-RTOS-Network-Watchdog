#include <Arduino.h>
#include <WiFi.h>
#include <ESP32Ping.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>

const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

AsyncWebServer server(80);

// --- Define the Structure for a Network Target ---
struct NetworkTarget {
  String name;
  IPAddress ip;
  int latency; // -1 means offline
};

// --- Array of Targets (You can expand this up to 50+ if you want!) ---
// --- Array of 50 Targets ---
NetworkTarget targets[] = {
  // 1-10: The Heavy Hitters (Global DNS)
  {"Google DNS Primary", IPAddress(8, 8, 8, 8), -1},
  {"Google DNS Secondary", IPAddress(8, 8, 4, 4), -1},
  {"Cloudflare Primary", IPAddress(1, 1, 1, 1), -1},
  {"Cloudflare Secondary", IPAddress(1, 0, 0, 1), -1},
  {"Quad9 Security Pri", IPAddress(9, 9, 9, 9), -1},
  {"Quad9 Security Sec", IPAddress(149, 112, 112, 112), -1},
  {"OpenDNS Home Pri", IPAddress(208, 67, 222, 222), -1},
  {"OpenDNS Home Sec", IPAddress(208, 67, 220, 220), -1},
  {"Level3/Lumen Pri", IPAddress(4, 2, 2, 1), -1},
  {"Level3/Lumen Sec", IPAddress(4, 2, 2, 2), -1},

  // 11-20: Privacy & Ad-Blocking DNS
  {"AdGuard DNS Pri", IPAddress(94, 140, 14, 14), -1},
  {"AdGuard DNS Sec", IPAddress(94, 140, 15, 15), -1},
  {"Mullvad Privacy DNS", IPAddress(194, 242, 2, 2), -1},
  {"Control D", IPAddress(76, 76, 2, 0), -1},
  {"CleanBrowsing Pri", IPAddress(185, 228, 168, 9), -1},
  {"CleanBrowsing Sec", IPAddress(185, 228, 169, 9), -1},
  {"Alternate DNS Pri", IPAddress(76, 76, 19, 19), -1},
  {"Alternate DNS Sec", IPAddress(76, 223, 122, 150), -1},
  {"Yandex DNS Pri", IPAddress(77, 88, 8, 8), -1},
  {"Yandex DNS Sec", IPAddress(77, 88, 8, 1), -1},

  // 21-30: Enterprise & Commercial DNS
  {"Comodo Secure Pri", IPAddress(8, 26, 56, 26), -1},
  {"Comodo Secure Sec", IPAddress(8, 20, 247, 20), -1},
  {"Verisign DNS Pri", IPAddress(64, 6, 64, 6), -1},
  {"Verisign DNS Sec", IPAddress(64, 6, 65, 6), -1},
  {"Neustar UltraDNS", IPAddress(156, 154, 70, 1), -1},
  {"SafeDNS Primary", IPAddress(195, 46, 39, 39), -1},
  {"DNS.Watch Primary", IPAddress(84, 200, 69, 80), -1},
  {"DNS.Watch Secondary", IPAddress(84, 200, 70, 40), -1},
  {"Freenom World Pri", IPAddress(80, 80, 80, 80), -1},
  {"Freenom World Sec", IPAddress(80, 80, 81, 81), -1},

  // 31-40: Internet Root Servers & Infrastructure
  {"A Root Server (Verisign)", IPAddress(198, 41, 0, 4), -1},
  {"B Root Server (USC/ISI)", IPAddress(199, 9, 14, 201), -1},
  {"C Root Server (Cogent)", IPAddress(192, 33, 4, 12), -1},
  {"D Root Server (UMD)", IPAddress(199, 7, 91, 13), -1},
  {"E Root Server (NASA)", IPAddress(192, 203, 230, 10), -1},
  {"F Root Server (ISC)", IPAddress(192, 5, 5, 241), -1},
  {"G Root Server (DoD)", IPAddress(192, 112, 36, 4), -1},
  {"H Root Server (ARL)", IPAddress(198, 97, 190, 53), -1},
  {"I Root Server (Netnod)", IPAddress(192, 36, 148, 17), -1},
  {"Hurricane Electric", IPAddress(74, 82, 42, 42), -1},

  // 41-50: Local Network & Simulated Failures
  {"Local Router/Gateway", IPAddress(192, 168, 1, 1), -1},
  {"Local Switch (Simulated)", IPAddress(192, 168, 1, 2), -1},
  {"Network Attached Storage", IPAddress(192, 168, 1, 100), -1},
  {"Smart Home IoT Hub", IPAddress(192, 168, 1, 150), -1},
  {"Security Camera 1", IPAddress(192, 168, 1, 201), -1},
  {"Security Camera 2", IPAddress(192, 168, 1, 202), -1},
  {"Internal Database Server", IPAddress(10, 0, 0, 5), -1},
  {"Dead IP (Test-Net-1)", IPAddress(192, 0, 2, 1), -1},      // Guaranteed timeout
  {"Dead IP (Test-Net-2)", IPAddress(198, 51, 100, 1), -1},   // Guaranteed timeout
  {"Dead IP (Test-Net-3)", IPAddress(203, 0, 113, 1), -1}     // Guaranteed timeout
};

// Calculate exactly how many targets are in the array automatically
const int NUM_TARGETS = sizeof(targets) / sizeof(targets[0]);

// --- The HTML Template Processor ---
String processor(const String& var) {
  if (var == "TABLE_ROWS") {
    String html = "";
    
    // Loop through every target and generate an HTML row
    for (int i = 0; i < NUM_TARGETS; i++) {
      String statusClass = (targets[i].latency != -1) ? "online" : "offline";
      String statusText = (targets[i].latency != -1) ? "ONLINE" : "OFFLINE";
      String pingText = (targets[i].latency != -1) ? String(targets[i].latency) + " ms" : "Timeout";

      html += "<tr>";
      html += "<td>" + targets[i].name + "</td>";
      html += "<td>" + targets[i].ip.toString() + "</td>";
      html += "<td class='" + statusClass + "'>" + statusText + "</td>";
      html += "<td>" + pingText + "</td>";
      html += "</tr>";
    }
    return html;
  }
  return String();
}


// FreeRTOS Ping Task
void pingTask(void *pvParameters) {
  for (;;) { 
    for (int i = 0; i < NUM_TARGETS; i++) {
      bool success = Ping.ping(targets[i].ip, 1);
      
      if (success) {
        targets[i].latency = (int)Ping.averageTime();
      } else {
        targets[i].latency = -1; 
      }
      
      // Increased delay to 250ms to give the Wi-Fi stack plenty of breathing room
      vTaskDelay(250 / portTICK_PERIOD_MS); 
    }
    
    vTaskDelay(2000 / portTICK_PERIOD_MS); 
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  if (!LittleFS.begin(true)) {
    Serial.println("Error mounting LittleFS");
    return;
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  
  Serial.println("\nSUCCESS! Connected to Wi-Fi.");
  Serial.print("ESP32 IP Address: ");
  Serial.println(WiFi.localIP());

  xTaskCreate(pingTask, "Ping_Task", 4096, NULL, 1, NULL);

  // Endpoint 1: Serve the static HTML file
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/index.html", "text/html");
  });

  // Endpoint 2: The REST API (Returns pure JSON data)
  server.on("/api/data", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = "[";
    for (int i = 0; i < NUM_TARGETS; i++) {
      json += "{\"name\":\"" + targets[i].name + "\",\"ip\":\"" + targets[i].ip.toString() + "\",\"latency\":" + String(targets[i].latency) + "}";
      if (i < NUM_TARGETS - 1) json += ",";
    }
    json += "]";
    
    request->send(200, "application/json", json);
  });

  server.begin();
}

void loop() {
  vTaskDelay(1000 / portTICK_PERIOD_MS); 
}