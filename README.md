# Enterprise RTOS Network Watchdog

![C++](https://img.shields.io/badge/C++-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white)
![FreeRTOS](https://img.shields.io/badge/FreeRTOS-22314E?style=for-the-badge&logo=freertos&logoColor=white)
![Espressif](https://img.shields.io/badge/ESP32-E7352C?style=for-the-badge&logo=espressif&logoColor=white)
![JavaScript](https://img.shields.io/badge/JavaScript-F7DF1E?style=for-the-badge&logo=javascript&logoColor=black)

## 1. Executive Summary
This project outlines the architecture and deployment of an autonomous, highly concurrent network probing engine and API server. Built on an ESP32 microcontroller, the system leverages FreeRTOS, LittleFS, and an asynchronous REST architecture to monitor distributed IP nodes and serve live ICMP latency telemetry to a decoupled web frontend.

## 2. Problem Statement
Edge networks frequently experience silent failures. Centralized cloud monitoring systems lose visibility the moment a remote site's primary connection drops, making it difficult to differentiate between a dead router, a failed switch, or a localized ISP outage. Standard fault management solutions (e.g., PRTG, Zabbix) require heavy, expensive servers deployed at the edge site simply to run continuous ping sweeps. A lightweight, decentralized hardware solution is required to probe the network locally and maintain uptime visibility.

## 3. System Architecture


The solution is an autonomous Edge Network Watchdog that acts as both a continuous Layer 3 polling engine and an asynchronous web server. It replaces heavy C++ HTML string generation with a pure JSON REST API (`/api/data`), serving a static HTML file from partitioned onboard flash memory (`LittleFS`) and relying on client-side JavaScript to render the telemetry interface.

## 4. Implementation & Methodology
The architecture heavily utilizes FreeRTOS to prevent thread blocking and ensure high availability. The processor's core logic is divided into distinct tasks:
* **Background Hardware Thread:** Continuously loops through an array of distributed IP nodes, transmitting ICMP pings and updating a shared C++ struct with real-time latency metrics and timeout states.
* **Foreground Web Server:** An asynchronous web server serves a static frontend utilizing dark-mode CSS. The frontend JavaScript autonomously polls the ESP32's REST API endpoint, dynamically generating the data table in the user's browser without fragmenting the microcontroller's heap memory.
* **Non-Blocking Execution:** Even if 100% of the target nodes go offline—causing massive consecutive ICMP timeouts in the background—the FreeRTOS scheduler ensures the web server thread remains completely unblocked and instantly responsive to HTTP requests.

## 5. Getting Started

### Hardware Requirements
* ESP32 Development Board (e.g., NodeMCU ESP32)
* Local Wi-Fi Network

### Step 1: Configuration
1. Clone this repository and open it using VS Code with the **PlatformIO** extension.
2. Open `src/main.cpp`.
3. Locate the Wi-Fi configuration section and replace `YOUR_WIFI_SSID` and `YOUR_WIFI_PASSWORD` with your local network credentials.
4. Modify the `targetIPs` array to include the specific local or external IP addresses you wish to monitor.

### Step 2: Flash the Filesystem (LittleFS)
Before flashing the firmware, you must upload the static HTML/JS/CSS files to the ESP32's flash memory partition:
1. Click the PlatformIO icon (alien head) in the VS Code sidebar.
2. Under your board environment, expand **Platform**.
3. Click **Build Filesystem Image** followed by **Upload Filesystem Image**.

### Step 3: Flash the Firmware
1. Once the filesystem is uploaded, click the standard **Upload** arrow in the PlatformIO bottom toolbar to compile and flash the `main.cpp` firmware.
2. Open the Serial Monitor (`baud 115200`) to view the ESP32's assigned IP address.
3. Open a web browser and navigate to that IP address to view the live dashboard.

## 6. Limitations & Future Scope
Currently, target IPs are hardcoded into the C++ firmware array, requiring a physical re-flash to alter the monitoring fleet. Additionally, the ESP32's lightweight TCP/IP stack limits concurrent web clients, positioning it as a localized diagnostic tool rather than a mass-facing server. Moving forward, this device will act as a reliable edge telemetry source for downstream data analytics, allowing external polling engines to query the Watchdog's JSON API on a schedule to track historical SLAs.