#include "WebPortal.h"
#include <Preferences.h>
#include <ESPmDNS.h>

// CSS Style definition
const char *WEB_STYLE = R"(
    <style> :root { --primary: #4361ee; --secondary: #3a0ca3; --light: #f8f9fa; --dark: #212529; --success: #4bb543; --danger: #ff3333; --warning: #ffaa00; } * { box-sizing: border-box; } body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background: linear-gradient(135deg, var(--primary), var(--secondary)); margin: 0; padding: 0; min-height: 100vh; color: var(--light); } .container { max-width: 600px; margin: 0 auto; padding: 2rem; } .card { background: rgba(255, 255, 255, 0.95); border-radius: 10px; padding: 2rem; box-shadow: 0 10px 30px rgba(0, 0, 0, 0.1); color: var(--dark); margin-bottom: 2rem; } h1 { color: white; text-align: center; margin-bottom: 1.5rem; } h2 { color: var(--secondary); margin-top: 0; } .form-group { margin-bottom: 1.5rem; } label { display: block; margin-bottom: 0.5rem; font-weight: 600; } input, select { width: 100%; padding: 0.75rem; border: 1px solid #ddd; border-radius: 4px; font-size: 1rem; } button { background: var(--primary); color: white; border: none; padding: 1rem; border-radius: 4px; cursor: pointer; font-size: 1rem; width: 100%; transition: all 0.3s; font-weight: 600; margin-top: 0.5rem; } button:hover { background: var(--secondary); transform: translateY(-2px); } button.warning { background: var(--warning); } button.warning:hover { background: #e69500; } .status { padding: 1rem; border-radius: 4px; margin: 1rem 0; text-align: center; } .success { background-color: rgba(75, 181, 67, 0.2); color: var(--success); } .error { background-color: rgba(255, 51, 51, 0.2); color: var(--danger); } .info { background-color: rgba(0, 123, 255, 0.2); color: #0069d9; } .hidden { display: none; } .network-list { margin: 1rem 0; } .network-item { padding: 0.75rem; border: 1px solid #ddd; border-radius: 4px; margin-bottom: 0.5rem; cursor: pointer; transition: all 0.2s; } .network-item:hover { background-color: #f5f5f5; } .network-item.active { border-color: var(--primary); background-color: rgba(67, 97, 238, 0.1); } .signal-strength { float: right; color: #666; } .device-info { background-color: rgba(0, 0, 0, 0.05); padding: 1rem; border-radius: 4px; margin-bottom: 1rem; } .password-container { position: relative; } .password-toggle { position: absolute; right: 10px; top: 50%; transform: translateY(-50%); cursor: pointer; } </style>
    )";

// Initialize static variables
WebServer WebPortal::_server(WEB_SERVER_PORT);
String WebPortal::_adminPassword = "admin123";     // Default admin password
String WebPortal::_wiFiConfigPassword = "user123"; // Default empty WiFi config password
Session WebPortal::_adminSession = {"", AuthLevel::ADMIN, 0};
Session WebPortal::_wiFiConfigSession = {"", AuthLevel::WIFI_CONFIG, 0};
bool WebPortal::_initialized = false;
String WebPortal::_lastError = "";
uint32_t WebPortal::_startTime = 0;
uint32_t WebPortal::_pageRequests = 0;
uint32_t WebPortal::_lastClientActivity = 0;
void (*WebPortal::_onStatusUpdateCallback)() = nullptr;
void (*WebPortal::_onAdminLoginCallback)() = nullptr;
void (*WebPortal::_onAdminLogoutCallback)() = nullptr;
void (*WebPortal::_onWiFiConfigStartCallback)() = nullptr;
void (*WebPortal::_onWiFiConfigSaveCallback)() = nullptr;

// Constants
const char *WebPortal::PREFERENCE_NAMESPACE = "webPortal";
const char *WebPortal::ADMIN_PASSWORD_KEY = "adminPass";
const char *WebPortal::WIFI_CONFIG_PASSWORD_KEY = "wifiPass";

/**
 * Initialize the web portal
 */
bool WebPortal::begin()
{
    if (_initialized)
    {
        return true;
    }

    _startTime = millis();

    // Load stored settings
    Preferences prefs;
    if (prefs.begin(PREFERENCE_NAMESPACE, true))
    {
        _adminPassword = prefs.getString(ADMIN_PASSWORD_KEY, "admin123");
        _wiFiConfigPassword = prefs.getString(WIFI_CONFIG_PASSWORD_KEY, "user123");
        prefs.end();
    }

    // Set up server routes
    setupRoutes();

    // Start the server
    _server.begin();
    _initialized = true;

    // Set up mDNS responder
    if (MDNS.begin("watermeter"))
    {
        MDNS.addService("http", "tcp", WEB_SERVER_PORT);
    }

    _lastError = "";
    return true;
}

/**
 * Stop the web portal
 */
void WebPortal::end()
{
    if (!_initialized)
    {
        return;
    }

    _server.stop();
    _initialized = false;
}

/**
 * Update function to be called in the main loop
 */
void WebPortal::update()
{
    if (!_initialized)
    {
        return;
    }

    // Handle client requests
    handleClient();

    // Check for expired sessions
    checkSessionTimeouts();
}

/**
 * Handle client requests
 */
void WebPortal::handleClient()
{
    _server.handleClient();
}

/**
 * Check if the server is running
 */
bool WebPortal::isRunning()
{
    return _initialized;
}

/**
 * Get the last error message
 */
String WebPortal::getLastError()
{
    return _lastError;
}

/**
 * Set the admin password
 */
void WebPortal::setAdminPassword(const String &password)
{
    if (password.length() >= 6)
    {
        _adminPassword = password;

        // Save to preferences
        Preferences prefs;
        if (prefs.begin(PREFERENCE_NAMESPACE, false))
        {
            prefs.putString(ADMIN_PASSWORD_KEY, password);
            prefs.end();
        }
    }
}

/**
 * Get the admin password
 */
String WebPortal::getAdminPassword()
{
    return _adminPassword;
}

/**
 * Verify the admin password
 */
bool WebPortal::verifyAdminPassword(const String &password)
{
    return password == _adminPassword;
}

/**
 * Set the WiFi configuration password
 */
void WebPortal::setWiFiConfigPassword(const String &password)
{
    _wiFiConfigPassword = password;

    // Save to preferences
    Preferences prefs;
    if (prefs.begin(PREFERENCE_NAMESPACE, false))
    {
        prefs.putString(WIFI_CONFIG_PASSWORD_KEY, password);
        prefs.end();
    }
}

/**
 * Get the WiFi configuration password
 */
String WebPortal::getWiFiConfigPassword()
{
    return _wiFiConfigPassword;
}

/**
 * Verify the WiFi configuration password
 */
bool WebPortal::verifyWiFiConfigPassword(const String &password)
{
    // If no password is set, any password is accepted
    if (_wiFiConfigPassword.length() == 0)
    {
        return true;
    }
    return password == _wiFiConfigPassword;
}

/**
 * Set the building number
 */
void WebPortal::setBuildingNumber(uint8_t number)
{
    WiFiManager::setBuildingNumber(number);
}

/**
 * Get the building number
 */
uint8_t WebPortal::getBuildingNumber()
{
    return WiFiManager::getBuildingNumber();
}

/**
 * Check if user has a valid session for the required authorization level
 */
bool WebPortal::hasValidSession(AuthLevel level)
{
    if (level == AuthLevel::NONE)
    {
        return true;
    }

    if (level == AuthLevel::ADMIN)
    {
        return _adminSession.isValid();
    }

    if (level == AuthLevel::WIFI_CONFIG)
    {
        return _wiFiConfigSession.isValid() || _adminSession.isValid();
    }

    return false;
}

/**
 * Invalidate a session of specified level
 */
void WebPortal::invalidateSession(AuthLevel level)
{
    if (level == AuthLevel::ADMIN)
    {
        _adminSession.expiry = 0;
    }
    else if (level == AuthLevel::WIFI_CONFIG)
    {
        _wiFiConfigSession.expiry = 0;
    }
}

/**
 * Check for expired sessions and invalidate them
 */
void WebPortal::checkSessionTimeouts()
{
    uint32_t currentTime = millis();

    // Check admin session
    if (_adminSession.expiry != 0 && currentTime > _adminSession.expiry)
    {
        _adminSession.expiry = 0;
        if (_onAdminLogoutCallback)
        {
            _onAdminLogoutCallback();
        }
    }

    // Check WiFi config session
    if (_wiFiConfigSession.expiry != 0 && currentTime > _wiFiConfigSession.expiry)
    {
        _wiFiConfigSession.expiry = 0;
    }
}

/**
 * Get system status in JSON format
 */
String WebPortal::getSystemStatusJSON()
{
    String json = "{";

    // WiFi Status
    json += "\"wifi\":{";
    json += "\"connected\":" + String(WiFiManager::isConnected() ? "true" : "false") + ",";
    json += "\"ssid\":\"" + WiFiManager::getCurrentSSID() + "\",";
    json += "\"rssi\":" + String(WiFiManager::getRSSI()) + ",";
    json += "\"ipAddress\":\"" + WiFiManager::getLocalIP() + "\",";
    json += "\"uptime\":" + String(WiFiManager::getUptime() / 1000) + "";
    json += "},";

    // Alarm Status
    json += "\"alarm\":{";
    json += "\"status\":\"" + String(static_cast<int>(alarmSystem.getStatus())) + "\",";
    json += "\"rightSideActive\":" + String(alarmSystem.isAlarmActive(RIGHT_SIDE) ? "true" : "false") + ",";
    json += "\"leftSideActive\":" + String(alarmSystem.isAlarmActive(LEFT_SIDE) ? "true" : "false") + ",";
    json += "\"uptime\":" + String(alarmSystem.getUptime() / 1000) + ",";
    json += "\"lastAlarm\":" + String(alarmSystem.getLastAlarmTime() / 1000) + "";
    json += "},";

    // General System Status
    json += "\"system\":{";
    json += "\"buildingNumber\":" + String(getBuildingNumber()) + ",";
    json += "\"freeHeap\":" + String(ESP.getFreeHeap()) + ",";
    json += "\"cpuFreqMHz\":" + String(ESP.getCpuFreqMHz()) + ",";
    json += "\"uptime\":" + String(millis() / 1000) + "";
    json += "}";

    json += "}";
    return json;
}

/**
 * Get apartment status in JSON format
 */
String WebPortal::getApartmentStatusJSON()
{
    String json = "{\"apartments\":[";

    bool firstItem = true;
    for (int i = 0; i < TOTAL_APARTMENTS; i++)
    {
        uint8_t aptNumber = i + 1;

        if (!firstItem)
        {
            json += ",";
        }

        json += "{";
        json += "\"number\":" + String(aptNumber) + ",";
        json += "\"enabled\":" + String(alarmSystem.isSensorEnabled(aptNumber) ? "true" : "false") + ",";
        json += "\"telegramConfigured\":" + String(telegramHandler.isApartmentConfigured(aptNumber) ? "true" : "false") + ",";
        json += "\"telegramEnabled\":" + String(telegramHandler.isApartmentEnabled(aptNumber) ? "true" : "false") + ",";
        json += "\"triggered\":" + String(alarmSystem.isSensorTriggered(aptNumber) ? "true" : "false") + "";
        json += "}";

        firstItem = false;
    }

    json += "]}";
    return json;
}

/**
 * Get wire status in JSON format
 */
String WebPortal::getWireStatusJSON()
{
    WireCutStatus status = alarmSystem.getWireCutStatus();
    String json = "{";

    json += "\"rightSideRightBox\":{\"cut\":" + String(status.rightSideRightBox ? "true" : "false") +
            ",\"enabled\":" + String(status.rightSideRightBoxEnabled ? "true" : "false") + "},";

    json += "\"rightSideLeftBox\":{\"cut\":" + String(status.rightSideLeftBox ? "true" : "false") +
            ",\"enabled\":" + String(status.rightSideLeftBoxEnabled ? "true" : "false") + "},";

    json += "\"leftSideRightBox\":{\"cut\":" + String(status.leftSideRightBox ? "true" : "false") +
            ",\"enabled\":" + String(status.leftSideRightBoxEnabled ? "true" : "false") + "},";

    json += "\"leftSideLeftBox\":{\"cut\":" + String(status.leftSideLeftBox ? "true" : "false") +
            ",\"enabled\":" + String(status.leftSideLeftBoxEnabled ? "true" : "false") + "},";

    json += "\"rightSideDistribution\":{\"cut\":" + String(status.rightSideDistribution ? "true" : "false") +
            ",\"enabled\":" + String(status.rightSideDistributionEnabled ? "true" : "false") + "},";

    json += "\"leftSideDistribution\":{\"cut\":" + String(status.leftSideDistribution ? "true" : "false") +
            ",\"enabled\":" + String(status.leftSideDistributionEnabled ? "true" : "false") + "}";

    json += "}";
    return json;
}

/**
 * Get alarm status in JSON format
 */
String WebPortal::getAlarmStatusJSON()
{
    String json = "{";

    json += "\"status\":" + String(static_cast<int>(alarmSystem.getStatus())) + ",";
    json += "\"rightSideActive\":" + String(alarmSystem.isAlarmActive(RIGHT_SIDE) ? "true" : "false") + ",";
    json += "\"leftSideActive\":" + String(alarmSystem.isAlarmActive(LEFT_SIDE) ? "true" : "false") + ",";
    json += "\"lastAlarmTime\":" + String(alarmSystem.getLastAlarmTime() / 1000) + "";

    json += "}";
    return json;
}

/**
 * Check if admin is logged in
 */
bool WebPortal::isAdminLoggedIn()
{
    return _adminSession.isValid();
}

/**
 * Check if WiFi config is active
 */
bool WebPortal::isWiFiConfigActive()
{
    return _wiFiConfigSession.isValid();
}

/**
 * Set callback for system status update
 */
void WebPortal::onSystemStatusUpdate(void (*callback)())
{
    _onStatusUpdateCallback = callback;
}

/**
 * Set callback for admin login
 */
void WebPortal::onAdminLogin(void (*callback)())
{
    _onAdminLoginCallback = callback;
}

/**
 * Set callback for admin logout
 */
void WebPortal::onAdminLogout(void (*callback)())
{
    _onAdminLogoutCallback = callback;
}

/**
 * Set callback for WiFi config start
 */
void WebPortal::onWiFiConfigStart(void (*callback)())
{
    _onWiFiConfigStartCallback = callback;
}

/**
 * Set callback for WiFi config save
 */
void WebPortal::onWiFiConfigSave(void (*callback)())
{
    _onWiFiConfigSaveCallback = callback;
}

/**
 * Setup all server routes
 */
void WebPortal::setupRoutes()
{
    // Main routes
    _server.on(ROUTE_ROOT, HTTP_GET, []()
               { WebPortal::handleRoot(); });
    _server.on(ROUTE_DASHBOARD, HTTP_GET, []()
               { WebPortal::handleDashboard(); });
    _server.on(ROUTE_WIFI_CONFIG, HTTP_GET, []()
               { WebPortal::handleWiFiConfigPage(); });
    _server.on(ROUTE_WIFI_CONFIG, HTTP_POST, []()
               { WebPortal::handleWiFiConfigPage(); });
    _server.on(ROUTE_ADMIN_LOGIN, HTTP_GET, []()
               { WebPortal::handleAdminLogin(); });
    _server.on(ROUTE_ADMIN_LOGIN, HTTP_POST, []()
               { WebPortal::handleAdminLogin(); });
    _server.on(ROUTE_ADMIN_PANEL, HTTP_GET, []()
               { WebPortal::handleAdminPanel(); });
    _server.on(ROUTE_ADMIN_LOGOUT, HTTP_GET, []()
               { WebPortal::handleAdminLogout(); });

    // API endpoints
    _server.on(ROUTE_API_STATUS, HTTP_GET, []()
               { WebPortal::handleAPIStatus(); });
    _server.on(ROUTE_API_APARTMENT_STATUS, HTTP_GET, []()
               { WebPortal::handleAPIApartmentStatus(); });
    _server.on(ROUTE_API_WIRE_STATUS, HTTP_GET, []()
               { WebPortal::handleAPIWireStatus(); });
    _server.on(ROUTE_API_ALARM_STATUS, HTTP_GET, []()
               { WebPortal::handleAPIAlarmStatus(); });

    // Admin configuration pages
    _server.on(ROUTE_ADMIN_TELEGRAM_CONFIG, HTTP_GET, []()
               { WebPortal::handleAdminTelegramConfig(); });
    _server.on(ROUTE_ADMIN_BUILDING_CONFIG, HTTP_GET, []()
               { WebPortal::handleAdminBuildingConfig(); });
    _server.on(ROUTE_ADMIN_ADVANCED_CONFIG, HTTP_GET, []()
               { WebPortal::handleAdminAdvancedConfig(); });
    _server.on("/admin/get-telegram-config", HTTP_GET, []()
               { WebPortal::handleGetTelegramConfig(); });

    // Action handlers
    _server.on(ROUTE_SCAN_NETWORKS, HTTP_GET, []()
               { WebPortal::handleScanNetworks(); });
    _server.on(ROUTE_SAVE_WIFI, HTTP_POST, []()
               { WebPortal::handleSaveWiFi(); });
    _server.on(ROUTE_ADMIN_SAVE_TELEGRAM, HTTP_POST, []()
               { WebPortal::handleSaveTelegram(); });
    _server.on(ROUTE_ADMIN_SAVE_BUILDING, HTTP_POST, []()
               { WebPortal::handleSaveBuildingConfig(); });
    _server.on(ROUTE_ADMIN_SAVE_ADVANCED, HTTP_POST, []()
               { WebPortal::handleSaveAdvancedConfig(); });
    _server.on(ROUTE_ADMIN_APARTMENT_TOGGLE, HTTP_POST, []()
               { WebPortal::handleToggleApartment(); });

    // Handle not found
    _server.onNotFound([]()
                       { WebPortal::handleNotFound(); });
}

/**
 * Serve static content
 */
void WebPortal::serveStatic(const String &path, const char *contentType, const char *content)
{
    _server.send(200, contentType, content);
}

/**
 * Authenticate a user for the required level
 */
bool WebPortal::authenticate(AuthLevel requiredLevel)
{
    if (requiredLevel == AuthLevel::NONE)
    {
        return true;
    }

    // Check if already authenticated
    if (hasValidSession(requiredLevel))
    {
        updateLastActivity();
        return true;
    }

    // If authentication is required, check if credentials were provided
    if (_server.hasHeader("Cookie"))
    {
        String cookie = _server.header("Cookie");

        // Check for admin session
        if (requiredLevel == AuthLevel::ADMIN || requiredLevel == AuthLevel::WIFI_CONFIG)
        {
            int adminSessionStart = cookie.indexOf("adminSession=");
            if (adminSessionStart != -1)
            {
                adminSessionStart += 13; // Length of "adminSession="
                int adminSessionEnd = cookie.indexOf(";", adminSessionStart);
                if (adminSessionEnd == -1)
                {
                    adminSessionEnd = cookie.length();
                }
                String sessionId = cookie.substring(adminSessionStart, adminSessionEnd);
                if (validateSession(sessionId, AuthLevel::ADMIN))
                {
                    updateLastActivity();
                    return true;
                }
            }
        }

        // Check for WiFi config session (if admin session not valid)
        if (requiredLevel == AuthLevel::WIFI_CONFIG)
        {
            int wifiSessionStart = cookie.indexOf("wifiSession=");
            if (wifiSessionStart != -1)
            {
                wifiSessionStart += 12; // Length of "wifiSession="
                int wifiSessionEnd = cookie.indexOf(";", wifiSessionStart);
                if (wifiSessionEnd == -1)
                {
                    wifiSessionEnd = cookie.length();
                }
                String sessionId = cookie.substring(wifiSessionStart, wifiSessionEnd);
                if (validateSession(sessionId, AuthLevel::WIFI_CONFIG))
                {
                    updateLastActivity();
                    return true;
                }
            }
        }
    }

    // If not authenticated, redirect to appropriate login page
    if (requiredLevel == AuthLevel::ADMIN)
    {
        _server.sendHeader("Location", ROUTE_ADMIN_LOGIN);
        _server.send(302);
    }
    else if (requiredLevel == AuthLevel::WIFI_CONFIG)
    {
        _server.sendHeader("Location", ROUTE_WIFI_CONFIG);
        _server.send(302);
    }

    return false;
}

/**
 * Create a new session for the given authentication level
 */
String WebPortal::createSession(AuthLevel level)
{
    String sessionId = generateSessionId();
    uint32_t expiry = millis();

    if (level == AuthLevel::ADMIN)
    {
        expiry += ADMIN_SESSION_TIMEOUT;
        _adminSession = {sessionId, level, expiry};
    }
    else if (level == AuthLevel::WIFI_CONFIG)
    {
        expiry += WIFI_CONFIG_SESSION_TIMEOUT;
        _wiFiConfigSession = {sessionId, level, expiry};
    }

    return sessionId;
}

/**
 * Generate a random session ID
 */
String WebPortal::generateSessionId()
{
    String id = "";
    for (int i = 0; i < 32; i++)
    {
        id += "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"[random(0, 62)];
    }
    return id;
}

/**
 * Validate a session ID for the given level
 */
bool WebPortal::validateSession(const String &sessionId, AuthLevel level)
{
    if (level == AuthLevel::ADMIN)
    {
        if (_adminSession.id == sessionId && _adminSession.isValid())
        {
            // Refresh session expiry on successful validation
            _adminSession.expiry = millis() + ADMIN_SESSION_TIMEOUT;
            return true;
        }
    }
    else if (level == AuthLevel::WIFI_CONFIG)
    {
        if (_wiFiConfigSession.id == sessionId && _wiFiConfigSession.isValid())
        {
            // Refresh session expiry on successful validation
            _wiFiConfigSession.expiry = millis() + WIFI_CONFIG_SESSION_TIMEOUT;
            return true;
        }
    }

    return false;
}

/**
 * Update last activity timestamp
 */
void WebPortal::updateLastActivity()
{
    _lastClientActivity = millis();
}

/**
 * Get HTML header with title
 */
String WebPortal::getHTMLHeader(const String &title, bool includeRefresh)
{
    String header = "<!DOCTYPE html><html><head>";
    header += "<meta charset='UTF-8'>";
    header += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    header += "<title>" + title + " - Water Meter Security</title>";

    // Include auto-refresh if needed
    if (includeRefresh)
    {
        header += "<meta http-equiv='refresh' content='30'>";
    }

    // Include common CSS
    header += WEB_STYLE;

    header += "</head><body>";
    return header;
}

/**
 * Get HTML footer
 */
String WebPortal::getHTMLFooter()
{
    String footer = "<div style='margin-top: 2rem; text-align: center; font-size: 0.8rem; color: #ddd;'>";
    footer += "Water Meter Anti-Theft System &copy; " + String(2025) + " | Building " + String(getBuildingNumber());
    footer += "</div>";
    footer += "</body></html>";
    return footer;
}

/**
 * Get navigation menu HTML
 */
String WebPortal::getNavigationMenu(bool isAdmin)
{
    String menu = "<div style='margin-bottom: 1rem; display: flex; justify-content: space-between; align-items: center;'>";
    menu += "<div>";
    menu += "<a href='" + String(ROUTE_ROOT) + "' style='margin-right: 1rem; color: white; text-decoration: none;'>Home</a>";
    menu += "<a href='" + String(ROUTE_DASHBOARD) + "' style='margin-right: 1rem; color: white; text-decoration: none;'>Dashboard</a>";
    menu += "<a href='" + String(ROUTE_WIFI_CONFIG) + "' style='margin-right: 1rem; color: white; text-decoration: none;'>WiFi Settings</a>";

    if (isAdmin)
    {
        menu += "<a href='" + String(ROUTE_ADMIN_PANEL) + "' style='margin-right: 1rem; color: white; text-decoration: none;'>Admin Panel</a>";
        menu += "<a href='" + String(ROUTE_ADMIN_LOGOUT) + "' style='color: #ffaa00; text-decoration: none;'>Logout</a>";
    }
    else
    {
        menu += "<a href='" + String(ROUTE_ADMIN_LOGIN) + "' style='color: #ffaa00; text-decoration: none;'>Admin Login</a>";
    }

    menu += "</div>";

    // WiFi status indicator
    if (WiFiManager::isConnected())
    {
        menu += "<div style='background: rgba(75, 181, 67, 0.2); padding: 0.3rem 0.6rem; border-radius: 4px; color: var(--success);'>";
        menu += "Wi-Fi: Connected | RSSI: " + String(WiFiManager::getRSSI()) + " dBm";
        menu += "</div>";
    }
    else
    {
        menu += "<div style='background: rgba(255, 51, 51, 0.2); padding: 0.3rem 0.6rem; border-radius: 4px; color: var(--danger);'>";
        menu += "Wi-Fi: Disconnected | <a href='" + String(ROUTE_WIFI_CONFIG) + "' style='color: var(--danger);'>Configure</a>";
        menu += "</div>";
    }

    menu += "</div>";
    return menu;
}

/**
 * Get system status widget HTML
 */
String WebPortal::getSystemStatusWidget()
{
    String widget = "<div class='card'>";
    widget += "<h2>System Status</h2>";

    // Alarm status
    String alarmStatusClass = "success";
    String alarmStatusText = "Normal";

    switch (alarmSystem.getStatus())
    {
    case AlarmSystemStatus::NORMAL:
        alarmStatusClass = "success";
        alarmStatusText = "Normal";
        break;
    case AlarmSystemStatus::THEFT_DETECTED:
        alarmStatusClass = "error";
        alarmStatusText = "Theft Detected";
        break;
    case AlarmSystemStatus::WIRE_CUT_DETECTED:
        alarmStatusClass = "error";
        alarmStatusText = "Wire Cut Detected";
        break;
    }

    widget += "<div class='status " + alarmStatusClass + "'>" + alarmStatusText + "</div>";

    // Other system info
    widget += "<div class='device-info'>";
    widget += "<div><strong>System Uptime:</strong> <span id='systemUptime'>" + String(millis() / 1000) + "s</span></div>";
    widget += "<div><strong>WiFi SSID:</strong> " + WiFiManager::getCurrentSSID() + "</div>";
    widget += "<div><strong>Building Number:</strong> " + String(getBuildingNumber()) + "</div>";

    // Show alarm details if any alarm is active
    if (alarmSystem.isAlarmActive(RIGHT_SIDE) || alarmSystem.isAlarmActive(LEFT_SIDE))
    {
        widget += "<div style='margin-top: 1rem;'><strong>Active Alarms:</strong></div>";

        if (alarmSystem.isAlarmActive(RIGHT_SIDE))
        {
            widget += "<div style='color: var(--danger);'>• Right Side Alarm Active</div>";
        }

        if (alarmSystem.isAlarmActive(LEFT_SIDE))
        {
            widget += "<div style='color: var(--danger);'>• Left Side Alarm Active</div>";
        }
    }

    widget += "</div>";
    widget += "</div>";

    return widget;
}

/**
 * Handle root page request
 */
void WebPortal::handleRoot()
{
    // Redirect to dashboard
    _server.sendHeader("Location", ROUTE_DASHBOARD);
    _server.send(302);
}

/**
 * Handle dashboard page request
 */
void WebPortal::handleDashboard()
{
    _pageRequests++;
    updateLastActivity();

    String html = getHTMLHeader("Dashboard", true); // Include auto-refresh

    // Add navigation menu (pass admin status)
    html += getNavigationMenu(isAdminLoggedIn());

    html += "<div class='container'>";
    html += "<h1>Water Meter Security System</h1>";

    // System status widget
    html += getSystemStatusWidget();

    // Sensor Status Table
    html += buildSensorStatusTable();

    // Wire Cut Status Table
    html += buildWireCutStatusTable();

    html += "</div>";

    html += "<script>";

    // Utility functions for time formatting
    html += "function formatTime(seconds) {";
    html += "  const days = Math.floor(seconds / 86400);";
    html += "  const hours = Math.floor((seconds % 86400) / 3600);";
    html += "  const minutes = Math.floor((seconds % 3600) / 60);";
    html += "  const secs = seconds % 60;";
    html += "  let result = '';";
    html += "  if (days > 0) result += days + 'd ';";
    html += "  if (hours > 0) result += hours + 'h ';";
    html += "  if (minutes > 0) result += minutes + 'm ';";
    html += "  result += secs + 's';";
    html += "  return result;";
    html += "}";

    // Update function for real-time data
    html += "function updateData() {";
    html += "  fetch('/api/status').then(resp => resp.json()).then(data => {";
    html += "    document.getElementById('systemUptime').textContent = formatTime(data.system.uptime);";
    html += "  });";
    html += "}";

    // Set interval for updates
    html += "setInterval(updateData, 1000);";
    html += "updateData();";

    html += "</script>";

    html += getHTMLFooter();

    _server.send(200, HTML_CONTENT_TYPE, html);
}

/**
 * Build HTML for sensor status table
 */
String WebPortal::buildSensorStatusTable()
{
    String html = "<div class='card'>";
    html += "<h2>Apartments Status</h2>";

    html += "<div style='overflow-x: auto;'>";
    html += "<table style='width: 100%; border-collapse: collapse;'>";
    html += "<thead style='background-color: rgba(0,0,0,0.05);'>";
    html += "<tr>";
    html += "<th style='padding: 0.5rem; text-align: center;'>Apt #</th>";
    html += "<th style='padding: 0.5rem; text-align: center;'>Status</th>";
    html += "<th style='padding: 0.5rem; text-align: center;'>Side</th>";
    html += "<th style='padding: 0.5rem; text-align: center;'>Box</th>";
    html += "<th style='padding: 0.5rem; text-align: center;'>Position</th>";
    html += "<th style='padding: 0.5rem; text-align: center;'>Telegram</th>";
    html += "</tr>";
    html += "</thead>";
    html += "<tbody>";

    for (int i = 0; i < TOTAL_APARTMENTS; i++)
    {
        uint8_t aptNumber = i + 1;
        BuildingSide side = getApartmentSide(aptNumber);
        BoxPosition box = getApartmentBox(aptNumber);
        String position = "Unknown";

        // Find the apartment in the APARTMENT_LOCATIONS array to get its position
        for (int j = 0; j < TOTAL_APARTMENTS; j++)
        {
            if (APARTMENT_LOCATIONS[j].side == side &&
                APARTMENT_LOCATIONS[j].box == box &&
                getApartmentIndex(aptNumber) == j)
            {
                position = APARTMENT_LOCATIONS[j].position;
                break;
            }
        }

        String sideText = (side == RIGHT_SIDE) ? "الجانب الأيمن" : "الجانب الأيسر";
        String boxText = (box == RIGHT_BOX) ? "الصندوق الأيمن" : "الصندوق الأيسر";

        String statusClass = alarmSystem.isSensorEnabled(aptNumber) ? "success" : "info";
        String statusText = alarmSystem.isSensorEnabled(aptNumber) ? "Enabled" : "Disabled";

        // If sensor is triggered, show warning
        if (alarmSystem.isSensorTriggered(aptNumber))
        {
            statusClass = "error";
            statusText = "Triggered";
        }

        String telegramStatus = telegramHandler.isApartmentConfigured(aptNumber) ? (telegramHandler.isApartmentEnabled(aptNumber) ? "Active" : "Disabled") : "Not Configured";

        String telegramClass = telegramHandler.isApartmentConfigured(aptNumber) ? (telegramHandler.isApartmentEnabled(aptNumber) ? "success" : "info") : "info";

        html += "<tr style='border-bottom: 1px solid #f0f0f0;'>";
        html += "<td style='padding: 0.5rem; text-align: center;'>" + String(aptNumber) + "</td>";
        html += "<td style='padding: 0.5rem; text-align: center;'><span class='status " + statusClass + "' style='display: inline-block; padding: 0.2rem 0.5rem;'>" + statusText + "</span></td>";
        html += "<td style='padding: 0.5rem; text-align: center;'>" + sideText + "</td>";
        html += "<td style='padding: 0.5rem; text-align: center;'>" + boxText + "</td>";
        html += "<td style='padding: 0.5rem; text-align: center;'>" + String(position) + "</td>";
        html += "<td style='padding: 0.5rem; text-align: center;'><span class='status " + telegramClass + "' style='display: inline-block; padding: 0.2rem 0.5rem;'>" + telegramStatus + "</span></td>";
        html += "</tr>";
    }

    html += "</tbody>";
    html += "</table>";
    html += "</div>";
    html += "</div>";

    return html;
}

/**
 * Build HTML for wire cut status table
 */
String WebPortal::buildWireCutStatusTable()
{
    WireCutStatus wireStatus = alarmSystem.getWireCutStatus();

    String html = "<div class='card'>";
    html += "<h2>Wire Status</h2>";

    html += "<div style='overflow-x: auto;'>";
    html += "<table style='width: 100%; border-collapse: collapse;'>";
    html += "<thead style='background-color: rgba(0,0,0,0.05);'>";
    html += "<tr>";
    html += "<th style='padding: 0.5rem; text-align: left;'>Location</th>";
    html += "<th style='padding: 0.5rem; text-align: center;'>Status</th>";
    html += "<th style='padding: 0.5rem; text-align: center;'>Monitoring</th>";
    html += "</tr>";
    html += "</thead>";
    html += "<tbody>";

    // Right Side - Right Box
    html += "<tr style='border-bottom: 1px solid #f0f0f0;'>";
    html += "<td style='padding: 0.5rem;'>Right Side - Right Box</td>";
    html += "<td style='padding: 0.5rem; text-align: center;'>";
    html += "<span class='status " + String(wireStatus.rightSideRightBox ? "error" : "success") + "' style='display: inline-block; padding: 0.2rem 0.5rem;'>";
    html += wireStatus.rightSideRightBox ? "Cut Detected" : "OK";
    html += "</span></td>";
    html += "<td style='padding: 0.5rem; text-align: center;'>";
    html += "<span class='status " + String(wireStatus.rightSideRightBoxEnabled ? "success" : "info") + "' style='display: inline-block; padding: 0.2rem 0.5rem;'>";
    html += wireStatus.rightSideRightBoxEnabled ? "Enabled" : "Disabled";
    html += "</span></td>";
    html += "</tr>";

    // Right Side - Left Box
    html += "<tr style='border-bottom: 1px solid #f0f0f0;'>";
    html += "<td style='padding: 0.5rem;'>Right Side - Left Box</td>";
    html += "<td style='padding: 0.5rem; text-align: center;'>";
    html += "<span class='status " + String(wireStatus.rightSideLeftBox ? "error" : "success") + "' style='display: inline-block; padding: 0.2rem 0.5rem;'>";
    html += wireStatus.rightSideLeftBox ? "Cut Detected" : "OK";
    html += "</span></td>";
    html += "<td style='padding: 0.5rem; text-align: center;'>";
    html += "<span class='status " + String(wireStatus.rightSideLeftBoxEnabled ? "success" : "info") + "' style='display: inline-block; padding: 0.2rem 0.5rem;'>";
    html += wireStatus.rightSideLeftBoxEnabled ? "Enabled" : "Disabled";
    html += "</span></td>";
    html += "</tr>";

    // Left Side - Right Box
    html += "<tr style='border-bottom: 1px solid #f0f0f0;'>";
    html += "<td style='padding: 0.5rem;'>Left Side - Right Box</td>";
    html += "<td style='padding: 0.5rem; text-align: center;'>";
    html += "<span class='status " + String(wireStatus.leftSideRightBox ? "error" : "success") + "' style='display: inline-block; padding: 0.2rem 0.5rem;'>";
    html += wireStatus.leftSideRightBox ? "Cut Detected" : "OK";
    html += "</span></td>";
    html += "<td style='padding: 0.5rem; text-align: center;'>";
    html += "<span class='status " + String(wireStatus.leftSideRightBoxEnabled ? "success" : "info") + "' style='display: inline-block; padding: 0.2rem 0.5rem;'>";
    html += wireStatus.leftSideRightBoxEnabled ? "Enabled" : "Disabled";
    html += "</span></td>";
    html += "</tr>";

    // Left Side - Left Box
    html += "<tr style='border-bottom: 1px solid #f0f0f0;'>";
    html += "<td style='padding: 0.5rem;'>Left Side - Left Box</td>";
    html += "<td style='padding: 0.5rem; text-align: center;'>";
    html += "<span class='status " + String(wireStatus.leftSideLeftBox ? "error" : "success") + "' style='display: inline-block; padding: 0.2rem 0.5rem;'>";
    html += wireStatus.leftSideLeftBox ? "Cut Detected" : "OK";
    html += "</span></td>";
    html += "<td style='padding: 0.5rem; text-align: center;'>";
    html += "<span class='status " + String(wireStatus.leftSideLeftBoxEnabled ? "success" : "info") + "' style='display: inline-block; padding: 0.2rem 0.5rem;'>";
    html += wireStatus.leftSideLeftBoxEnabled ? "Enabled" : "Disabled";
    html += "</span></td>";
    html += "</tr>";

    // Right Side - Distribution
    html += "<tr style='border-bottom: 1px solid #f0f0f0;'>";
    html += "<td style='padding: 0.5rem;'>Right Side - Distribution</td>";
    html += "<td style='padding: 0.5rem; text-align: center;'>";
    html += "<span class='status " + String(wireStatus.rightSideDistribution ? "error" : "success") + "' style='display: inline-block; padding: 0.2rem 0.5rem;'>";
    html += wireStatus.rightSideDistribution ? "Cut Detected" : "OK";
    html += "</span></td>";
    html += "<td style='padding: 0.5rem; text-align: center;'>";
    html += "<span class='status " + String(wireStatus.rightSideDistributionEnabled ? "success" : "info") + "' style='display: inline-block; padding: 0.2rem 0.5rem;'>";
    html += wireStatus.rightSideDistributionEnabled ? "Enabled" : "Disabled";
    html += "</span></td>";
    html += "</tr>";

    // Left Side - Distribution
    html += "<tr style='border-bottom: 1px solid #f0f0f0;'>";
    html += "<td style='padding: 0.5rem;'>Left Side - Distribution</td>";
    html += "<td style='padding: 0.5rem; text-align: center;'>";
    html += "<span class='status " + String(wireStatus.leftSideDistribution ? "error" : "success") + "' style='display: inline-block; padding: 0.2rem 0.5rem;'>";
    html += wireStatus.leftSideDistribution ? "Cut Detected" : "OK";
    html += "</span></td>";
    html += "<td style='padding: 0.5rem; text-align: center;'>";
    html += "<span class='status " + String(wireStatus.leftSideDistributionEnabled ? "success" : "info") + "' style='display: inline-block; padding: 0.2rem 0.5rem;'>";
    html += wireStatus.leftSideDistributionEnabled ? "Enabled" : "Disabled";
    html += "</span></td>";
    html += "</tr>";

    html += "</tbody>";
    html += "</table>";
    html += "</div>";
    html += "</div>";

    return html;
}

/**
 * Build HTML for alarm status table
 */
String WebPortal::buildAlarmStatusTable()
{
    String html = "<div class='card'>";
    html += "<h2>Alarm Status</h2>";

    // Alarm system status
    html += "<div style='margin-bottom: 1rem;'>";
    html += "<strong>System Status: </strong>";

    String statusClass = "success";
    String statusText = "Normal";

    switch (alarmSystem.getStatus())
    {
    case AlarmSystemStatus::NORMAL:
        statusClass = "success";
        statusText = "Normal";
        break;
    case AlarmSystemStatus::THEFT_DETECTED:
        statusClass = "error";
        statusText = "Theft Detected";
        break;
    case AlarmSystemStatus::WIRE_CUT_DETECTED:
        statusClass = "error";
        statusText = "Wire Cut Detected";
        break;
    }

    html += "<span class='status " + String(statusClass) + "'>" + String(statusText) + "</span>";
    html += "</div>";

    // Right side alarm
    html += "<div style='margin-bottom: 0.5rem;'>";
    html += "<strong>Right Side Alarm: </strong>";
    html += "<span class='status " + String((alarmSystem.isAlarmActive(RIGHT_SIDE) ? "error" : "success")) + "'>";
    html += alarmSystem.isAlarmActive(RIGHT_SIDE) ? "Active" : "Inactive";
    html += "</span>";
    html += "</div>";

    // Left side alarm
    html += "<div style='margin-bottom: 0.5rem;'>";
    html += "<strong>Left Side Alarm: </strong>";
    html += "<span class='status " + String((alarmSystem.isAlarmActive(LEFT_SIDE) ? "error" : "success")) + "'>";
    html += String(alarmSystem.isAlarmActive(LEFT_SIDE) ? "Active" : "Inactive");
    html += "</span>";
    html += "</div>";

    // Last alarm time (if any)
    if (alarmSystem.getLastAlarmTime() > 0)
    {
        html += "<div style='margin-top: 1rem;'>";
        html += "<strong>Last Alarm: </strong>";

        uint32_t secondsAgo = (millis() - alarmSystem.getLastAlarmTime()) / 1000;
        html += "<span id='lastAlarmTime'>" + String(secondsAgo) + "s ago</span>";
        html += "</div>";
    }

    html += "</div>";

    return html;
}

/**
 * Handle WiFi configuration page request
 */
void WebPortal::handleWiFiConfigPage()
{
    _pageRequests++;
    updateLastActivity();

    bool passwordRequired = _wiFiConfigPassword.length() > 0;
    bool isLoggedIn = hasValidSession(AuthLevel::WIFI_CONFIG);

    // Check if we need to show the login form
    if (passwordRequired && !isLoggedIn)
    {
        bool authAttempt = false;
        String password = "";

        // Process login attempt
        if (_server.hasArg("password") && _server.hasArg("action") && _server.arg("action") == "login")
        {
            authAttempt = true;
            password = _server.arg("password");

            if (verifyWiFiConfigPassword(password))
            {
                String sessionId = createSession(AuthLevel::WIFI_CONFIG);

                // Set session cookie and redirect
                _server.sendHeader("Set-Cookie", "wifiSession=" + sessionId + "; Path=/; Max-Age=" + String(WIFI_CONFIG_SESSION_TIMEOUT / 1000));
                _server.sendHeader("Location", ROUTE_WIFI_CONFIG);
                _server.send(302);
                return;
            }
        }

        // Show login form
        String html = getHTMLHeader("WiFi Configuration");
        html += getNavigationMenu(false);

        html += "<div class='container'>";
        html += "<h1>WiFi Configuration</h1>";

        html += "<div class='card'>";
        html += "<h2>Authentication Required</h2>";

        if (authAttempt)
        {
            html += "<div class='status error'>Invalid password. Please try again.</div>";
        }

        html += "<form method='post'>";
        html += "<div class='form-group'>";
        html += "<label for='password'>WiFi Configuration Password:</label>";
        html += "<input type='password' id='password' name='password' required>";
        html += "</div>";
        html += "<input type='hidden' name='action' value='login'>";
        html += "<button type='submit'>Login</button>";
        html += "</form>";
        html += "</div>";

        html += "</div>";
        html += getHTMLFooter();

        _server.send(200, HTML_CONTENT_TYPE, html);
    }
    else
    {
        // User is already logged in or no password required
        // Callback for WiFi config start (once when page is loaded)
        if (_onWiFiConfigStartCallback)
        {
            _onWiFiConfigStartCallback();
        }

        String html = getHTMLHeader("WiFi Configuration");
        html += getNavigationMenu(isAdminLoggedIn());

        html += "<div class='container'>";
        html += "<h1>WiFi Configuration</h1>";

        html += "<div class='card'>";
        html += "<h2>Current Connection</h2>";

        if (WiFiManager::isConnected())
        {
            html += "<div class='status success'>Connected to WiFi</div>";
            html += "<div class='device-info'>";
            html += "<div><strong>SSID:</strong> " + WiFiManager::getCurrentSSID() + "</div>";
            html += "<div><strong>IP Address:</strong> " + WiFiManager::getLocalIP() + "</div>";
            html += "<div><strong>Signal Strength:</strong> " + String(WiFiManager::getRSSI()) + " dBm</div>";
            html += "</div>";
        }
        else
        {
            html += "<div class='status error'>Not Connected to WiFi</div>";

            if (WiFiManager::isInAPMode())
            {
                html += "<div class='device-info'>";
                html += "<div><strong>AP Mode Active:</strong> " + WiFiManager::getAPSSID() + "</div>";
                html += "<div><strong>AP IP Address:</strong> " + WiFiManager::getAPIP() + "</div>";
                html += "</div>";
            }
        }
        html += "</div>";

        // WiFi network scan
        html += "<div class='card'>";
        html += "<h2>Available Networks</h2>";
        html += "<button id='scanBtn' onclick='scanNetworks()'>Scan for Networks</button>";
        html += "<div id='networkList' class='network-list'></div>";
        html += "</div>";

        // Connection form
        html += "<div class='card'>";
        html += "<h2>Connect to Network</h2>";
        html += "<form id='wifiForm' onsubmit='connectToWifi(event)'>";
        html += "<div class='form-group'>";
        html += "<label for='ssid'>SSID:</label>";
        html += "<input type='text' id='ssid' name='ssid' required>";
        html += "</div>";
        html += "<div class='form-group password-container'>";
        html += "<label for='password'>Password:</label>";
        html += "<input type='password' id='password' name='password'>";
        html += "<span class='password-toggle' onclick='togglePassword()'>👁️</span>";
        html += "</div>";
        html += "<button type='submit'>Connect</button>";
        html += "</form>";
        html += "<div id='connectionStatus' class='status hidden'></div>";
        html += "</div>";

        html += "</div>"; // End container

        html += "<script>";
        // Scan for networks
        html += "function scanNetworks() {";
        html += "  document.getElementById('scanBtn').disabled = true;";
        html += "  document.getElementById('scanBtn').innerText = 'Scanning...';";
        html += "  document.getElementById('networkList').innerHTML = '<div class=\"status info\">Scanning for networks...</div>';";
        html += "  fetch('/scan-networks')";
        html += "    .then(function(response) {";
        html += "      if (!response.ok) {";
        html += "        throw new Error('Network scan failed: ' + response.status);";
        html += "      }";
        html += "      return response.json();";
        html += "    })";
        html += "    .then(function(data) {";
        html += "      console.log('Network scan response:', data);";
        html += "      const networkList = document.getElementById('networkList');";
        html += "      networkList.innerHTML = '';";
        html += "      if (!data.networks || data.networks.length === 0) {";
        html += "        networkList.innerHTML = '<div class=\"status info\">No networks found</div>';";
        html += "      } else {";
        html += "        data.networks.forEach(function(network) {";
        html += "          const item = document.createElement('div');";
        html += "          item.className = 'network-item';";
        html += "          item.onclick = function() { selectNetwork(network.ssid); };";
        html += "          let signalStrength = 'Weak';";
        html += "          if (network.rssi > -67) signalStrength = 'Strong';";
        html += "          else if (network.rssi > -80) signalStrength = 'Medium';";
        html += "          const encryptedText = network.encrypted && network.encrypted !== 'OPEN' ? '🔒' : '';";
        html += "          item.innerHTML = network.ssid + ' ' + encryptedText + ' <span class=\"signal-strength\">' + signalStrength + ' (' + network.rssi + ' dBm)</span>';";
        html += "          networkList.appendChild(item);";
        html += "        });";
        html += "      }";
        html += "      document.getElementById('scanBtn').disabled = false;";
        html += "      document.getElementById('scanBtn').innerText = 'Scan for Networks';";
        html += "    })";
        html += "    .catch(function(error) {";
        html += "      console.error('Error scanning networks:', error);";
        html += "      document.getElementById('networkList').innerHTML = '<div class=\"status error\">Error scanning: ' + error.message + '</div>';";
        html += "      document.getElementById('scanBtn').disabled = false;";
        html += "      document.getElementById('scanBtn').innerText = 'Scan for Networks';";
        html += "    });";
        html += "}";

        // Select network from scan results
        html += "function selectNetwork(ssid) {";
        html += "  document.getElementById('ssid').value = ssid;";
        html += "  document.getElementById('password').focus();\n";
        html += "  const networks = document.getElementsByClassName('network-item');";
        html += "  for (let i = 0; i < networks.length; i++) {";
        html += "    networks[i].classList.remove('active');";
        html += "    if (networks[i].textContent.startsWith(ssid)) {";
        html += "      networks[i].classList.add('active');";
        html += "    }";
        html += "  }";
        html += "}";

        // Connect to WiFi
        html += "function connectToWifi(event) {";
        html += "  event.preventDefault();";
        html += "  const ssid = document.getElementById('ssid').value;";
        html += "  const password = document.getElementById('password').value;";
        html += "  const status = document.getElementById('connectionStatus');";
        html += "  status.className = 'status info';";
        html += "  status.textContent = 'Connecting to ' + ssid + '...';";
        html += "  status.classList.remove('hidden');";
        html += "  document.getElementById('ssid').disabled = true;";
        html += "  document.getElementById('password').disabled = true;";
        html += "  document.querySelector('#wifiForm button').disabled = true;";
        html += "  const formData = new FormData();";
        html += "  formData.append('ssid', ssid);";
        html += "  formData.append('password', password);";
        html += "  fetch('/save-wifi', {";
        html += "    method: 'POST',";
        html += "    body: formData";
        html += "  })";
        html += "  .then(function(response) {";
        html += "    return response.json();";
        html += "  })";
        html += "  .then(function(data) {";
        html += "    if (data.success) {";
        html += "      status.className = 'status success';";
        html += "      status.textContent = 'Connected successfully! Redirecting...';";
        html += "      setTimeout(function() { window.location.href = '/dashboard'; }, 3000);";
        html += "    } else {";
        html += "      status.className = 'status error';";
        html += "      status.textContent = 'Connection failed: ' + data.message;";
        html += "      document.getElementById('ssid').disabled = false;";
        html += "      document.getElementById('password').disabled = false;";
        html += "      document.querySelector('#wifiForm button').disabled = false;";
        html += "    }";
        html += "  })";
        html += "  .catch(function(error) {";
        html += "    status.className = 'status error';";
        html += "    status.textContent = 'Error: ' + error;";
        html += "    document.getElementById('ssid').disabled = false;";
        html += "    document.getElementById('password').disabled = false;";
        html += "    document.querySelector('#wifiForm button').disabled = false;";
        html += "  });";
        html += "}";

        // Toggle password visibility
        html += "function togglePassword() {";
        html += "  const passwordField = document.getElementById('password');";
        html += "  if (passwordField.type === 'password') {";
        html += "    passwordField.type = 'text';";
        html += "  } else {";
        html += "    passwordField.type = 'password';";
        html += "  }";
        html += "}";

        // Scan networks on page load
        html += "window.onload = function() {";
        html += "  scanNetworks();";
        html += "};";
        html += "</script>";

        html += getHTMLFooter();

        _server.send(200, HTML_CONTENT_TYPE, html);
    }
}

/**
 * Handle admin login page request
 */
void WebPortal::handleAdminLogin()
{
    _pageRequests++;
    updateLastActivity();

    // If already logged in, redirect to admin panel
    if (hasValidSession(AuthLevel::ADMIN))
    {
        _server.sendHeader("Location", ROUTE_ADMIN_PANEL);
        _server.send(302);
        return;
    }

    bool authAttempt = false;
    String password = "";

    // Process login attempt
    if (_server.hasArg("password") && _server.hasArg("action") && _server.arg("action") == "login")
    {
        authAttempt = true;
        password = _server.arg("password");

        if (verifyAdminPassword(password))
        {
            String sessionId = createSession(AuthLevel::ADMIN);

            // Set session cookie and redirect
            _server.sendHeader("Set-Cookie", "adminSession=" + sessionId + "; Path=/; Max-Age=" + String(ADMIN_SESSION_TIMEOUT / 1000));
            _server.sendHeader("Location", ROUTE_ADMIN_PANEL);

            // Call the login callback
            if (_onAdminLoginCallback)
            {
                _onAdminLoginCallback();
            }

            _server.send(302);
            return;
        }
    }

    // Show login form
    String html = getHTMLHeader("Admin Login");
    html += getNavigationMenu(false);

    html += "<div class='container'>";
    html += "<h1>Admin Login</h1>";

    html += "<div class='card'>";
    html += "<h2>Authentication Required</h2>";

    if (authAttempt)
    {
        html += "<div class='status error'>Invalid password. Please try again.</div>";
    }

    html += "<form method='post'>";
    html += "<div class='form-group'>";
    html += "<label for='password'>Admin Password:</label>";
    html += "<input type='password' id='password' name='password' required>";
    html += "</div>";
    html += "<input type='hidden' name='action' value='login'>";
    html += "<button type='submit'>Login</button>";
    html += "</form>";
    html += "</div>";

    html += "</div>";
    html += getHTMLFooter();

    _server.send(200, HTML_CONTENT_TYPE, html);
}

/**
 * Handle admin panel page request
 */
void WebPortal::handleAdminPanel()
{
    if (!authenticate(AuthLevel::ADMIN))
    {
        return;
    }

    _pageRequests++;
    updateLastActivity();

    String html = getHTMLHeader("Admin Panel");
    html += getNavigationMenu(true);

    html += "<div class='container'>";
    html += "<h1>Admin Panel</h1>";

    // Admin options
    html += "<div class='card'>";
    html += "<h2>System Configuration</h2>";
    html += "<div style='display: grid; grid-template-columns: repeat(auto-fit, minmax(250px, 1fr)); gap: 1rem;'>";

    // Telegram configuration
    html += "<div>";
    html += "<button onclick=\"window.location.href='" + String(ROUTE_ADMIN_TELEGRAM_CONFIG) + "'\" style='margin-bottom: 0.5rem;'>Telegram Configuration</button>";
    html += "<p style='font-size: 0.9rem; color: #666;'>Configure Telegram notifications for apartments</p>";
    html += "</div>";

    // Building configuration
    html += "<div>";
    html += "<button onclick=\"window.location.href='" + String(ROUTE_ADMIN_BUILDING_CONFIG) + "'\" style='margin-bottom: 0.5rem;'>Building Configuration</button>";
    html += "<p style='font-size: 0.9rem; color: #666;'>Change building number, WiFi access point settings</p>";
    html += "</div>";

    // Advanced configuration
    html += "<div>";
    html += "<button onclick=\"window.location.href='" + String(ROUTE_ADMIN_ADVANCED_CONFIG) + "'\" style='margin-bottom: 0.5rem;'>Advanced Settings</button>";
    html += "<p style='font-size: 0.9rem; color: #666;'>Change admin password, alarm settings, and system configuration</p>";
    html += "</div>";

    // Reset Wire Cut detection
    html += "<div>";
    html += "<button onclick=\"alert('This functionality is not available in the current version.')\" class='warning' style='margin-bottom: 0.5rem;'>Reset Wire Cut Detection</button>";
    html += "<p style='font-size: 0.9rem; color: #666;'>Reset wire cut detection for disabled sensors</p>";
    html += "</div>";

    html += "</div>"; // End grid
    html += "</div>"; // End card

    // System status
    html += getSystemStatusWidget();

    // Apartment status table
    html += buildApartmentRowsHTML();

    // Wire cut status table
    html += buildWireCutStatusTable();

    html += "<script>";
    // Function to handle reset wire cut detection
    html += "function confirmResetWireCut() {";
    html += "  alert('This functionality is not available in the current version. Only can do it by resetting the System through the Switch.');";
    html += "}";
    html += "</script>";

    html += "</div>"; // End container
    html += getHTMLFooter();

    _server.send(200, HTML_CONTENT_TYPE, html);
}

/**
 * Build apartment rows HTML for admin panel
 */
String WebPortal::buildApartmentRowsHTML()
{
    String html = "<div class='card'>";
    html += "<h2>Apartment Configuration</h2>";

    html += "<div style='overflow-x: auto;'>";
    html += "<table style='width: 100%; border-collapse: collapse;'>";
    // Continuing from where we left off
    html += "<thead style='background-color: rgba(0,0,0,0.05);'>";
    html += "<tr>";
    html += "<th style='padding: 0.5rem; text-align: center;'>Apt #</th>";
    html += "<th style='padding: 0.5rem; text-align: center;'>Location</th>";
    html += "<th style='padding: 0.5rem; text-align: center;'>Sensor Status</th>";
    html += "<th style='padding: 0.5rem; text-align: center;'>Telegram Status</th>";
    html += "<th style='padding: 0.5rem; text-align: center;'>Actions</th>";
    html += "</tr>";
    html += "</thead>";
    html += "<tbody>";

    for (int i = 0; i < TOTAL_APARTMENTS; i++)
    {
        uint8_t aptNumber = i + 1;
        BuildingSide side = getApartmentSide(aptNumber);
        BoxPosition box = getApartmentBox(aptNumber);

        String sideText = (side == RIGHT_SIDE) ? "Right Side" : "Left Side";
        String boxText = (box == RIGHT_BOX) ? "Right Box" : "Left Box";

        String sensorStatus = alarmSystem.isSensorEnabled(aptNumber) ? "Enabled" : "Disabled";
        String sensorClass = alarmSystem.isSensorEnabled(aptNumber) ? "success" : "info";

        String telegramStatus = "Not Configured";
        String telegramClass = "info";

        if (telegramHandler.isApartmentConfigured(aptNumber))
        {
            if (telegramHandler.isApartmentEnabled(aptNumber))
            {
                telegramStatus = "Active";
                telegramClass = "success";
            }
            else
            {
                telegramStatus = "Disabled";
                telegramClass = "info";
            }
        }

        html += "<tr style='border-bottom: 1px solid #f0f0f0;'>";
        html += "<td style='padding: 0.5rem; text-align: center;'>" + String(aptNumber) + "</td>";
        html += "<td style='padding: 0.5rem; text-align: center;'>" + sideText + ", " + boxText + "</td>";

        html += "<td style='padding: 0.5rem; text-align: center;'>";
        html += "<span class='status " + sensorClass + "' style='display: inline-block; padding: 0.2rem 0.5rem;'>" + sensorStatus + "</span>";
        html += "</td>";

        html += "<td style='padding: 0.5rem; text-align: center;'>";
        html += "<span class='status " + telegramClass + "' style='display: inline-block; padding: 0.2rem 0.5rem;'>" + telegramStatus + "</span>";
        html += "</td>";

        html += "<td style='padding: 0.5rem; text-align: center;'>";
        // Toggle sensor button
        html += "<button onclick='toggleApartment(" + String(aptNumber) + ", \"sensor\")' class='small-button'>";
        html += alarmSystem.isSensorEnabled(aptNumber) ? "Disable Sensor" : "Enable Sensor";
        html += "</button> ";

        // Toggle telegram button (only if configured)
        if (telegramHandler.isApartmentConfigured(aptNumber))
        {
            html += "<button onclick='toggleApartment(" + String(aptNumber) + ", \"telegram\")' class='small-button'>";
            html += telegramHandler.isApartmentEnabled(aptNumber) ? "Disable Telegram" : "Enable Telegram";
            html += "</button>";
        }

        html += "</td>";
        html += "</tr>";
    }

    html += "</tbody>";
    html += "</table>";
    html += "</div>";

    // JavaScript for apartment toggle
    html += "<script>";
    html += "function toggleApartment(aptNumber, type) {";
    html += "  fetch('" + String(ROUTE_ADMIN_APARTMENT_TOGGLE) + "', {";
    html += "    method: 'POST',";
    html += "    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },";
    html += "    body: 'apartment=' + aptNumber + '&type=' + type";
    html += "  })";
    html += "  .then(response => response.json())";
    html += "  .then(data => {";
    html += "    if (data.success) {";
    html += "      window.location.reload();";
    html += "    } else {";
    html += "      alert('Error: ' + data.message);";
    html += "    }";
    html += "  })";
    html += "  .catch(error => {";
    html += "    alert('Error: ' + error);";
    html += "  });";
    html += "}";
    html += "</script>";

    html += "</div>"; // End card

    return html;
}

/**
 * Handle admin logout request
 */
void WebPortal::handleAdminLogout()
{
    // Clear admin session
    invalidateSession(AuthLevel::ADMIN);

    // Redirect to home page after logout
    _server.sendHeader("Set-Cookie", "adminSession=; Path=/; Max-Age=0");
    _server.sendHeader("Location", ROUTE_ROOT);
    _server.send(302);

    // Call the logout callback
    if (_onAdminLogoutCallback)
    {
        _onAdminLogoutCallback();
    }
}

/**
 * Handle API Status request
 */
void WebPortal::handleAPIStatus()
{
    _server.send(200, JSON_CONTENT_TYPE, getSystemStatusJSON());
}

/**
 * Handle API Apartment Status request
 */
void WebPortal::handleAPIApartmentStatus()
{
    _server.send(200, JSON_CONTENT_TYPE, getApartmentStatusJSON());
}

/**
 * Handle API Wire Status request
 */
void WebPortal::handleAPIWireStatus()
{
    _server.send(200, JSON_CONTENT_TYPE, getWireStatusJSON());
}

/**
 * Handle API Alarm Status request
 */
void WebPortal::handleAPIAlarmStatus()
{
    _server.send(200, JSON_CONTENT_TYPE, getAlarmStatusJSON());
}

/**
 * Handle admin Telegram configuration page
 */
void WebPortal::handleAdminTelegramConfig()
{
    if (!authenticate(AuthLevel::ADMIN))
    {
        return;
    }

    _pageRequests++;
    updateLastActivity();

    String html = getHTMLHeader("Telegram Configuration");
    html += getNavigationMenu(true);

    html += "<div class='container'>";
    html += "<h1>Telegram Configuration</h1>";

    html += "<div class='card'>";
    html += "<h2>Configure Telegram Notifications</h2>";
    html += "<p>Configure Telegram notifications for each apartment. You'll need the Telegram Bot Token and Chat ID for each apartment.</p>";

    // Telegram configuration form
    html += "<form id='telegramForm' method='post' action='" + String(ROUTE_ADMIN_SAVE_TELEGRAM) + "'>";

    // Apartment selection
    html += "<div class='form-group'>";
    html += "<label for='apartment'>Select Apartment:</label>";
    html += "<select id='apartment' name='apartment' onchange='loadTelegramConfig()'>";

    for (int i = 0; i < TOTAL_APARTMENTS; i++)
    {
        uint8_t aptNumber = i + 1;
        BuildingSide side = getApartmentSide(aptNumber);
        BoxPosition box = getApartmentBox(aptNumber);

        String sideText = (side == RIGHT_SIDE) ? "Right Side" : "Left Side";
        String boxText = (box == RIGHT_BOX) ? "Right Box" : "Left Box";

        html += "<option value='" + String(aptNumber) + "'>Apartment " + String(aptNumber) + " (" + sideText + ", " + boxText + ")</option>";
    }

    html += "</select>";
    html += "</div>";

    // Display current configuration if any
    html += "<div id='currentConfig' class='device-info' style='margin-bottom: 1rem;'>";
    html += "<p>Select an apartment to see its current configuration.</p>";
    html += "</div>";

    // Bot Token
    html += "<div class='form-group'>";
    html += "<label for='token'>Telegram Bot Token:</label>";
    html += "<input type='text' id='token' name='token' placeholder='Enter the Telegram Bot Token'>";
    html += "</div>";

    // Chat ID
    html += "<div class='form-group'>";
    html += "<label for='chatId'>Telegram Chat ID:</label>";
    html += "<input type='text' id='chatId' name='chatId' placeholder='Enter the Telegram Chat ID'>";
    html += "</div>";

    // Enable/Disable
    html += "<div class='form-group'>";
    html += "<label for='enabled'>Enable Notifications:</label>";
    html += "<select id='enabled' name='enabled'>";
    html += "<option value='1'>Enabled</option>";
    html += "<option value='0'>Disabled</option>";
    html += "</select>";
    html += "</div>";

    // Save button
    html += "<button type='submit'>Save Configuration</button>";

    // Remove configuration button
    html += "<button type='button' class='warning' onclick='removeConfig()' style='margin-top: 1rem;'>Remove Configuration</button>";

    html += "</form>";
    html += "</div>"; // End card

    // JavaScript for telegram configuration
    html += "<script>";

    // Function to load telegram configuration for selected apartment
    html += "function loadTelegramConfig() {";
    html += "  const apartment = document.getElementById('apartment').value;";
    html += "  const configDiv = document.getElementById('currentConfig');";
    html += "  configDiv.innerHTML = '<p>Loading configuration...</p>';";

    // Use the new endpoint to get configuration data
    html += "  fetch('/admin/get-telegram-config?apartment=' + apartment)";
    html += "    .then(response => response.json())";
    html += "    .then(data => {";
    html += "      if (data.success) {";
    html += "        if (data.configured) {";
    html += "          configDiv.innerHTML = `";
    html += "            <p><strong>Current Status:</strong> ${data.enabled ? 'Enabled' : 'Disabled'}</p>";
    html += "            <p><strong>Bot Token:</strong> ${data.token}</p>";
    html += "            <p><strong>Chat ID:</strong> ${data.chatId}</p>`;";
    html += "          document.getElementById('token').value = data.token || '';";
    html += "          document.getElementById('chatId').value = data.chatId || '';";
    html += "          document.getElementById('enabled').value = data.enabled ? '1' : '0';";
    html += "        } else {";
    html += "          configDiv.innerHTML = '<p>No Telegram configuration for this apartment</p>';";
    html += "          document.getElementById('token').value = '';";
    html += "          document.getElementById('chatId').value = '';";
    html += "          document.getElementById('enabled').value = '1';";
    html += "        }";
    html += "      } else {";
    html += "        configDiv.innerHTML = '<p class=\"status error\">Error: ' + data.message + '</p>';";
    html += "      }";
    html += "    })";
    html += "    .catch(error => {";
    html += "      configDiv.innerHTML = '<p class=\"status error\">Error: ' + error + '</p>';";
    html += "    });";
    html += "}";

    // Function to remove configuration
    html += "function removeConfig() {";
    html += "  const apartment = document.getElementById('apartment').value;";
    html += "  if (confirm('Are you sure you want to remove Telegram configuration for Apartment ' + apartment + '?')) {";
    html += "    fetch('/admin/save-telegram', {";
    html += "      method: 'POST',";
    html += "      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },";
    html += "      body: 'apartment=' + apartment + '&action=remove'";
    html += "    })";
    html += "    .then(response => response.json())";
    html += "    .then(data => {";
    html += "      if (data.success) {";
    html += "        alert('Configuration removed successfully!');";
    html += "        window.location.reload();";
    html += "      } else {";
    html += "        alert('Error: ' + data.message);";
    html += "      }";
    html += "    })";
    html += "    .catch(error => {";
    html += "      alert('Error: ' + error);";
    html += "    });";
    html += "  }";
    html += "}";

    // Load configuration for the first apartment on page load
    html += "window.onload = function() {";
    html += "  loadTelegramConfig();";
    html += "};";

    html += "</script>";

    html += "</div>"; // End container
    html += getHTMLFooter();

    _server.send(200, HTML_CONTENT_TYPE, html);
}

/**
 * Handle admin Building configuration page
 */
void WebPortal::handleAdminBuildingConfig()
{
    if (!authenticate(AuthLevel::ADMIN))
    {
        return;
    }

    _pageRequests++;
    updateLastActivity();

    String html = getHTMLHeader("Building Configuration");
    html += getNavigationMenu(true);

    html += "<div class='container'>";
    html += "<h1>Building Configuration</h1>";

    html += "<div class='card'>";
    html += "<h2>Configure Building Parameters</h2>";

    // Building configuration form
    html += "<form method='post' action='" + String(ROUTE_ADMIN_SAVE_BUILDING) + "'>";

    // Building Number
    html += "<div class='form-group'>";
    html += "<label for='buildingNumber'>Building Number:</label>";
    html += "<input type='number' id='buildingNumber' name='buildingNumber' value='" + String(getBuildingNumber()) + "' min='1' max='999'>";
    html += "</div>";

    // WiFi Config Password
    html += "<div class='form-group'>";
    html += "<label for='wifiConfigPassword'>WiFi Configuration Password (leave empty for no password):</label>";
    html += "<input type='password' id='wifiConfigPassword' name='wifiConfigPassword' value='" + getWiFiConfigPassword() + "'>";
    html += "<p style='font-size: 0.9rem; color: #666; margin-top: 0.5rem;'>This password controls access to the WiFi settings page.</p>";
    html += "</div>";

    // Save button
    html += "<button type='submit'>Save Configuration</button>";
    html += "</form>";
    html += "</div>"; // End card

    html += "</div>"; // End container
    html += getHTMLFooter();

    _server.send(200, HTML_CONTENT_TYPE, html);
}

/**
 * Handle admin Advanced configuration page
 */
void WebPortal::handleAdminAdvancedConfig()
{
    if (!authenticate(AuthLevel::ADMIN))
    {
        return;
    }

    _pageRequests++;
    updateLastActivity();

    String html = getHTMLHeader("Advanced Configuration");
    html += getNavigationMenu(true);

    html += "<div class='container'>";
    html += "<h1>Advanced Configuration</h1>";

    html += "<div class='card'>";
    html += "<h2>Admin Password</h2>";

    // Admin password form
    html += "<form method='post' action='" + String(ROUTE_ADMIN_SAVE_ADVANCED) + "'>";

    // Current password
    html += "<div class='form-group'>";
    html += "<label for='currentPassword'>Current Password:</label>";
    html += "<input type='password' id='currentPassword' name='currentPassword' required>";
    html += "</div>";

    // New password
    html += "<div class='form-group'>";
    html += "<label for='newPassword'>New Password:</label>";
    html += "<input type='password' id='newPassword' name='newPassword' required>";
    html += "</div>";

    // Confirm new password
    html += "<div class='form-group'>";
    html += "<label for='confirmPassword'>Confirm New Password:</label>";
    html += "<input type='password' id='confirmPassword' name='confirmPassword' required>";
    html += "</div>";

    // Hidden field for action
    html += "<input type='hidden' name='action' value='changePassword'>";

    // Save button
    html += "<button type='submit'>Change Password</button>";
    html += "</form>";
    html += "</div>"; // End card

    html += "<div class='card'>";
    html += "<h2>Alarm Settings</h2>";

    // Alarm settings form
    html += "<form method='post' action='" + String(ROUTE_ADMIN_SAVE_ADVANCED) + "'>";

    // Alarm duration
    html += "<div class='form-group'>";
    html += "<label for='alarmDuration'>Alarm Duration (milliseconds):</label>";
    html += "<input type='number' id='alarmDuration' name='alarmDuration' value='20000' min='5000' max='60000' step='1000'>";
    html += "</div>";

    // Alarm interval
    html += "<div class='form-group'>";
    html += "<label for='alarmInterval'>Alarm Interval (milliseconds):</label>";
    html += "<input type='number' id='alarmInterval' name='alarmInterval' value='1000' min='500' max='5000' step='100'>";
    html += "</div>";

    // Sensor settling time
    html += "<div class='form-group'>";
    html += "<label for='sensorSettlingTime'>Sensor Settling Time (milliseconds):</label>";
    html += "<input type='number' id='sensorSettlingTime' name='sensorSettlingTime' value='100' min='50' max='500' step='10'>";
    html += "</div>";

    // Hidden field for action
    html += "<input type='hidden' name='action' value='alarmSettings'>";

    // Save button
    html += "<button type='submit'>Save Alarm Settings</button>";
    html += "</form>";
    html += "</div>"; // End card

    html += "</div>"; // End container
    html += getHTMLFooter();

    _server.send(200, HTML_CONTENT_TYPE, html);
}

/**
 * Handle Scan Networks request
 */
void WebPortal::handleScanNetworks()
{
    updateLastActivity();
    Serial.println("Starting network scan from web interface...");

    // Add CORS headers for modern browsers
    _server.sendHeader("Access-Control-Allow-Origin", "*");
    _server.sendHeader("Access-Control-Allow-Methods", "GET");
    _server.sendHeader("Access-Control-Allow-Headers", "Content-Type");

    // Clear existing scan results
    WiFi.scanDelete();

    // Perform direct WiFi scan - this works on ESP32 while staying connected
    Serial.println("Scanning for networks...");
    int networkCount = WiFi.scanNetworks();
    Serial.printf("Scan completed, found %d networks\n", networkCount);

    // Build response JSON
    String response = "{\"networks\":[";

    for (int i = 0; i < networkCount; i++)
    {
        if (i > 0)
        {
            response += ",";
        }

        // Get network details and escape special characters in SSID
        String ssid = WiFi.SSID(i);
        ssid.replace("\"", "\\\""); // Escape double quotes

        // Get encryption type
        String encryption = "UNKNOWN";
        switch (WiFi.encryptionType(i))
        {
        case WIFI_AUTH_OPEN:
            encryption = "OPEN";
            break;
        case WIFI_AUTH_WEP:
            encryption = "WEP";
            break;
        case WIFI_AUTH_WPA_PSK:
            encryption = "WPA";
            break;
        case WIFI_AUTH_WPA2_PSK:
            encryption = "WPA2";
            break;
        case WIFI_AUTH_WPA_WPA2_PSK:
            encryption = "WPA/WPA2";
            break;
        default:
            encryption = "UNKNOWN";
        }

        // Build network object with proper keys
        response += "{";
        response += "\"ssid\":\"" + ssid + "\",";
        response += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
        response += "\"encrypted\":\"" + encryption + "\"";
        response += "}";

        // Small delay to prevent watchdog triggers
        yield();
    }

    response += "]}";

    Serial.println("Sending scan results to client");
    _server.send(200, JSON_CONTENT_TYPE, response);
}

/**
 * Handle Save WiFi request
 */
void WebPortal::handleSaveWiFi()
{
    // Require authentication if WiFi config password is set
    if (_wiFiConfigPassword.length() > 0 && !hasValidSession(AuthLevel::WIFI_CONFIG))
    {
        _server.send(403, JSON_CONTENT_TYPE, "{\"success\":false,\"message\":\"Authentication required. Refresh and try again\"}");
        return;
    }

    if (!_server.hasArg("ssid"))
    {
        _server.send(400, JSON_CONTENT_TYPE, "{\"success\":false,\"message\":\"SSID is required\"}");
        return;
    }

    String ssid = _server.arg("ssid");
    String password = _server.arg("password");

    if (ssid.length() == 0)
    {
        _server.send(400, JSON_CONTENT_TYPE, "{\"success\":false,\"message\":\"SSID cannot be empty\"}");
        return;
    }

    // Save WiFi credentials
    if (WiFiManager::saveWiFiCredentials(ssid, password))
    {
        // Try to connect with the new credentials
        if (WiFiManager::reconnect())
        {
            
            Serial.println("WiFi Reconnected successfully!");

            // Success - connected with new credentials
            _server.send(200, JSON_CONTENT_TYPE, "{\"success\":true,\"message\":\"Connected to " + ssid + "\",\"ip\":\"" + WiFiManager::getLocalIP() + "\"}");

            // Call the callback
            if (_onWiFiConfigSaveCallback)
            {
                _onWiFiConfigSaveCallback();
            }
        }
        else
        {
            // Failed to connect - but credentials are saved
            _server.send(200, JSON_CONTENT_TYPE, "{\"success\":false,\"message\":\"Credentials saved, but failed to connect. Device will continue trying to connect.\"}");
        }
    }
    else
    {
        // Failed to save credentials
        _server.send(500, JSON_CONTENT_TYPE, "{\"success\":false,\"message\":\"Failed to save WiFi credentials\"}");
    }
}


/**
 * Escape special characters in JSON strings
 */
String escapeJSON(const String &input) {
    String output = input;
    output.replace("\\", "\\\\");
    output.replace("\"", "\\\"");
    output.replace("\n", "\\n");
    output.replace("\r", "\\r");
    output.replace("\t", "\\t");
    return output;
}

/**
 * Handle getting Telegram configuration for a specific apartment
 */
void WebPortal::handleGetTelegramConfig()
{
    if (!authenticate(AuthLevel::ADMIN))
    {
        _server.send(403, JSON_CONTENT_TYPE, "{\"success\":false,\"message\":\"Authentication required. Refresh and try again\"}");
        return;
    }

    if (!_server.hasArg("apartment"))
    {
        _server.send(400, JSON_CONTENT_TYPE, "{\"success\":false,\"message\":\"Apartment number is required\"}");
        return;
    }

    uint8_t apartmentNumber = _server.arg("apartment").toInt();

    // Create JSON response
    String json = "{";
    json += "\"success\":true,";
    json += "\"apartment\":" + String(apartmentNumber) + ",";
    json += "\"configured\":" + String(telegramHandler.isApartmentConfigured(apartmentNumber) ? "true" : "false") + ",";

    // If configured, include token and chatId
    if (telegramHandler.isApartmentConfigured(apartmentNumber))
    {
        // We need a way to get these values from TelegramHandler
        // Assuming TelegramHandler class has these methods
        json += "\"enabled\":" + String(telegramHandler.isApartmentEnabled(apartmentNumber) ? "true" : "false") + ",";
        json += "\"token\":\"" + String(escapeJSON(telegramHandler.getApartmentToken(apartmentNumber))) + "\",";
        json += "\"chatId\":\"" + String(escapeJSON(String((telegramHandler.getApartmentChatId(apartmentNumber))))) + "\"";
    }
    else
    {
        json += "\"enabled\":false,";
        json += "\"token\":\"\",";
        json += "\"chatId\":\"\"";
    }

    json += "}";

    _server.send(200, JSON_CONTENT_TYPE, json);
}

/**
 * Handle Save Telegram Configuration request
 */
void WebPortal::handleSaveTelegram()
{
    if (!authenticate(AuthLevel::ADMIN))
    {
        return;
    }

    if (!_server.hasArg("apartment"))
    {
        _server.send(400, JSON_CONTENT_TYPE, "{\"success\":false,\"message\":\"Apartment number is required\"}");
        return;
    }

    uint8_t apartmentNumber = _server.arg("apartment").toInt();

    // Check if this is a removal request
    if (_server.hasArg("action") && _server.arg("action") == "remove")
    {
        // Remove configuration
        if (telegramHandler.removeApartmentConfig(apartmentNumber))
        {
            _server.send(200, JSON_CONTENT_TYPE, "{\"success\":true,\"message\":\"Configuration removed successfully\"}");
        }
        else
        {
            _server.send(500, JSON_CONTENT_TYPE, "{\"success\":false,\"message\":\"Failed to remove configuration\"}");
        }
        return;
    }

    // Otherwise, proceed with saving/updating configuration
    if (!_server.hasArg("token") || !_server.hasArg("chatId"))
    {
        _server.send(400, JSON_CONTENT_TYPE, "{\"success\":false,\"message\":\"Token and Chat ID are required\"}");
        return;
    }

    String token = _server.arg("token");
    int64_t chatId = _server.arg("chatId").toInt(); // Changed from String to int64_t
    bool enabled = (_server.hasArg("enabled") && _server.arg("enabled") == "1");

    // Configure the apartment
    if (telegramHandler.configureApartment(apartmentNumber, token, chatId))
    {
        // Set enabled state
        if (enabled)
        {
            telegramHandler.enableApartment(apartmentNumber);
            // Send welcome message
            telegramHandler.sendServiceEnabledMessage(apartmentNumber);
        }
        else
        {
            telegramHandler.disableApartment(apartmentNumber);
        }

        _server.send(200, JSON_CONTENT_TYPE, "{\"success\":true,\"message\":\"Configuration saved successfully\"}");
    }
    else
    {
        _server.send(500, JSON_CONTENT_TYPE, "{\"success\":false,\"message\":\"Failed to save configuration: " + telegramHandler.getLastError() + "\"}");
    }
}

/**
 * Handle Save Building Configuration request
 */
void WebPortal::handleSaveBuildingConfig()
{
    if (!authenticate(AuthLevel::ADMIN))
    {
        return;
    }

    if (!_server.hasArg("buildingNumber"))
    {
        _server.send(400, JSON_CONTENT_TYPE, "{\"success\":false,\"message\":\"Building number is required\"}");
        return;
    }

    uint8_t buildingNumber = _server.arg("buildingNumber").toInt();
    String wifiConfigPassword = _server.arg("wifiConfigPassword");

    // Save building number
    if (WiFiManager::saveBuildingNumber(buildingNumber))
    {
        // Save WiFi config password
        setWiFiConfigPassword(wifiConfigPassword);

        _server.send(200, JSON_CONTENT_TYPE, "{\"success\":true,\"message\":\"Building configuration saved successfully\"}");
    }
    else
    {
        _server.send(500, JSON_CONTENT_TYPE, "{\"success\":false,\"message\":\"Failed to save building configuration\"}");
    }
}

/**
 * Handle Save Advanced Configuration request
 */
void WebPortal::handleSaveAdvancedConfig()
{
    if (!authenticate(AuthLevel::ADMIN))
    {
        return;
    }

    String action = _server.arg("action");

    if (action == "changePassword")
    {
        // Password change request
        if (!_server.hasArg("currentPassword") || !_server.hasArg("newPassword") || !_server.hasArg("confirmPassword"))
        {
            _server.send(400, JSON_CONTENT_TYPE, "{\"success\":false,\"message\":\"All password fields are required\"}");
            return;
        }

        String currentPassword = _server.arg("currentPassword");
        String newPassword = _server.arg("newPassword");
        String confirmPassword = _server.arg("confirmPassword");

        // Verify current password
        if (!verifyAdminPassword(currentPassword))
        {
            _server.send(400, JSON_CONTENT_TYPE, "{\"success\":false,\"message\":\"Current password is incorrect\"}");
            return;
        }

        // Verify new password matches confirmation
        if (newPassword != confirmPassword)
        {
            _server.send(400, JSON_CONTENT_TYPE, "{\"success\":false,\"message\":\"New passwords do not match\"}");
            return;
        }

        // Check new password length
        if (newPassword.length() < 6)
        {
            _server.send(400, JSON_CONTENT_TYPE, "{\"success\":false,\"message\":\"New password must be at least 6 characters\"}");
            return;
        }

        // Set new password
        setAdminPassword(newPassword);

        _server.send(200, JSON_CONTENT_TYPE, "{\"success\":true,\"message\":\"Password changed successfully\"}");
    }
    else if (action == "alarmSettings")
    {
        // Alarm settings change request
        if (!_server.hasArg("alarmDuration") || !_server.hasArg("alarmInterval") || !_server.hasArg("sensorSettlingTime"))
        {
            _server.send(400, JSON_CONTENT_TYPE, "{\"success\":false,\"message\":\"All alarm settings fields are required\"}");
            return;
        }

        uint32_t alarmDuration = _server.arg("alarmDuration").toInt();
        uint32_t alarmInterval = _server.arg("alarmInterval").toInt();
        uint32_t sensorSettlingTime = _server.arg("sensorSettlingTime").toInt();

        // Set alarm parameters
        alarmSystem.setAlarmDuration(alarmDuration);
        alarmSystem.setAlarmInterval(alarmInterval);
        alarmSystem.setSensorSettlingTime(sensorSettlingTime);

        _server.send(200, JSON_CONTENT_TYPE, "{\"success\":true,\"message\":\"Alarm settings saved successfully\"}");
    }
    else
    {
        _server.send(400, JSON_CONTENT_TYPE, "{\"success\":false,\"message\":\"Invalid action\"}");
    }
}

/**
 * Handle Toggle Apartment request
 */
void WebPortal::handleToggleApartment()
{
    if (!authenticate(AuthLevel::ADMIN))
    {
        return;
    }

    if (!_server.hasArg("apartment") || !_server.hasArg("type"))
    {
        _server.send(400, JSON_CONTENT_TYPE, "{\"success\":false,\"message\":\"Apartment number and type are required\"}");
        return;
    }

    uint8_t apartmentNumber = _server.arg("apartment").toInt();
    String type = _server.arg("type");

    if (type == "sensor")
    {
        // Toggle sensor
        bool currentState = alarmSystem.isSensorEnabled(apartmentNumber);
        bool newState = !currentState;

        if (newState)
        {
            if (alarmSystem.enableSensor(apartmentNumber))
            {
                _server.send(200, JSON_CONTENT_TYPE, "{\"success\":true,\"message\":\"Sensor enabled successfully\"}");
            }
            else
            {
                _server.send(500, JSON_CONTENT_TYPE, "{\"success\":false,\"message\":\"Failed to enable sensor\"}");
            }
        }
        else
        {
            if (alarmSystem.disableSensor(apartmentNumber))
            {
                _server.send(200, JSON_CONTENT_TYPE, "{\"success\":true,\"message\":\"Sensor disabled successfully\"}");
            }
            else
            {
                _server.send(500, JSON_CONTENT_TYPE, "{\"success\":false,\"message\":\"Failed to disable sensor\"}");
            }
        }
    }
    else if (type == "telegram")
    {
        // Toggle telegram
        bool currentState = telegramHandler.isApartmentEnabled(apartmentNumber);
        bool newState = !currentState;

        if (newState)
        {
            if (telegramHandler.enableApartment(apartmentNumber))
            {
                // Send notification
                telegramHandler.sendServiceEnabledMessage(apartmentNumber);
                _server.send(200, JSON_CONTENT_TYPE, "{\"success\":true,\"message\":\"Telegram notifications enabled successfully\"}");
            }
            else
            {
                _server.send(500, JSON_CONTENT_TYPE, "{\"success\":false,\"message\":\"Failed to enable Telegram notifications\"}");
            }
        }
        else
        {
            if (telegramHandler.disableApartment(apartmentNumber))
            {
                // Send notification
                telegramHandler.sendServiceDisabledMessage(apartmentNumber);
                _server.send(200, JSON_CONTENT_TYPE, "{\"success\":true,\"message\":\"Telegram notifications disabled successfully\"}");
            }
            else
            {
                _server.send(500, JSON_CONTENT_TYPE, "{\"success\":false,\"message\":\"Failed to disable Telegram notifications\"}");
            }
        }
    }
    else
    {
        _server.send(400, JSON_CONTENT_TYPE, "{\"success\":false,\"message\":\"Invalid type\"}");
    }
}

/**
 * Handle Reset Wire Cut request
 */
void WebPortal::handleResetWireCut()
{
    if (!authenticate(AuthLevel::ADMIN))
    {
        return;
    }

    // Reset all wire cut statuses
    alarmSystem.resetWireCutStatus(RIGHT_SIDE, RIGHT_BOX);
    alarmSystem.resetWireCutStatus(RIGHT_SIDE, LEFT_BOX);
    alarmSystem.resetWireCutStatus(LEFT_SIDE, RIGHT_BOX);
    alarmSystem.resetWireCutStatus(LEFT_SIDE, LEFT_BOX);
    alarmSystem.resetDistributionWireCutStatus(RIGHT_SIDE);
    alarmSystem.resetDistributionWireCutStatus(LEFT_SIDE);

    _server.send(200, JSON_CONTENT_TYPE, "{\"success\":true,\"message\":\"Wire cut detection reset successfully\"}");
}

/**
 * Handle Not Found request
 */
void WebPortal::handleNotFound()
{
    String message = "404 - Page Not Found\n\n";
    message += "URI: " + String(_server.uri()) + "\n";
    message += "Method: " + String(_server.method() == HTTP_GET ? "GET" : "POST") + "\n";

    _server.send(404, TEXT_CONTENT_TYPE, message);
}

/**
 * Set error message
 */
void WebPortal::setError(const String &error)
{
    _lastError = error;
}

/**
 * Clear error message
 */
void WebPortal::clearError()
{
    _lastError = "";
}

/**
 * Get client IP address
 */
String WebPortal::getClientIP()
{
    return _server.client().remoteIP().toString();
}

/**
 * Check if IP is local (private network)
 */
bool WebPortal::isLocalIP(const String &ip)
{
    // Check if IP starts with 192.168. or 10. or 172.16-31.
    if (ip.startsWith("192.168.") || ip.startsWith("10."))
    {
        return true;
    }

    if (ip.startsWith("172."))
    {
        // Extract the second octet
        int dotPos = ip.indexOf('.', 4);
        if (dotPos > 0)
        {
            int secondOctet = ip.substring(4, dotPos).toInt();
            if (secondOctet >= 16 && secondOctet <= 31)
            {
                return true;
            }
        }
    }

    return false;
}

WebPortal webPortal;
