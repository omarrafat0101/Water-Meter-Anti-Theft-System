// WiFiConfig.h

#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <ESPmDNS.h>

// WiFi Configuration Constants
#define WIFI_CONNECT_TIMEOUT 10000  // 10 seconds timeout for connection attempts
#define AP_CONFIG_TIMEOUT 180000    // 3 minutes timeout for AP configuration mode
#define WIFI_RETRY_DELAY 5000       // 5 seconds between connection retries
#define MAX_WIFI_RETRIES 3          // Maximum number of connection retry attempts

class WiFiManager {
public:
    // Initialization
    static bool begin();
    static void end();
    
    // Connection Status
    static bool isConnected();
    static bool isInAPMode();
    static String getLocalIP();
    static String getAPIP();
    static int32_t getRSSI();
    static String getCurrentSSID();
    
    // Connection Management
    static void handleConnection();  // Non-blocking connection handling
    static bool reconnect();         // Force reconnection attempt
    static void disconnect();
    
    // AP Mode Management
    static void startAPMode(bool dualMode = false); // Modified to support dual operation
    static void stopAPMode();
    static String getAPSSID();       // Get current AP SSID
    
    // Credentials Management
    static bool saveWiFiCredentials(const String& ssid, const String& password);
    static bool clearWiFiCredentials();
    static bool hasStoredCredentials();
    
    // Building Configuration
    static void setBuildingNumber(uint8_t number);
    static uint8_t getBuildingNumber();
    static bool saveBuildingNumber(uint8_t number);
    
    // Network Scanning
    static int16_t scanNetworks();
    static String getScannedSSID(uint8_t index);
    static int32_t getScannedRSSI(uint8_t index);
    static String getScannedEncryption(uint8_t index);
    static const char* getHostname() { return DEVICE_HOSTNAME; }
    
    // Event Callbacks
    static void setOnConnectCallback(void (*callback)());
    static void setOnDisconnectCallback(void (*callback)());
    static void setOnAPStartCallback(void (*callback)());
    static void setOnAPStopCallback(void (*callback)());
    
    // Status Information
    static String getStatusString();
    static String getLastError();
    static uint32_t getUptime();
    static uint32_t getLastConnectTime();
    static uint8_t getConnectionAttempts();

    static bool isSystemReady();

private:
    // Private member variables
    static bool _isAPMode;
    static bool _isConnected;
    static String _lastError;
    static uint8_t _connectionAttempts;
    static uint32_t _lastConnectAttempt;
    static uint32_t _apStartTime;
    static uint8_t _buildingNumber;
    static bool _isReconnecting;
    static uint32_t _reconnectionStartTime;
    static bool _isInitializing;
    static bool _isSystemReady;
    static bool _dualModeActive;  // New variable to track dual mode
    static uint32_t _lastReconnectAttempt;  // Track last reconnection attempt

    // Callback function pointers
    static void (*_onConnectCallback)();
    static void (*_onDisconnectCallback)();
    static void (*_onAPStartCallback)();
    static void (*_onAPStopCallback)();
    
    // Private methods
    static bool connectToSavedNetwork();
    static void loadWiFiCredentials(String& ssid, String& password);
    static void initializePreferences();
    static void setupMDNS();
    static void updateConnectionStatus();
    static void generateAPSSID();
    static bool validateCredentials(const String& ssid, const String& password);
    static void logConnectionDetails();
    static void handleDualMode(); // New private method for dual mode handling
    
    // EEPROM/Preferences helpers
    static void saveToPreferences(const char* key, const String& value);
    static String loadFromPreferences(const char* key);
    
    // Event handlers
    static void onWiFiEvent(WiFiEvent_t event);
    
    // Constants
    static const char* PREFERENCE_NAMESPACE;
    static const char* SSID_KEY;
    static const char* PASSWORD_KEY;
    static const char* BUILDING_NUMBER_KEY;
    static const char* DEFAULT_AP_PASSWORD;
    static const char* DEVICE_HOSTNAME;
};

// External declarations for use in other files
extern WiFiManager wifiManager;

#endif // WIFI_CONFIG_H