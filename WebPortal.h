// WebPortal.h
#ifndef WEB_PORTAL_H
#define WEB_PORTAL_H

#include <Arduino.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include "WiFiConfig.h"
#include "TelegramHandler.h"
#include "AlarmSystem.h"
#include "PinsConfig.h"
#include "ApartmentGrouping.h"

// Web Portal Configuration
#define WEB_SERVER_PORT 80
#define ADMIN_SESSION_TIMEOUT 300000 // 5 minutes timeout for admin session
#define WIFI_CONFIG_SESSION_TIMEOUT 180000 // 3 minutes timeout for WiFi config session

// HTML Content Types
#define HTML_CONTENT_TYPE "text/html"
#define JSON_CONTENT_TYPE "application/json"
#define TEXT_CONTENT_TYPE "text/plain"

// Page Routes
#define ROUTE_ROOT "/"
#define ROUTE_DASHBOARD "/dashboard"
#define ROUTE_WIFI_CONFIG "/wifi-config"
#define ROUTE_ADMIN_LOGIN "/admin"
#define ROUTE_ADMIN_PANEL "/admin-panel"
#define ROUTE_ADMIN_LOGOUT "/admin-logout"
#define ROUTE_API_STATUS "/api/status"
#define ROUTE_API_APARTMENT_STATUS "/api/apartment-status"
#define ROUTE_API_WIRE_STATUS "/api/wire-status"
#define ROUTE_API_ALARM_STATUS "/api/alarm-status"
#define ROUTE_SCAN_NETWORKS "/scan-networks"
#define ROUTE_SAVE_WIFI "/save-wifi"
#define ROUTE_ADMIN_TELEGRAM_CONFIG "/admin/telegram"
#define ROUTE_ADMIN_SAVE_TELEGRAM "/admin/save-telegram"
#define ROUTE_ADMIN_BUILDING_CONFIG "/admin/building"
#define ROUTE_ADMIN_SAVE_BUILDING "/admin/save-building"
#define ROUTE_ADMIN_APARTMENT_TOGGLE "/admin/toggle-apartment"
#define ROUTE_ADMIN_ADVANCED_CONFIG "/admin/advanced"
#define ROUTE_ADMIN_SAVE_ADVANCED "/admin/save-advanced"

// Authentication levels
enum class AuthLevel {
    NONE,             // Public - no authentication required
    WIFI_CONFIG,      // WiFi configuration - requires WiFi password
    ADMIN             // Admin - requires admin password
};

// Session structure
struct Session {
    String id;
    AuthLevel level;
    uint32_t expiry;  // Session expiry timestamp
    bool isValid() const { return millis() < expiry; }
};

class WebPortal {
public:
    // Initialization and setup
    static bool begin();
    static void end();
    static void update();  // Call this from loop()
    
    // Server status
    static bool isRunning();
    static String getLastError();
    
    // Admin authentication
    static void setAdminPassword(const String& password);
    static String getAdminPassword();
    static bool verifyAdminPassword(const String& password);
    
    // WiFi configuration password
    static void setWiFiConfigPassword(const String& password);
    static String getWiFiConfigPassword();
    static bool verifyWiFiConfigPassword(const String& password);
    
    // Building configuration
    static void setBuildingNumber(uint8_t number);
    static uint8_t getBuildingNumber();
    
    // Session management
    static bool hasValidSession(AuthLevel level);
    static void invalidateSession(AuthLevel level);
    static void checkSessionTimeouts();
    
    // Handle requests directly
    static void handleClient();
    
    // Status reporting
    static String getSystemStatusJSON();
    static String getApartmentStatusJSON();
    static String getWireStatusJSON();
    static String getAlarmStatusJSON();
    
    // Authentication state
    static bool isAdminLoggedIn();
    static bool isWiFiConfigActive();
    
    // Event handlers
    static void onSystemStatusUpdate(void (*callback)());
    static void onAdminLogin(void (*callback)());
    static void onAdminLogout(void (*callback)());
    static void onWiFiConfigStart(void (*callback)());
    static void onWiFiConfigSave(void (*callback)());

private:
    // Server instance
    static WebServer _server;
    
    // Authentication
    static String _adminPassword;
    static String _wiFiConfigPassword;
    static Session _adminSession;
    static Session _wiFiConfigSession;
    
    // State tracking
    static bool _initialized;
    static String _lastError;
    static uint32_t _startTime;
    static uint32_t _pageRequests;
    static uint32_t _lastClientActivity;
    static void (*_onStatusUpdateCallback)();
    static void (*_onAdminLoginCallback)();
    static void (*_onAdminLogoutCallback)();
    static void (*_onWiFiConfigStartCallback)();
    static void (*_onWiFiConfigSaveCallback)();
    
    // Helper methods
    static void setupRoutes();
    static void serveStatic(const String& path, const char* contentType, const char* content);
    static bool authenticate(AuthLevel requiredLevel);
    static String createSession(AuthLevel level);
    static String generateSessionId();
    static bool validateSession(const String& sessionId, AuthLevel level);
    static void updateLastActivity();
    
    // Common HTML components
    static String getHTMLHeader(const String& title, bool includeRefresh = false);
    static String getHTMLFooter();
    static String getCommonCSS();
    static String getCommonJavaScript();
    static String getNavigationMenu(bool isAdmin);
    static String getSystemStatusWidget();
    
    // Main page handlers
    static void handleRoot();
    static void handleDashboard();
    static void handleWiFiConfigPage();
    static void handleAdminLogin();
    static void handleAdminPanel();
    static void handleAdminLogout();
    static void handleFavicon();
    
    // API endpoints
    static void handleAPIStatus();
    static void handleAPIApartmentStatus();
    static void handleAPIWireStatus();
    static void handleAPIAlarmStatus();
    
    // Admin page handlers
    static void handleAdminTelegramConfig();
    static void handleAdminBuildingConfig();
    static void handleAdminAdvancedConfig();
    
    // Action handlers
    static void handleScanNetworks();
    static void handleSaveWiFi();
    static void handleSaveTelegram();
    static void handleGetTelegramConfig();
    static void handleSaveBuildingConfig();
    static void handleSaveAdvancedConfig();
    static void handleToggleApartment();
    static void handleResetWireCut();
    
    // Error handling
    static void handleNotFound();
    static void setError(const String& error);
    static void clearError();
    
    // Security helpers
    static String getClientIP();
    static void logAccess(const String& route, const String& clientIP, bool authenticated);
    static bool isLocalIP(const String& ip);
    
    // HTML content builders
    static String buildDashboardContent();
    static String buildWiFiConfigContent();
    static String buildAdminLoginContent();
    static String buildAdminPanelContent();
    static String buildTelegramConfigContent();
    static String buildBuildingConfigContent();
    static String buildAdvancedConfigContent();
    static String buildApartmentRowsHTML();
    static String buildSensorStatusTable();
    static String buildWireCutStatusTable();
    static String buildAlarmStatusTable();
    
    // Session management
    static void saveSessionToPreferences();
    static void loadSessionFromPreferences();
    static const char* PREFERENCE_NAMESPACE;
    static const char* ADMIN_PASSWORD_KEY;
    static const char* WIFI_CONFIG_PASSWORD_KEY;
};

// External declaration for global access
extern WebPortal webPortal;

#endif // WEB_PORTAL_H