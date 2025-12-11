// TelegramHandler.h

#ifndef TELEGRAM_HANDLER_H
#define TELEGRAM_HANDLER_H

#include <Arduino.h>
// #include <UniversalTelegramBot.h>
#include "CTBot.h"
#include <WiFiClientSecure.h>
#include "WiFiConfig.h"
#include "PinsConfig.h"
#include "TelegramMessages.h"

// Telegram Configuration Constants
#define MAX_APARTMENTS 24

// Message queue structure
struct QueuedMessage {
    String token;
    int64_t chatId;
    String message;
    uint8_t retries;
    uint32_t nextAttemptTime;
    bool inProgress;
};
    
// Alert Types for Different Scenarios
enum class AlertType {
    OWNER,              // Alert for the apartment being stolen from
    SAME_BOX,          // Alert for apartments in the same box
    ADJACENT_BOX,      // Alert for apartments in the adjacent box
    OTHER_SIDE         // Alert for apartments on the other side
};

// Constants for message sending
static const uint16_t HTTP_TIMEOUT = 1000;          // 1 second timeout
static const uint8_t MAX_RETRIES = 3;              // Max retries for failed messages
static const uint16_t RETRY_DELAY = 100;          // Delay between retries in ms
static const uint16_t RATE_LIMIT_DELAY = 500;       // Min delay between messages (ms)
static const uint8_t MAX_QUEUE_SIZE = 50;          // Maximum queue size


class TelegramHandler {
public:
    // Initialization
    static bool begin();
    static void end();
    
    // Connection Status
    static bool isReady();
    static String getLastError();
    
    // Apartment Configuration Management
    static bool configureApartment(uint8_t apartmentNumber, const String& token, int64_t chatId);
    static bool removeApartmentConfig(uint8_t apartmentNumber);
    static bool isApartmentConfigured(uint8_t apartmentNumber);
    static bool enableApartment(uint8_t apartmentNumber);
    static bool disableApartment(uint8_t apartmentNumber);
    static bool isApartmentEnabled(uint8_t apartmentNumber);
    static void loadAllConfigurations();
    static void saveAllConfigurations();
    
    // System Status Messages
    static bool sendSystemOnlineMessage(uint8_t apartmentNumber);
    static bool sendServiceEnabledMessage(uint8_t apartmentNumber);
    static bool sendServiceDisabledMessage(uint8_t apartmentNumber);
    static void sendSystemOnlineMessageToEnabledApartments();
    
    // Apartment Configuration Getters
    static String getApartmentToken(uint8_t apartmentNumber);
    static int64_t getApartmentChatId(uint8_t apartmentNumber);
    
    // Theft Alert Messages
    static bool sendTheftAlertToOwner(uint8_t apartmentNumber);
    static bool sendTheftAlertToSameBox(uint8_t targetApartment, uint8_t notifyApartment);
    static bool sendTheftAlertToAdjacentBox(uint8_t targetApartment, uint8_t notifyApartment);
    static bool sendTheftAlertToOtherSide(uint8_t targetApartment, uint8_t notifyApartment);
    
    // Wire Cut Alert Messages
    static bool sendWireCutAlert(BuildingSide side, BoxPosition box);
    static bool sendDistributionWireCutAlert(BuildingSide side);
    static bool sendStartupWireCutAlert(BuildingSide side, BoxPosition box);
    static bool sendStartupDistributionWireCutAlert(BuildingSide side);
    
    // Batch Message Sending
    static void sendTheftAlertsToAll(uint8_t targetApartment);
    static void sendWireCutAlertsToSide(BuildingSide side);
    static void sendStartupWireCutAlertsToSide(BuildingSide side);

    // Process pending messages - call this from the main loop
    static void update();

private:
    // Apartment Configuration Structure
    struct ApartmentConfig {
        String token;
        int64_t chatId;
        bool enabled;
        bool configured;
    };
    
    // Static Member Variables
    static ApartmentConfig _apartmentConfigs[MAX_APARTMENTS];
    static String _lastError;
    static WiFiClientSecure _client;
    static uint32_t _lastMessageTime;
    static bool _isInitialized;
    static CTBot _bot; // Added CTBot as a static member
    
    // Constants for Storage
    static const char* PREFERENCE_NAMESPACE;
    static const char* TOKEN_KEY_PREFIX;
    static const char* CHAT_ID_KEY_PREFIX;
    static const char* ENABLED_KEY_PREFIX;
    
    
    static QueuedMessage _messageQueue[MAX_QUEUE_SIZE];
    static uint8_t _queueHead;
    static uint8_t _queueTail;
    static uint8_t _queueSize;
    static bool _processingQueue;

    // Helper Methods
    static bool validateToken(const String& token);
    static bool validateChatId(int64_t chatId);
    static bool validateApartmentNumber(uint8_t apartmentNumber);
    
    // Message Sending Helpers
    static bool sendMessage(const String& token, int64_t chatId, const String& messageAR, const String& messageEN);
    static bool sendFormattedMessage(uint8_t apartmentNumber, const char* messageAR, const char* messageEN, ...);
    
    // Storage Helpers
    static void saveApartmentConfig(uint8_t apartmentNumber);
    static void loadApartmentConfig(uint8_t apartmentNumber);
    static String generateStorageKey(const char* prefix, uint8_t apartmentNumber);

    // Queue management methods
    static bool enqueueMessage(const String& token, int64_t chatId, const String& message);
    static bool processMessageQueue();
    static bool sendMessageWithTimeout(const String& token, int64_t chatId, const String& message);
};

// External declaration for global access
extern TelegramHandler telegramHandler;

#endif // TELEGRAM_HANDLER_H
