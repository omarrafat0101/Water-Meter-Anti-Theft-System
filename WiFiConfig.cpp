// WiFiConfig.cpp
#include "WiFiConfig.h"

// Initialize static member variables
bool WiFiManager::_isAPMode = false;
bool WiFiManager::_isConnected = false;
String WiFiManager::_lastError = "";
uint8_t WiFiManager::_connectionAttempts = 0;
uint32_t WiFiManager::_lastConnectAttempt = 0;
uint32_t WiFiManager::_apStartTime = 0;
uint8_t WiFiManager::_buildingNumber = 0;
bool WiFiManager::_isReconnecting = false;
uint32_t WiFiManager::_reconnectionStartTime = 0;
bool WiFiManager::_isInitializing = false;
bool WiFiManager::_isSystemReady = false;
bool WiFiManager::_dualModeActive = false;
uint32_t WiFiManager::_lastReconnectAttempt = 0;

// Initialize callback function pointers
void (*WiFiManager::_onConnectCallback)() = nullptr;
void (*WiFiManager::_onDisconnectCallback)() = nullptr;
void (*WiFiManager::_onAPStartCallback)() = nullptr;
void (*WiFiManager::_onAPStopCallback)() = nullptr;

// Define constants
const char *WiFiManager::PREFERENCE_NAMESPACE = "wifi-config";
const char *WiFiManager::SSID_KEY = "ssid";
const char *WiFiManager::PASSWORD_KEY = "password";
const char *WiFiManager::BUILDING_NUMBER_KEY = "bldg-num";
const char *WiFiManager::DEFAULT_AP_PASSWORD = "WaterMeterSecurity";
const char *WiFiManager::DEVICE_HOSTNAME = "water-meter-security";

// Create an instance of WiFiManager
WiFiManager wifiManager;

// Implementation of public methods
bool WiFiManager::begin()
{
    _isInitializing = true;
    _isSystemReady = false;

    // Initialize WiFi
    WiFi.mode(WIFI_OFF);
    delay(100);

    initializePreferences();
    _buildingNumber = static_cast<uint8_t>(loadFromPreferences(BUILDING_NUMBER_KEY).toInt());
    WiFi.onEvent(onWiFiEvent);
    WiFi.setSleep(false); // Disable WiFi sleep mode

    // Start connection attempt without waiting
    String ssid, password;
    loadWiFiCredentials(ssid, password);

    // If credentials are available, attempt to connect
    if (!ssid.isEmpty())
    {
        WiFi.setAutoReconnect(true);
        WiFi.begin(ssid.c_str(), password.c_str());
        Serial.printf("Starting connection to: %s\n", ssid.c_str());
    }

    // Start AP Mode anyway
    startAPMode(true);


    _isInitializing = false;
    return true; // Return true since initialization started
}

void WiFiManager::end()
{
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    _isConnected = false;
    _isAPMode = false;
}

bool WiFiManager::isConnected()
{
    return _isConnected;
}

bool WiFiManager::isInAPMode()
{
    return _isAPMode;
}

bool WiFiManager::isSystemReady()
{
    return _isSystemReady;
}

String WiFiManager::getLocalIP()
{
    if (_isConnected)
    {
        return WiFi.localIP().toString();
    }
    return "";
}

String WiFiManager::getAPIP()
{
    if (_isAPMode)
    {
        return WiFi.softAPIP().toString();
    }
    return "";
}

int32_t WiFiManager::getRSSI()
{
    if (_isConnected)
    {
        return WiFi.RSSI();
    }
    return 0;
}

String WiFiManager::getCurrentSSID()
{
    if (_isConnected)
    {
        return WiFi.SSID();
    }
    return "";
}

void WiFiManager::handleConnection()
{
    if (!_isConnected)
    {
        // Start AP mode if not already active
        if (!_isAPMode)
        {
            startAPMode(true); // Start AP mode with dual mode flag
        }

        // Attempt reconnection periodically without blocking
        if (!_isReconnecting && millis() - _lastReconnectAttempt >= WIFI_RETRY_DELAY)
        {
            _lastReconnectAttempt = millis();
            _isReconnecting = true;
            String ssid, password;
            loadWiFiCredentials(ssid, password);
            if (!ssid.isEmpty())
            {
                WiFi.begin(ssid.c_str(), password.c_str());
            }
        }
    }
    else
    {
        _isReconnecting = false;
    }
}

bool WiFiManager::reconnect()
{

    if (_isReconnecting)
    {
        // Already attempting to reconnect
        return true;
    }

    _lastConnectAttempt = millis();
    _reconnectionStartTime = _lastConnectAttempt;
    _connectionAttempts++;
    _isReconnecting = true;
    _isConnected = false;

    String ssid, password;
    loadWiFiCredentials(ssid, password);

    if (ssid.isEmpty())
    {
        _lastError = "No saved credentials";
        _isReconnecting = false;
        return false;
    }

    Serial.printf("Reconnecting to WiFi: %s (Attempt %d)\n", ssid.c_str(), _connectionAttempts);
    
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(100); // Brief delay to ensure clean disconnect
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
    WiFi.begin(ssid.c_str(), password.c_str());

    return true;
}

void WiFiManager::disconnect()
{
    WiFi.disconnect();
    _isConnected = false;
    if (_onDisconnectCallback != nullptr)
    {
        _onDisconnectCallback();
    }
}

void WiFiManager::startAPMode(bool dualMode)
{
    _dualModeActive = dualMode;
    if (!_isAPMode)
    {
        // Set appropriate WiFi mode
        WiFi.mode(dualMode ? WIFI_AP_STA : WIFI_AP);
        delay(100); // Brief delay to ensure mode change

        // Configure AP
        WiFi.softAP(getAPSSID().c_str(), DEFAULT_AP_PASSWORD);
        _isAPMode = true;
        _apStartTime = millis();

        Serial.printf("AP Started: %s\n", getAPSSID().c_str());

        if (_onAPStartCallback)
        {
            _onAPStartCallback();
        }
    }
}

void WiFiManager::stopAPMode()
{
    if (!_isAPMode)
    {
        return; // Not in AP mode
    }

    WiFi.softAPdisconnect(true);
    _isAPMode = false;

    if (_onAPStopCallback != nullptr)
    {
        _onAPStopCallback();
    }
}

String WiFiManager::getAPSSID()
{
    return String("WaterMeterSecurity Building ") + String(_buildingNumber);
}

bool WiFiManager::saveWiFiCredentials(const String &ssid, const String &password)
{
    if (!validateCredentials(ssid, password))
    {
        _lastError = "Invalid credentials";
        return false;
    }

    saveToPreferences(SSID_KEY, ssid);
    saveToPreferences(PASSWORD_KEY, password);

    return true;
}

bool WiFiManager::clearWiFiCredentials()
{
    Preferences preferences;
    preferences.begin(PREFERENCE_NAMESPACE, false);

    preferences.remove(SSID_KEY);
    preferences.remove(PASSWORD_KEY);

    preferences.end();
    return true;
}

bool WiFiManager::hasStoredCredentials()
{
    String ssid = loadFromPreferences(SSID_KEY);
    return !ssid.isEmpty();
}

void WiFiManager::setBuildingNumber(uint8_t number)
{
    _buildingNumber = number;
}

uint8_t WiFiManager::getBuildingNumber()
{
    return _buildingNumber;
}

bool WiFiManager::saveBuildingNumber(uint8_t number)
{
    _buildingNumber = number;
    saveToPreferences(BUILDING_NUMBER_KEY, String(number));
    return true;
}

int16_t WiFiManager::scanNetworks()
{
    // Save current WiFi mode
    wifi_mode_t currentMode = WiFi.getMode();
    
    // Set to AP+STA mode to scan if not already in a mode that allows scanning
    if (currentMode == WIFI_OFF || currentMode == WIFI_AP) {
        WiFi.mode(WIFI_AP_STA);
        delay(100); // Brief delay to ensure mode change applies
    }
    
    Serial.println("Starting WiFi scan...");
    int16_t numNetworks = WiFi.scanNetworks(false, true); // Async = false, show hidden = true
    Serial.printf("Scan completed, found %d networks\n", numNetworks);
    
    // Restore original mode if we changed it
    if (currentMode != WiFi.getMode() && (currentMode == WIFI_OFF || currentMode == WIFI_AP)) {
        WiFi.mode(currentMode);
    }
    
    return numNetworks;
}

String WiFiManager::getScannedSSID(uint8_t index)
{
    return WiFi.SSID(index);
}

int32_t WiFiManager::getScannedRSSI(uint8_t index)
{
    return WiFi.RSSI(index);
}

String WiFiManager::getScannedEncryption(uint8_t index)
{
    switch (WiFi.encryptionType(index))
    {
    case WIFI_AUTH_OPEN:
        return "Open";
    case WIFI_AUTH_WEP:
        return "WEP";
    case WIFI_AUTH_WPA_PSK:
        return "WPA";
    case WIFI_AUTH_WPA2_PSK:
        return "WPA2";
    case WIFI_AUTH_WPA_WPA2_PSK:
        return "WPA/WPA2";
    case WIFI_AUTH_WPA2_ENTERPRISE:
        return "WPA2-Enterprise";
    default:
        return "Unknown";
    }
}

void WiFiManager::setOnConnectCallback(void (*callback)())
{
    _onConnectCallback = callback;
}

void WiFiManager::setOnDisconnectCallback(void (*callback)())
{
    _onDisconnectCallback = callback;
}

void WiFiManager::setOnAPStartCallback(void (*callback)())
{
    _onAPStartCallback = callback;
}

void WiFiManager::setOnAPStopCallback(void (*callback)())
{
    _onAPStopCallback = callback;
}

String WiFiManager::getStatusString()
{
    if (_isConnected)
    {
        return "Connected to " + WiFi.SSID() + " (" + String(WiFi.RSSI()) + " dBm)";
    }
    else if (_isAPMode)
    {
        return "AP Mode: " + getAPSSID();
    }
    else
    {
        return "Disconnected";
    }
}

String WiFiManager::getLastError()
{
    return _lastError;
}

uint32_t WiFiManager::getUptime()
{
    return millis();
}

uint32_t WiFiManager::getLastConnectTime()
{
    return _lastConnectAttempt;
}

uint8_t WiFiManager::getConnectionAttempts()
{
    return _connectionAttempts;
}

// Implementation of private methods
bool WiFiManager::connectToSavedNetwork()
{
    String ssid, password;
    loadWiFiCredentials(ssid, password);

    if (ssid.isEmpty())
    {
        _lastError = "No saved credentials";
        return false;
    }

    Serial.print("Connecting to WiFi: ");
    Serial.println(ssid);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());

    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < WIFI_CONNECT_TIMEOUT)
    {
        delay(100);
    }

    _isConnected = (WiFi.status() == WL_CONNECTED);

    if (_isConnected)
    {
        _connectionAttempts = 0;
        _lastConnectAttempt = millis();
        logConnectionDetails();
        if (_onConnectCallback != nullptr)
        {
            _onConnectCallback();
        }
        return true;
    }

    _lastError = "Failed to connect to " + ssid;
    Serial.println(_lastError);
    return false;
}

void WiFiManager::loadWiFiCredentials(String &ssid, String &password)
{
    ssid = loadFromPreferences(SSID_KEY);
    password = loadFromPreferences(PASSWORD_KEY);
}

void WiFiManager::initializePreferences()
{
    Preferences preferences;
    if (!preferences.begin(PREFERENCE_NAMESPACE, false))
    {
        Serial.println("Failed to initialize Preferences");
    };

    // If first run, initialize with defaults
    if (!preferences.isKey(BUILDING_NUMBER_KEY))
    {
        preferences.putUInt(BUILDING_NUMBER_KEY, 0);
    }

    preferences.end();
}

void WiFiManager::setupMDNS()
{
    String hostname = DEVICE_HOSTNAME;
    if (_buildingNumber > 0)
    {
        hostname += "-building-" + String(_buildingNumber);
    }

    if (!MDNS.begin(hostname.c_str()))
    {
        Serial.println("Error setting up MDNS responder!");
    }
    else
    {
        MDNS.addService("http", "tcp", 80);
        Serial.println("MDNS responder started");
    }
}

void WiFiManager::generateAPSSID()
{
    // Not needed as we're using building number
}

bool WiFiManager::validateCredentials(const String &ssid, const String &password)
{
    if (ssid.isEmpty())
    {
        return false;
    }

    // Check password length for WPA networks
    if (password.length() > 0 && password.length() < 8)
    {
        return false;
    }

    return true;
}

void WiFiManager::logConnectionDetails()
{
    Serial.println("WiFi connected");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Subnet Mask: ");
    Serial.println(WiFi.subnetMask());
    Serial.print("Gateway IP: ");
    Serial.println(WiFi.gatewayIP());
    Serial.print("DNS: ");
    Serial.println(WiFi.dnsIP());
    Serial.print("RSSI: ");
    Serial.println(WiFi.RSSI());
}

void WiFiManager::saveToPreferences(const char *key, const String &value)
{
    Preferences preferences;
    preferences.begin(PREFERENCE_NAMESPACE, false);
    preferences.putString(key, value);
    preferences.end();
}

String WiFiManager::loadFromPreferences(const char *key)
{
    Preferences preferences;
    preferences.begin(PREFERENCE_NAMESPACE, false);
    String value = preferences.getString(key, "");
    preferences.end();
    return value;
}

void WiFiManager::onWiFiEvent(WiFiEvent_t event)
{
    static bool isStaConnected = false;

    switch (event)
    {
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        _isConnected = false;
        isStaConnected = false;
        _isReconnecting = false;
        
        // Start AP mode when WiFi disconnects, if not already in AP mode
        if (!_isAPMode) {
            Serial.println("WiFi disconnected. Starting AP mode.");
            startAPMode(true); // Start AP mode with dual mode flag to allow reconnection
        }
        
        if (_onDisconnectCallback)
        {
            _onDisconnectCallback();
        }
        break;

    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
        isStaConnected = true;
        break;

    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        if (isStaConnected) {
            Serial.println("WiFi got IP");
            WiFiManager::_isConnected = true;
            WiFiManager::_isSystemReady = true;
            WiFiManager::setupMDNS();
            _isConnected = true;
            _isReconnecting = false;


            if (WiFiManager::_onConnectCallback != nullptr)
            {
                WiFiManager::_onConnectCallback();
            }
        }
        break;

    default:
        break;
    }
}
