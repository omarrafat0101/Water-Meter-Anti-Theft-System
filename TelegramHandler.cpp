// TelegramHandler.cpp

#include "TelegramHandler.h"
#include <Preferences.h>
#include <stdarg.h>

// Add debug logging macro for Telegram operations
#ifdef TELEGRAM_DEBUG
#define TELEGRAM_LOG(format, ...) Serial.printf("[Telegram Debug] " format "\n", ##__VA_ARGS__)
#else
#define TELEGRAM_LOG(format, ...)
#endif

// Static member initialization
TelegramHandler::ApartmentConfig TelegramHandler::_apartmentConfigs[MAX_APARTMENTS];
String TelegramHandler::_lastError = "";
WiFiClientSecure TelegramHandler::_client;
uint32_t TelegramHandler::_lastMessageTime = 0;
bool TelegramHandler::_isInitialized = false;
CTBot TelegramHandler::_bot; // Initialize the static CTBot member
const char *TelegramHandler::PREFERENCE_NAMESPACE = "telegram";
const char *TelegramHandler::TOKEN_KEY_PREFIX = "token_";
const char *TelegramHandler::CHAT_ID_KEY_PREFIX = "chatid_";
const char *TelegramHandler::ENABLED_KEY_PREFIX = "enabled_";

QueuedMessage TelegramHandler::_messageQueue[MAX_QUEUE_SIZE];
uint8_t TelegramHandler::_queueHead = 0;
uint8_t TelegramHandler::_queueTail = 0;
uint8_t TelegramHandler::_queueSize = 0;
bool TelegramHandler::_processingQueue = false;

// Initialization
bool TelegramHandler::begin()
{
  if (_isInitialized)
    return true;

  Serial.println(F("[Telegram] Initializing..."));
  TELEGRAM_LOG("Debug logging is enabled");

  // Load saved configurations
  loadAllConfigurations();
  TELEGRAM_LOG("Loaded configurations from storage");

  // We'll use the first configured apartment's token
  String token = "";
  for (uint8_t i = 0; i < MAX_APARTMENTS; i++)
  {
    if (_apartmentConfigs[i].configured)
    {
      token = _apartmentConfigs[i].token;
      break;
    }
  }

  if (token.isEmpty())
  {
    Serial.println(F("[Telegram] Warning: No configured token found"));
    token = "DEFAULT_TOKEN"; // You might want to set a default token or handle this differently
  }

  // set the telegram bot token - using the static _bot instance
  _bot.setTelegramToken(token);
  TELEGRAM_LOG("Telegram bot token set: %s", token.c_str());

  _isInitialized = true;
  Serial.println(F("[Telegram] Initialization complete"));
  TELEGRAM_LOG("Initialization successful");
  return true;
}

void TelegramHandler::end()
{
  _isInitialized = false;
}

// Connection Status
bool TelegramHandler::isReady()
{
  return _isInitialized && WiFiManager::isConnected();
}

String TelegramHandler::getLastError()
{
  return _lastError;
}

// Apartment Configuration Management
bool TelegramHandler::configureApartment(uint8_t apartmentNumber, const String &token, int64_t chatId)
{
  Serial.printf("[Telegram] Configuring apartment %d\n", apartmentNumber);

  if (!validateApartmentNumber(apartmentNumber))
  {
    _lastError = "Invalid apartment number";
    Serial.println(F("[Telegram] Error: Invalid apartment number"));
    return false;
  }

  if (!validateToken(token))
  {
    _lastError = "Invalid token format";
    Serial.println(F("[Telegram] Error: Invalid token format"));
    return false;
  }

  if (!validateChatId(chatId))
  {
    _lastError = "Invalid chat ID format";
    Serial.println(F("[Telegram] Error: Invalid chat ID format"));
    return false;
  }

  uint8_t index = apartmentNumber - 1;
  _apartmentConfigs[index].token = token;
  _apartmentConfigs[index].chatId = chatId;
  _apartmentConfigs[index].configured = true;

  // Save the configuration
  saveApartmentConfig(apartmentNumber);

  Serial.printf("[Telegram] Apartment %d configured successfully\n", apartmentNumber);
  return true;
}

bool TelegramHandler::removeApartmentConfig(uint8_t apartmentNumber)
{
  if (!validateApartmentNumber(apartmentNumber))
  {
    _lastError = "Invalid apartment number";
    return false;
  }

  uint8_t index = apartmentNumber - 1;
  _apartmentConfigs[index].token = "";
  _apartmentConfigs[index].chatId = 0;
  _apartmentConfigs[index].enabled = false;
  _apartmentConfigs[index].configured = false;

  // Save the empty configuration
  saveApartmentConfig(apartmentNumber);

  return true;
}

bool TelegramHandler::isApartmentConfigured(uint8_t apartmentNumber)
{
  if (!validateApartmentNumber(apartmentNumber))
  {
    return false;
  }

  uint8_t index = apartmentNumber - 1;
  return _apartmentConfigs[index].configured;
}

bool TelegramHandler::enableApartment(uint8_t apartmentNumber)
{
  Serial.printf("[Telegram] Enabling apartment %d\n", apartmentNumber);

  if (!validateApartmentNumber(apartmentNumber))
  {
    _lastError = "Invalid apartment number";
    return false;
  }

  uint8_t index = apartmentNumber - 1;
  if (!_apartmentConfigs[index].configured)
  {
    _lastError = "Apartment not configured";
    return false;
  }

  _apartmentConfigs[index].enabled = true;
  saveApartmentConfig(apartmentNumber);

  // Send notification about service activation
  sendServiceEnabledMessage(apartmentNumber);

  return true;
}

bool TelegramHandler::disableApartment(uint8_t apartmentNumber)
{
  if (!validateApartmentNumber(apartmentNumber))
  {
    _lastError = "Invalid apartment number";
    return false;
  }

  uint8_t index = apartmentNumber - 1;
  if (!_apartmentConfigs[index].configured)
  {
    _lastError = "Apartment not configured";
    return false;
  }

  _apartmentConfigs[index].enabled = false;
  saveApartmentConfig(apartmentNumber);

  // Send notification about service deactivation
  sendServiceDisabledMessage(apartmentNumber);

  return true;
}

bool TelegramHandler::isApartmentEnabled(uint8_t apartmentNumber)
{
  if (!validateApartmentNumber(apartmentNumber))
  {
    return false;
  }

  uint8_t index = apartmentNumber - 1;
  return _apartmentConfigs[index].configured && _apartmentConfigs[index].enabled;
}

void TelegramHandler::loadAllConfigurations()
{
  Serial.println(F("[Telegram] Loading all configurations from storage"));

  Preferences prefs;
  if (prefs.begin(PREFERENCE_NAMESPACE, true))
  {
    for (uint8_t i = 1; i <= MAX_APARTMENTS; i++)
    {
      loadApartmentConfig(i);
    }
    prefs.end();
  }

  Serial.println(F("[Telegram] Configurations loaded successfully"));
}

void TelegramHandler::saveAllConfigurations()
{
  for (uint8_t i = 1; i <= MAX_APARTMENTS; i++)
  {
    if (_apartmentConfigs[i - 1].configured)
    {
      saveApartmentConfig(i);
    }
  }
}

// System Status Messages
bool TelegramHandler::sendSystemOnlineMessage(uint8_t apartmentNumber)
{
  if (!isApartmentEnabled(apartmentNumber))
    return false;

  String hostname = String(WiFiManager::getHostname()) + "-Building-" + String(WiFiManager::getBuildingNumber());
  return sendFormattedMessage(
      apartmentNumber,
      SYSTEM_ONLINE_MSG_AR,
      SYSTEM_ONLINE_MSG_EN,
      apartmentNumber,
      hostname.c_str()
  );
}

void TelegramHandler::sendSystemOnlineMessageToEnabledApartments()
{
  for (uint8_t apt = 0; apt < MAX_APARTMENTS; apt++)
  {
    if (isApartmentEnabled(apt))
    {
      sendSystemOnlineMessage(apt);
    }
  }
}

bool TelegramHandler::sendServiceEnabledMessage(uint8_t apartmentNumber)
{
  if (!isApartmentConfigured(apartmentNumber))
    return false;

  return sendFormattedMessage(
      apartmentNumber,
      SERVICE_ENABLED_AR,
      SERVICE_ENABLED_EN,
      apartmentNumber);
}

bool TelegramHandler::sendServiceDisabledMessage(uint8_t apartmentNumber)
{
  if (!isApartmentConfigured(apartmentNumber))
    return false;

  return sendFormattedMessage(
      apartmentNumber,
      SERVICE_DISABLED_AR,
      SERVICE_DISABLED_EN,
      apartmentNumber);
}

// Theft Alert Messages
bool TelegramHandler::sendTheftAlertToOwner(uint8_t apartmentNumber)
{
  if (!isApartmentEnabled(apartmentNumber))
    return false;

  uint8_t index = apartmentNumber - 1;
  return sendMessage(
      _apartmentConfigs[index].token,
      _apartmentConfigs[index].chatId,
      THEFT_ALERT_OWNER_AR,
      THEFT_ALERT_OWNER_EN);
}

bool TelegramHandler::sendTheftAlertToSameBox(uint8_t targetApartment, uint8_t notifyApartment)
{
  if (!isApartmentEnabled(notifyApartment))
    return false;

  return sendFormattedMessage(
      notifyApartment,
      THEFT_ALERT_SAME_BOX_AR,
      THEFT_ALERT_SAME_BOX_EN,
      targetApartment);
}

bool TelegramHandler::sendTheftAlertToAdjacentBox(uint8_t targetApartment, uint8_t notifyApartment)
{
  if (!isApartmentEnabled(notifyApartment))
    return false;

  return sendFormattedMessage(
      notifyApartment,
      THEFT_ALERT_ADJACENT_BOX_AR,
      THEFT_ALERT_ADJACENT_BOX_EN,
      targetApartment);
}

bool TelegramHandler::sendTheftAlertToOtherSide(uint8_t targetApartment, uint8_t notifyApartment)
{
  if (!isApartmentEnabled(notifyApartment))
    return false;

  return sendFormattedMessage(
      notifyApartment,
      THEFT_ALERT_OTHER_SIDE_AR,
      THEFT_ALERT_OTHER_SIDE_EN,
      targetApartment);
}

// Wire Cut Alert Messages
bool TelegramHandler::sendWireCutAlert(BuildingSide side, BoxPosition box)
{
  Serial.printf("[Telegram] Sending wire cut alert for side: %d, box: %d\n", (int)side, (int)box);

  // Get all apartments on this side
  uint8_t apartments[12];
  uint8_t count = 0;

  for (uint8_t i = 0; i < TOTAL_APARTMENTS; i++)
  {
    uint8_t aptNum = i + 1;
    uint8_t index = getApartmentIndex(aptNum);

    if (index != 0xFF &&
        APARTMENT_LOCATIONS[index].side == side &&
        isApartmentEnabled(aptNum))
    {
      apartments[count++] = aptNum;
    }
  }

  // Send message to all enabled apartments on this side
  bool success = true;
  for (uint8_t i = 0; i < count; i++)
  {
    uint8_t aptNum = apartments[i];
    uint8_t index = aptNum - 1;

    if (!sendMessage(
            _apartmentConfigs[index].token,
            _apartmentConfigs[index].chatId,
            SENSOR_WIRE_CUT_ALERT_AR,
            SENSOR_WIRE_CUT_ALERT_EN))
    {
      success = false;
    }
  }

  return success;
}

bool TelegramHandler::sendDistributionWireCutAlert(BuildingSide side)
{

  // Get all apartments on this side
  uint8_t apartments[12];
  uint8_t count = 0;

  for (uint8_t i = 0; i < TOTAL_APARTMENTS; i++)
  {
    uint8_t aptNum = i + 1;
    uint8_t index = getApartmentIndex(aptNum);

    if (index != 0xFF &&
        APARTMENT_LOCATIONS[index].side == side &&
        isApartmentEnabled(aptNum))
    {
      apartments[count++] = aptNum;
    }
  }

  // Send message to all enabled apartments on this side
  bool success = true;
  for (uint8_t i = 0; i < count; i++)
  {
    uint8_t aptNum = apartments[i];
    uint8_t index = aptNum - 1;

    if (!sendMessage(
            _apartmentConfigs[index].token,
            _apartmentConfigs[index].chatId,
            DIST_CTRL_WIRE_CUT_ALERT_AR,
            DIST_CTRL_WIRE_CUT_ALERT_EN))
    {
      success = false;
    }
  }

  return success;
}

bool TelegramHandler::sendStartupWireCutAlert(BuildingSide side, BoxPosition box)
{

  // Get all apartments in this specific box on this side
  uint8_t apartments[6]; // Max 6 apartments per box
  uint8_t count = 0;

  for (uint8_t i = 0; i < TOTAL_APARTMENTS; i++)
  {
    uint8_t aptNum = i + 1;
    uint8_t index = getApartmentIndex(aptNum);

    if (index != 0xFF &&
        APARTMENT_LOCATIONS[index].side == side &&
        APARTMENT_LOCATIONS[index].box == box &&
        isApartmentEnabled(aptNum))
    {
      apartments[count++] = aptNum;
    }
  }

  // Send message to all enabled apartments in this box
  bool success = true;
  for (uint8_t i = 0; i < count; i++)
  {
    uint8_t aptNum = apartments[i];
    uint8_t index = aptNum - 1;

    if (!sendMessage(
            _apartmentConfigs[index].token,
            _apartmentConfigs[index].chatId,
            STARTUP_SENSOR_WIRE_CUT_AR,
            STARTUP_SENSOR_WIRE_CUT_EN))
    {
      success = false;
    }
  }

  return success;
}

bool TelegramHandler::sendStartupDistributionWireCutAlert(BuildingSide side)
{

  // Get all apartments on this side
  uint8_t apartments[12];
  uint8_t count = 0;

  for (uint8_t i = 0; i < TOTAL_APARTMENTS; i++)
  {
    uint8_t aptNum = i + 1;
    uint8_t index = getApartmentIndex(aptNum);

    if (index != 0xFF &&
        APARTMENT_LOCATIONS[index].side == side &&
        isApartmentEnabled(aptNum))
    {
      apartments[count++] = aptNum;
    }
  }

  // Send message to all enabled apartments on this side
  bool success = true;
  for (uint8_t i = 0; i < count; i++)
  {
    uint8_t aptNum = apartments[i];
    uint8_t index = aptNum - 1;

    if (!sendMessage(
            _apartmentConfigs[index].token,
            _apartmentConfigs[index].chatId,
            STARTUP_DIST_CTRL_WIRE_CUT_AR,
            STARTUP_DIST_CTRL_WIRE_CUT_EN))
    {
      success = false;
    }
  }

  return success;
}

// Batch Message Sending
void TelegramHandler::sendTheftAlertsToAll(uint8_t targetApartment)
{
  Serial.printf("[Telegram] Sending theft alerts for apartment %d to all recipients\n", targetApartment);

  // Send alert to owner first
  sendTheftAlertToOwner(targetApartment);

  // Get apartment index and location
  uint8_t targetIndex = getApartmentIndex(targetApartment);
  if (targetIndex == 0xFF)
    return;

  BuildingSide targetSide = APARTMENT_LOCATIONS[targetIndex].side;
  BoxPosition targetBox = APARTMENT_LOCATIONS[targetIndex].box;

  // Find apartments in the same box
  uint8_t sameBoxApts[6];
  uint8_t sameBoxCount = 0;
  getApartmentsInSameBox(targetApartment, sameBoxApts, sameBoxCount);

  // Find apartments in the adjacent box
  uint8_t adjacentBoxApts[6];
  uint8_t adjacentBoxCount = 0;
  // For adjacent box, we need to get apartments on the same side but different box
  for (uint8_t i = 0; i < TOTAL_APARTMENTS; i++)
  {
    uint8_t aptNum = i + 1;
    uint8_t index = getApartmentIndex(aptNum);

    if (index != 0xFF &&
        APARTMENT_LOCATIONS[index].side == targetSide &&
        APARTMENT_LOCATIONS[index].box != targetBox)
    {
      adjacentBoxApts[adjacentBoxCount++] = aptNum;
    }
  }

  // Find apartments on the other side
  uint8_t otherSideApts[12];
  uint8_t otherSideCount = 0;
  BuildingSide otherSide = (targetSide == BuildingSide::RIGHT_SIDE) ? BuildingSide::LEFT_SIDE : BuildingSide::RIGHT_SIDE;

  for (uint8_t i = 0; i < TOTAL_APARTMENTS; i++)
  {
    uint8_t aptNum = i + 1;
    uint8_t index = getApartmentIndex(aptNum);

    if (index != 0xFF && APARTMENT_LOCATIONS[index].side == otherSide)
    {
      otherSideApts[otherSideCount++] = aptNum;
    }
  }

  // Send messages to apartments in the same box
  for (uint8_t i = 0; i < sameBoxCount; i++)
  {
    uint8_t aptNum = sameBoxApts[i];
    if (aptNum != targetApartment && isApartmentEnabled(aptNum))
    {
      sendTheftAlertToSameBox(targetApartment, aptNum);
    }
  }

  // Send messages to apartments in the adjacent box
  for (uint8_t i = 0; i < adjacentBoxCount; i++)
  {
    uint8_t aptNum = adjacentBoxApts[i];
    if (isApartmentEnabled(aptNum))
    {
      sendTheftAlertToAdjacentBox(targetApartment, aptNum);
    }
  }

  // Send messages to apartments on the other side
  for (uint8_t i = 0; i < otherSideCount; i++)
  {
    uint8_t aptNum = otherSideApts[i];
    if (isApartmentEnabled(aptNum))
    {
      sendTheftAlertToOtherSide(targetApartment, aptNum);
    }
  }
}

void TelegramHandler::sendWireCutAlertsToSide(BuildingSide side)
{
  // For a wire cut, we need to notify all apartments on that side
  sendWireCutAlert(side, BoxPosition::RIGHT_BOX); // Box position doesn't matter here
}

void TelegramHandler::sendStartupWireCutAlertsToSide(BuildingSide side)
{
  // For a startup wire cut, we need to notify all apartments on that side
  sendStartupWireCutAlert(side, BoxPosition::RIGHT_BOX); // Box position doesn't matter here
}

// Private Helper Methods
bool TelegramHandler::validateToken(const String &token)
{
  // Basic validation - token should be at least 20 chars and contain numbers and letters
  if (token.length() < 20)
    return false;

  bool hasDigit = false;
  bool hasLetter = false;

  for (unsigned int i = 0; i < token.length(); i++)
  {
    if (isDigit(token[i]))
      hasDigit = true;
    if (isAlpha(token[i]))
      hasLetter = true;
  }

  return hasDigit && hasLetter;
}

bool TelegramHandler::validateChatId(int64_t chatId)
{
  // Basic validation for chat ID
  // Chat IDs are large numbers, typically negative for groups/channels
  // For simplicity, we'll just verify it's not zero
  return chatId != 0;
}

bool TelegramHandler::validateApartmentNumber(uint8_t apartmentNumber)
{
  return (apartmentNumber >= 1 && apartmentNumber <= MAX_APARTMENTS);
}

// Get apartment token
String TelegramHandler::getApartmentToken(uint8_t apartmentNumber)
{
  uint8_t index = getApartmentIndex(apartmentNumber);
  if (!validateApartmentNumber(apartmentNumber) || !_apartmentConfigs[index].configured)
  {
    return "";
  }

  if (index == 0xFF)
    return "";
  // Return the token for the specified apartment number
  return _apartmentConfigs[index].token;
}

// Get apartment chat ID
int64_t TelegramHandler::getApartmentChatId(uint8_t apartmentNumber)
{
  uint8_t index = getApartmentIndex(apartmentNumber);
  if (!validateApartmentNumber(apartmentNumber) || !_apartmentConfigs[index].configured)
  {
    return 0;
  }
  if (index == 0xFF)
    return 0;
  // Return the chat ID for the specified apartment number
  return _apartmentConfigs[index].chatId;
}

// Message Sending Helpers: add to queue
bool TelegramHandler::sendMessage(const String &token, int64_t chatId, const String &messageAR, const String &messageEN)
{
  String fullMessage = messageAR + "\n\n" + messageEN;
  Serial.printf("[Telegram] Queueing message to chat ID: %s\n", String(chatId));

  // Instead of sending directly, add to queue
  return enqueueMessage(token, chatId, fullMessage);
}

bool TelegramHandler::sendFormattedMessage(uint8_t apartmentNumber, const char *messageAR, const char *messageEN, ...)
{
  if (!isApartmentEnabled(apartmentNumber))
    return false;

  uint8_t index = apartmentNumber - 1;
  String token = _apartmentConfigs[index].token;
  int64_t chatId = _apartmentConfigs[index].chatId;

  // Format the messages with variable arguments
  va_list args;
  char formattedAR[512];
  char formattedEN[512];

  va_start(args, messageEN);

  va_list args_copy;
  va_copy(args_copy, args);

  vsnprintf(formattedAR, sizeof(formattedAR), messageAR, args);
  vsnprintf(formattedEN, sizeof(formattedEN), messageEN, args_copy);

  va_end(args_copy);
  va_end(args);

  return sendMessage(token, chatId, String(formattedAR), String(formattedEN));
}

// Storage Helpers
void TelegramHandler::saveApartmentConfig(uint8_t apartmentNumber)
{
  if (!validateApartmentNumber(apartmentNumber))
    return;

  uint8_t index = apartmentNumber - 1;
  Preferences prefs;

  if (prefs.begin(PREFERENCE_NAMESPACE, false))
  {
    String tokenKey = generateStorageKey(TOKEN_KEY_PREFIX, apartmentNumber);
    String chatIdKey = generateStorageKey(CHAT_ID_KEY_PREFIX, apartmentNumber);
    String enabledKey = generateStorageKey(ENABLED_KEY_PREFIX, apartmentNumber);

    prefs.putString(tokenKey.c_str(), _apartmentConfigs[index].token);
    // Store the int64_t chatId as a string
    prefs.putString(chatIdKey.c_str(), String(_apartmentConfigs[index].chatId));
    prefs.putBool(enabledKey.c_str(), _apartmentConfigs[index].enabled);

    prefs.end();
  }
}

void TelegramHandler::loadApartmentConfig(uint8_t apartmentNumber)
{
  if (!validateApartmentNumber(apartmentNumber))
    return;

  uint8_t index = apartmentNumber - 1;
  Preferences prefs;

  if (prefs.begin(PREFERENCE_NAMESPACE, true))
  {
    String tokenKey = generateStorageKey(TOKEN_KEY_PREFIX, apartmentNumber);
    String chatIdKey = generateStorageKey(CHAT_ID_KEY_PREFIX, apartmentNumber);
    String enabledKey = generateStorageKey(ENABLED_KEY_PREFIX, apartmentNumber);

    _apartmentConfigs[index].token = prefs.getString(tokenKey.c_str(), "");
    // Convert stored string back to int64_t
    String chatIdStr = prefs.getString(chatIdKey.c_str(), "0");
    _apartmentConfigs[index].chatId = atoll(chatIdStr.c_str());
    _apartmentConfigs[index].enabled = prefs.getBool(enabledKey.c_str(), false);
    _apartmentConfigs[index].configured = (_apartmentConfigs[index].token.length() > 0 && _apartmentConfigs[index].chatId != 0);

    prefs.end();
  }
}

String TelegramHandler::generateStorageKey(const char *prefix, uint8_t apartmentNumber)
{
  return String(prefix) + String(apartmentNumber);
}

// Queue management implementation
bool TelegramHandler::enqueueMessage(const String &token, int64_t chatId, const String &message)
{
  if (_queueSize >= MAX_QUEUE_SIZE)
  {
    _lastError = "Message queue is full";
    Serial.println(F("[Telegram] Error: Message queue is full"));
    return false;
  }

  // Check for duplicate message in queue
  uint8_t current = _queueHead;
  for (uint8_t i = 0; i < _queueSize; i++)
  {
    if (_messageQueue[current].token == token &&
        _messageQueue[current].chatId == chatId &&
        _messageQueue[current].message == message)
    {
      Serial.println(F("[Telegram] Duplicate message skipped"));
      return true;
    }
    current = (current + 1) % MAX_QUEUE_SIZE;
  }

  // No duplicate found, add message to queue
  _messageQueue[_queueTail].token = token;
  _messageQueue[_queueTail].chatId = chatId;
  _messageQueue[_queueTail].message = message;
  _messageQueue[_queueTail].retries = 0;
  _messageQueue[_queueTail].nextAttemptTime = millis();

  _queueTail = (_queueTail + 1) % MAX_QUEUE_SIZE;
  _queueSize++;

  Serial.printf("[Telegram] Message queued. Queue size: %d\n", _queueSize);
  return true;
}

bool TelegramHandler::sendMessageWithTimeout(const String &token, int64_t chatId, const String &message)
{
  // Set the bot's token for this message
  _bot.setTelegramToken(token);

  // Check for low memory condition
  if (ESP.getFreeHeap() < 10000)
  {
    TELEGRAM_LOG("Critical: Low memory condition detected: %d bytes", ESP.getFreeHeap());
    ESP.restart();
  }

  // Send message with timeout
  int32_t messageId = _bot.sendMessage(chatId, message);
  _lastMessageTime = millis();

  // Parse response handling various conditions
  if (messageId == 0)
  {
    // Get the last response from CTBot library
    String lastResponse = _bot.getLastResponse();

    // Check for rate limiting (HTTP 429)
    if (lastResponse.indexOf("\"error_code\":429") > 0)
    {
      // Parse the retry_after value
      int retryAfter = 60; // Default to 60 seconds if we can't parse
      int retryPos = lastResponse.indexOf("\"retry_after\":");
      if (retryPos > 0)
      {
        retryAfter = lastResponse.substring(retryPos + 14).toInt();
      }

      _lastError = "Rate limited by Telegram, retry after " + String(retryAfter) + " seconds";
      Serial.println(_lastError);

      // You could update your queue to respect this retry time
      return false;
    }
    // Check for chat not found (user blocked bot)
    else if (lastResponse.indexOf("\"error_code\":400") > 0 &&
             lastResponse.indexOf("chat not found") > 0)
    {
      _lastError = "Chat not found (user may have blocked the bot)";
      Serial.println(_lastError);
      return false;
    }
    // Check for unauthorized (invalid token)
    else if (lastResponse.indexOf("\"error_code\":401") > 0)
    {
      _lastError = "Unauthorized (invalid token)";
      Serial.println(_lastError);
      return false;
    }
    // Other errors
    else
    {
      _lastError = "Failed to send message: " + lastResponse;
      Serial.println(_lastError);
      return false;
    }
  }

  return true;
}

bool TelegramHandler::processMessageQueue()
{
  if (_queueSize == 0 || _processingQueue)
  {
    return true;
  }

  _processingQueue = true;

  uint32_t currentTime = millis();

  // Rate limiting - ensure minimum delay between messages
  if (currentTime - _lastMessageTime < RATE_LIMIT_DELAY)
  {
    _processingQueue = false;
    return true;
  }

  // Process the message at queue head
  QueuedMessage &msg = _messageQueue[_queueHead];

  // Skip if not ready to retry
  if (currentTime < msg.nextAttemptTime)
  {
    _processingQueue = false;
    return true;
  }

  // Try to send the message
  bool success = sendMessageWithTimeout(msg.token, msg.chatId, msg.message);

  if (success)
  {
    // Message sent successfully, remove from queue
    _queueHead = (_queueHead + 1) % MAX_QUEUE_SIZE;
    _queueSize--;
    Serial.printf("Message sent successfully. Queue size: %d\n", _queueSize);
  }
  else
  {
    // Message failed
    msg.retries++;

    if (msg.retries >= MAX_RETRIES)
    {
      // Max retries reached, remove from queue
      _queueHead = (_queueHead + 1) % MAX_QUEUE_SIZE;
      _queueSize--;
      Serial.println("Message failed after max retries");
    }
    else
    {
      // Schedule retry with dynamic backoff
      // Check if the error contains rate limit information
      if (_lastError.indexOf("Rate limited") >= 0)
      {
        // Extract retry_after from error
        int retryAfter = 60; // Default
        int pos = _lastError.indexOf("retry after ");
        if (pos > 0)
        {
          retryAfter = _lastError.substring(pos + 12).toInt();
        }
        msg.nextAttemptTime = currentTime + (retryAfter * 1000);
        Serial.printf("Rate limited. Will retry in %d seconds\n", retryAfter);
      }
      else
      {
        // Standard exponential backoff
        uint16_t backoff = RETRY_DELAY * (1 << msg.retries);
        msg.nextAttemptTime = currentTime + backoff;
        Serial.printf("Message failed. Will retry in %d ms\n", backoff);
      }
    }
  }

  _processingQueue = false;
  return success;
}

// Update method to be called from main loop
void TelegramHandler::update()
{
  if (isReady())
  {
    processMessageQueue();
  }
}

// Create a global instance
TelegramHandler telegramHandler;
