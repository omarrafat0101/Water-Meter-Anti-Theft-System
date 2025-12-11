#ifndef ALARM_SYSTEM_H
#define ALARM_SYSTEM_H

#include <Arduino.h>
#include "PinsConfig.h"
#include <Preferences.h>
#include "WiFiConfig.h"
#include "TelegramHandler.h"
#include "esp32-hal-timer.h"

// Alarm System Status Flags
enum class AlarmSystemStatus
{
    NORMAL,            // System running normally
    THEFT_DETECTED,    // Theft detected on one or more meters
    WIRE_CUT_DETECTED, // Wire cut detected
};

// Wire Cut Status Structure
struct WireCutStatus
{
    bool rightSideRightBox : 1;
    bool rightSideRightBoxEnabled : 1;

    bool rightSideLeftBox : 1;
    bool rightSideLeftBoxEnabled : 1;

    bool leftSideRightBox : 1;
    bool leftSideRightBoxEnabled : 1;

    bool leftSideLeftBox : 1;
    bool leftSideLeftBoxEnabled : 1;

    bool rightSideDistribution : 1;
    bool rightSideDistributionEnabled : 1;

    bool leftSideDistribution : 1;
    bool leftSideDistributionEnabled : 1;
};

class AlarmSystem
{
public:
    // Initialization
    static bool begin();
    static void end();

    // Main System Operation
    static void update();                 // Main loop function to be called regularly
    static AlarmSystemStatus getStatus(); // Get current system status

    // Sensor Management
    static bool enableSensor(uint8_t apartmentNumber);
    static bool disableSensor(uint8_t apartmentNumber);
    static bool isSensorEnabled(uint8_t apartmentNumber);
    static bool isSensorTriggered(uint8_t apartmentNumber);

    // Alarm Control
    static void activateAlarm(BuildingSide side);
    static void stopAlarm(BuildingSide side);
    static void stopAllAlarms();
    static bool isAlarmActive(BuildingSide side);

    // Wire Cut Detection
    static bool checkWireCut(BuildingSide side, BoxPosition box);
    static bool checkDistributionWireCut(BuildingSide side);
    static void resetWireCutStatus(BuildingSide side, BoxPosition box);
    static void resetDistributionWireCutStatus(BuildingSide side);
    static WireCutStatus getWireCutStatus();

    // System Configuration
    static void setAlarmDuration(uint32_t duration);
    static void setAlarmInterval(uint32_t interval);
    static void setSensorSettlingTime(uint32_t time);
    static void enableAllSensors();
    static void disableAllSensors();
    static void checkSensors();
    static bool readSensor(uint8_t apartmentNumber);
    static void powerSide(BuildingSide side, bool enable);

    // Diagnostic Functions
    static bool performSystemCheck();
    static String getSystemStatus();
    static String getLastError();
    static uint32_t getUptime();
    static uint32_t getLastAlarmTime();
    static uint8_t getActiveAlarmCount();

private:
    // Private member variables
    static AlarmSystemStatus _systemStatus;
    static WireCutStatus _wireCutStatus;
    static String _lastError;
    static bool _initialized;
    static uint32_t _startupTime;
    static uint32_t _lastUpdateTime;
    static uint32_t _lastAlarmTime;
    static bool _sensorStates[TOTAL_APARTMENTS];
    static bool _enabledApartments[TOTAL_APARTMENTS];
    static bool _alarmActive[2]; // [RIGHT_SIDE, LEFT_SIDE]
    static uint32_t _alarmStartTime[2];
    static bool _alarmState[2];
    static uint32_t _lastAlarmToggle[2];
    static Preferences _prefs;
    static const char *PREFERENCE_NAMESPACE; // Namespace for Preferences

    static hw_timer_t *_rightSideTimer;
    static hw_timer_t *_leftSideTimer;
    static portMUX_TYPE _timerMux;

    // Configuration settings
    static uint32_t _alarmDuration;
    static uint32_t _alarmInterval;
    static uint32_t _sensorSettlingTime;

    // Private helper methods
    static void initializeSensorStates();
    // static void checkSensors();
    static void checkWireCutsAtStartup();
    static void checkWireCuts();
    static void updateAlarms();
    static void handleTheftDetection(uint8_t apartmentNumber);
    static void handleWireCutDetection(BuildingSide side, BoxPosition box);
    static void handleDistributionWireCutDetection(BuildingSide side);

    // Alarm timing management
    static void updateAlarmState(BuildingSide side);
    static void toggleAlarm(BuildingSide side);
    static bool shouldToggleAlarm(BuildingSide side);

    // Validation helpers
    static bool isValidApartment(uint8_t apartmentNumber);
    static bool isValidSide(BuildingSide side);
    static bool isValidBox(BoxPosition box);


    // Error handling
    static void setError(const String &error);
    static void clearError();

    // System state management
    static void updateSystemStatus();
    static void saveApartmentState(uint8_t apartmentIndex); // Save the current state of enabled apartments to EEPROM
    static void loadApartmentsState();                      // Load the enabled apartments in startup

    static void IRAM_ATTR rightSideTimerISR();
    static void IRAM_ATTR leftSideTimerISR();
};

// External declaration for global access
extern AlarmSystem alarmSystem;

#endif // ALARM_SYSTEM_H