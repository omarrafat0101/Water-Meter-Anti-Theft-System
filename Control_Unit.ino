/**
   Water Meter Anti-Theft System

   This system monitors vibration sensors attached to water meters in a building
   to detect potential theft attempts. When vibration is detected, an alarm is triggered
   and notifications are sent via Telegram.

   The system also detects wire-cutting attempts and provides a web interface
   for configuration and monitoring.

   Author: Omar Raafat
   Date: 2025-04-11
*/

// Enable Telegram debug logging
#define TELEGRAM_DEBUG

#include <Arduino.h>
#include "PinsConfig.h"
#include "WiFiConfig.h"
#include "TelegramHandler.h"
#include "AlarmSystem.h"
#include "WebPortal.h"
#include "ApartmentGrouping.h"
#include <esp_task_wdt.h>


void setup() {
  // Initialize Serial for debugging
  Serial.begin(9600);
  Serial.println(F("\n\n--- Water Meter Anti-Theft System Starting ---"));

  // Initialize GPIO pins
  Serial.println(F("Initializing pins..."));
  PinConfiguration::initializeAllPins();


  // Initialize Telegram handler
  Serial.println(F("Initializing Telegram handler..."));
  if (!telegramHandler.begin()) {
    Serial.print(F("Failed to initialize Telegram handler: "));
    Serial.println(telegramHandler.getLastError());
    // Continue anyway - the system must work even without Telegram
  }
  
  // Initialize the alarm system (must be before WiFi to ensure alarm works even without WiFi)
  Serial.println(F("Initializing alarm system..."));
  if (!alarmSystem.begin()) {
    Serial.print(F("Failed to initialize alarm system: "));
    Serial.println(alarmSystem.getLastError());
  }

  // Initialize WiFi connection
  Serial.println(F("Initializing WiFi..."));
  if (!WiFiManager::begin()) {
    Serial.print(F("Failed to initialize WiFi: "));
    Serial.println(WiFiManager::getLastError());
    // Continue anyway - the system must work even without WiFi
  }

  

  // Initialize web portal
  Serial.println(F("Initializing web portal..."));
  if (!webPortal.begin()) {
    Serial.print(F("Failed to initialize web portal: "));
    Serial.println(webPortal.getLastError());
    // Continue anyway - the system must work even without web portal
  }

  // Set up callbacks
  WiFiManager::setOnConnectCallback([]() {
    Serial.println(F("WiFi connected"));
    // Send online notification to all enabled apartments
    telegramHandler.sendSystemOnlineMessageToEnabledApartments();
  });

   // First, ensure no watchdog is already configured
  esp_task_wdt_deinit();
  // Create and initialize the watchdog timer configuration structure
  esp_task_wdt_config_t wdt_config = {
    .timeout_ms = 35000,           // timeout (in milliseconds)
    .idle_core_mask = (1 << 0),    // Bitmask of cores to watch (core 0)
    .trigger_panic = true          // Trigger a panic when timeout (causing a reboot)
  };
  
  // Initialize the Task Watchdog Timer (TWDT) with the given configuration
  esp_err_t err = esp_task_wdt_init(&wdt_config);
  if (err != ESP_OK) {
    Serial.print(F("Failed to initialize TWDT: "));
    Serial.println(err);
    return;
  }
  
  // Add current task to the watchdog
  err = esp_task_wdt_add(NULL);
  if (err != ESP_OK) {
    Serial.print(F("Failed to add task to TWDT: "));
    Serial.println(err);
    return;
  }
  
  
  Serial.println(F("System initialization complete"));
}

void loop() {
  // Reset the watchdog timer regularly when things are working correctly
  esp_task_wdt_reset();
  
  // Update WiFi connection (non-blocking)
  WiFiManager::handleConnection();

  // Update alarm system (higher priority than other tasks)
  alarmSystem.update();

  // Update Telegram handler (process message queue)
  telegramHandler.update();

  // Update web portal (handle client requests)
  webPortal.update();
  
  // Yield to allow for WiFi processing, especially in non-blocking mode
  yield();
}
