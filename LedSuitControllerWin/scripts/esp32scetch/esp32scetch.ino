#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <vector>

// Replace with your network credentials
const char* ssid = "LR";
const char* password = "g6d9Df9x";

// Static IP configuration
IPAddress localIP(192, 168, 10, 51);       // Static IP address
IPAddress gateway(192, 168, 10, 1);        // Gateway (usually your router's IP)
IPAddress subnet(255, 255, 255, 0);       // Subnet mask

// NTP server configuration
const char* ntpServer = "192.168.10.180";
const long gmtOffset_sec = 0;             // UTC offset in seconds
const int daylightOffset_sec = 0;         // No daylight saving time

// TCP server configuration
const uint16_t tcpPort = 12345;
WiFiServer tcpServer(tcpPort);

// Buffer to store incoming data
const size_t bufferSize = 1024;
uint8_t dataBuffer[bufferSize];
size_t dataLength = 0;
// Pin configuration
const int ledPins[6] = {14, 15, 32, 27, 33, 12};


// Waypoint structure
struct Waypoint {
    uint32_t timestamp; // Timestamp in ms from T_start
    uint8_t state;      // Suit state (1 byte, 6 bits used)
};

// Store waypoints
std::vector<Waypoint> waypoints;

// Synchronization variables
uint64_t T_start = 0;              // Start time from the application
size_t nextWaypointIndex = 0;      // Index of the next waypoint to activate

// NTP synchronization
uint64_t lastNtpSyncTime = 0;      // Last NTP synchronization time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServer, gmtOffset_sec, 60000);

// Function to synchronize the ESP's internal clock with the NTP server
void synchronizeTime() {
    uint64_t startSyncTime = millis(); // Record the time before update
    bool success = timeClient.update();
    uint64_t endSyncTime = millis();   // Record the time after update

    if (success) {
        uint64_t roundTripDelay = endSyncTime - startSyncTime;
        uint64_t expectedError = roundTripDelay / 2; // Assuming symmetric latency
        uint64_t currentUnixTimeMs = (uint64_t)timeClient.getEpochTime() * 1000 + (millis() % 1000);
        lastNtpSyncTime = millis();
        Serial.printf("Time synchronized with NTP server. Current time: %llu ms since epoch.\n", currentUnixTimeMs);
        Serial.printf("Estimated round-trip delay: %llu ms\n", roundTripDelay);
        Serial.printf("Expected error: ±%llu ms\n", expectedError);
    } else {
        Serial.println("Failed to synchronize with NTP server.");
    }
}


// Function to parse waypoints from the data buffer
void parseWaypoints(const uint8_t* buffer, size_t length) {
    waypoints.clear(); // Clear existing waypoints
    nextWaypointIndex = 0; // Reset the index for playback

    for (size_t i = 0; i + 4 < length; i += 5) {
        Waypoint wp;
        wp.timestamp = (buffer[i] << 24) | (buffer[i + 1] << 16) | (buffer[i + 2] << 8) | buffer[i + 3];
        wp.state = buffer[i + 4];
        Serial.println(wp.state);
        waypoints.push_back(wp);
    }

    Serial.printf("Parsed %d waypoints.\n", waypoints.size());
}

// Function to activate LEDs based on the suit state
void activateLEDs(uint8_t state) {
    for (int i = 0; i < 6; ++i) {
        bool isActive = state & (1 << i);
        digitalWrite(ledPins[i], isActive ? HIGH : LOW);

        // Debug log for pin state
        Serial.printf("Pin %d -> %s\n", ledPins[i], isActive ? "HIGH" : "LOW");
    }
}

// Function to set T_start upon receiving the start signal from the application
void setStartTime(uint64_t receivedTStart) {
    uint64_t currentTimefake= (uint64_t)timeClient.getEpochTime() * 1000 + (millis() % 1000);
    T_start = currentTimefake + 4000;
    
    //T_start = receivedTStart;
    Serial.printf("Start signal received. T_start set to %llu ms since epoch.\n", T_start);
}

void setup() {
    // Initialize Serial for debugging
    Serial.begin(115200);

    // Initialize LED pins
    for (int i = 0; i < 6; ++i) {
        pinMode(ledPins[i], OUTPUT);
        digitalWrite(ledPins[i], LOW); // Ensure LEDs are off initially
    }

    // Configure static IP
    if (!WiFi.config(localIP, gateway, subnet)) {
        Serial.println("Failed to configure static IP!");
        while (true) {
            delay(1000);
        }
    }

    // Connect to Wi-Fi
    Serial.println("Connecting to Wi-Fi...");
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("\nConnected to Wi-Fi.");
    Serial.print("Static IP: ");
    Serial.println(WiFi.localIP());

    // Start the TCP server
    tcpServer.begin();
    Serial.printf("TCP server started on port %d\n", tcpPort);

    // Start NTP client
    timeClient.begin();
    synchronizeTime(); // Initial synchronization
}

void loop() {


    // Perform NTP synchronization every 60 seconds
    if (millis() - lastNtpSyncTime > 60000) {
        synchronizeTime();
    }

    // Check for a new client connection
    WiFiClient client = tcpServer.available();

    if (client) {
        Serial.println("Client connected.");
        dataLength = 0;

        // Read data from the client
        while (client.connected()) {
            if (client.available()) {
                // Read into buffer
                size_t bytesRead = client.read(dataBuffer + dataLength, bufferSize - dataLength);
                dataLength += bytesRead;
                Serial.println(dataLength);
                // If the first 8 bytes are received, treat them as T_start
                if (dataLength == 8) {
                    uint64_t receivedTStart = ((uint64_t)dataBuffer[0] << 56) | ((uint64_t)dataBuffer[1] << 48) |
                                              ((uint64_t)dataBuffer[2] << 40) | ((uint64_t)dataBuffer[3] << 32) |
                                              ((uint64_t)dataBuffer[4] << 24) | ((uint64_t)dataBuffer[5] << 16) |
                                              ((uint64_t)dataBuffer[6] << 8) | dataBuffer[7];
                    setStartTime(receivedTStart);
                    memmove(dataBuffer, dataBuffer + 8, dataLength - 8); // Shift remaining data
                    dataLength -= 8;
                }

                // If enough data is received for waypoints, parse them
                if (dataLength >= 5 && dataLength % 5 == 0) {
                    T_start = 0;
                    parseWaypoints(dataBuffer, dataLength);
                    dataLength = 0; // Reset the buffer
                }
            }
        }

        Serial.println("Client disconnected.");
    }

    // Process waypoints if T_start is valid
    if (T_start > 0) {
        uint64_t currentTime = (uint64_t)timeClient.getEpochTime() * 1000 + (millis() % 1000);

        // Check if there's a waypoint to activate
        if (nextWaypointIndex < waypoints.size() &&
            currentTime >= T_start + waypoints[nextWaypointIndex].timestamp) {
            Serial.printf("Activating waypoint %d at %llu ms\n", nextWaypointIndex, currentTime);
            activateLEDs(waypoints[nextWaypointIndex].state);
            nextWaypointIndex++; // Move to the next waypoint
        }
    }
}
