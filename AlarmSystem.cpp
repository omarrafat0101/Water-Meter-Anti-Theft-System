// AlarmSystem.cpp
// Implementation of the AlarmSystem class for the Water Meter Anti-Theft System

#include "AlarmSystem.h"
#include "PinsConfig.h"
#include "WiFiConfig.h"
#include "TelegramHandler.h"

// Initialize static member variables
AlarmSystemStatus AlarmSystem::_systemStatus = AlarmSystemStatus::NORMAL;
WireCutStatus AlarmSystem::_wireCutStatus = {false, true, false, true, false, true, false, true, false, true, false, true};
String AlarmSystem::_lastError = "";
bool AlarmSystem::_initialized = false;
uint32_t AlarmSystem::_startupTime = 0;
uint32_t AlarmSystem::_lastUpdateTime = 0;
uint32_t AlarmSystem::_lastAlarmTime = 0;
bool AlarmSystem::_sensorStates[TOTAL_APARTMENTS] = {false};
bool AlarmSystem::_enabledApartments[TOTAL_APARTMENTS] = {false};
bool AlarmSystem::_alarmActive[2] = {false, false}; // [RIGHT_SIDE, LEFT_SIDE]
uint32_t AlarmSystem::_alarmStartTime[2] = {0, 0};
bool AlarmSystem::_alarmState[2] = {false, false};
uint32_t AlarmSystem::_lastAlarmToggle[2] = {0, 0};
const char *AlarmSystem::PREFERENCE_NAMESPACE = "alarm_sys"; // Namespace for Preferences
Preferences AlarmSystem::_prefs;                             // Preferences object for storing state

// Configuration settings with default values
uint32_t AlarmSystem::_alarmDuration = ALARM_DURATION;
uint32_t AlarmSystem::_alarmInterval = ALARM_INTERVAL;
uint32_t AlarmSystem::_sensorSettlingTime = VCC_SETTLING_TIME;

// Add near the top of AlarmSystem.cpp with other static declarations
hw_timer_t *AlarmSystem::_rightSideTimer = NULL;
hw_timer_t *AlarmSystem::_leftSideTimer = NULL;
portMUX_TYPE AlarmSystem::_timerMux = portMUX_INITIALIZER_UNLOCKED;

// Global instance
AlarmSystem alarmSystem;

// ===== PUBLIC METHODS =====

bool AlarmSystem::begin()
{
    if (_initialized)
    {
        return true;
    }

    // Initialize sensor states and enabled apartments
    initializeSensorStates();

    // Load the enabled apartments (if existed) in initialization
    loadApartmentsState();

    // Set up timer for right side
    _rightSideTimer = timerBegin(1000000); // Timer with 1MHz resolution
    timerAttachInterrupt(_rightSideTimer, &AlarmSystem::rightSideTimerISR);
    timerAlarm(_rightSideTimer, _alarmInterval * 1000, true, 0); // Auto reload, unlimited
    timerStop(_rightSideTimer);                                  // Keep it stopped until alarm is activated

    // Set up timer for left side
    _leftSideTimer = timerBegin(1000000); // Timer with 1MHz resolution
    timerAttachInterrupt(_leftSideTimer, &AlarmSystem::leftSideTimerISR);
    timerAlarm(_leftSideTimer, _alarmInterval * 1000, true, 0); // Auto reload, unlimited
    timerStop(_leftSideTimer);                                  // Keep it stopped until alarm is activated

    // Record system startup time
    _startupTime = millis();
    _lastUpdateTime = _startupTime;

    // Initialize pins via the PinConfiguration class
    PinConfiguration::initializeAllPins();

    // Check for wire cuts at startup
    checkWireCutsAtStartup();

    // Handle startup wire cut notifications
    if (!_wireCutStatus.rightSideRightBoxEnabled)
    {
        telegramHandler.sendStartupWireCutAlert(RIGHT_SIDE, RIGHT_BOX);
        Serial.println("Startup Wire cut detected on right side, right box.");
    }
    if (!_wireCutStatus.rightSideLeftBoxEnabled)
    {
        telegramHandler.sendStartupWireCutAlert(RIGHT_SIDE, LEFT_BOX);
        Serial.println("Startup Wire cut detected on right side, left box.");
    }
    if (!_wireCutStatus.leftSideRightBoxEnabled)
    {
        telegramHandler.sendStartupWireCutAlert(LEFT_SIDE, RIGHT_BOX);
        Serial.println("Startup Wire cut detected on left side, right box.");
    }
    if (!_wireCutStatus.leftSideLeftBoxEnabled)
    {
        telegramHandler.sendStartupWireCutAlert(LEFT_SIDE, LEFT_BOX);
        Serial.println("Startup Wire cut detected on left side, left box.");
    }
    if (!_wireCutStatus.rightSideDistributionEnabled)
    {
        telegramHandler.sendStartupDistributionWireCutAlert(RIGHT_SIDE);
        Serial.println("Startup Wire cut detected on right side, distribution box.");
    }
    if (!_wireCutStatus.leftSideDistributionEnabled)
    {
        telegramHandler.sendStartupDistributionWireCutAlert(LEFT_SIDE);
        Serial.println("Startup Wire cut detected on left side, distribution box.");
    }

    _initialized = true;
    return true;
}

void AlarmSystem::end()
{
    if (!_initialized)
    {
        return;
    }

    // Stop all alarms
    stopAllAlarms();

    _initialized = false;
}

void AlarmSystem::update()
{
    if (!_initialized)
    {
        return;
    }

    // Get current time
    uint32_t currentTime = millis();

    // Update alarms (toggle on/off based on interval)
    updateAlarms();

    // Check wire cuts for both sides and boxes
    checkWireCuts();

    // Check sensors for theft detection
    checkSensors();

    // Update system status based on current conditions
    updateSystemStatus();

    // Record last update time
    _lastUpdateTime = currentTime;
}

AlarmSystemStatus AlarmSystem::getStatus()
{
    return _systemStatus;
}

bool AlarmSystem::enableSensor(uint8_t apartmentNumber)
{
    if (!isValidApartment(apartmentNumber))
    {
        setError("Invalid apartment number: " + String(apartmentNumber));
        return false;
    }

    uint8_t index = getApartmentIndex(apartmentNumber);
    if (index == 0xFF)
    {
        setError("Invalid apartment index for apartment: " + String(apartmentNumber));
        return false;
    }

    _enabledApartments[index] = true;
    // Save state for this specific apartment
    saveApartmentState(index);
    return true;
}

bool AlarmSystem::disableSensor(uint8_t apartmentNumber)
{
    if (!isValidApartment(apartmentNumber))
    {
        setError("Invalid apartment number: " + String(apartmentNumber));
        return false;
    }

    uint8_t index = getApartmentIndex(apartmentNumber);
    if (index == 0xFF)
    {
        setError("Invalid apartment index for apartment: " + String(apartmentNumber));
        return false;
    }

    _enabledApartments[index] = false;
    // Save state for this specific apartment
    saveApartmentState(index);
    return true;
}

bool AlarmSystem::isSensorEnabled(uint8_t apartmentNumber)
{
    if (!isValidApartment(apartmentNumber))
    {
        return false;
    }

    uint8_t index = getApartmentIndex(apartmentNumber);
    if (index == 0xFF)
    {
        return false;
    }

    return _enabledApartments[index];
}

bool AlarmSystem::isSensorTriggered(uint8_t apartmentNumber)
{
    if (!isValidApartment(apartmentNumber))
    {
        return false;
    }

    uint8_t index = getApartmentIndex(apartmentNumber);
    if (index == 0xFF)
    {
        return false;
    }

    return _sensorStates[index];
}

void AlarmSystem::activateAlarm(BuildingSide side)
{
    if (!isValidSide(side))
    {
        return;
    }

    uint8_t sideIndex = static_cast<uint8_t>(side);

    if (!_alarmActive[sideIndex])
    {
        _alarmActive[sideIndex] = true;
        _alarmStartTime[sideIndex] = millis();
        _alarmState[sideIndex] = true;

        // Initially turn on the siren
        Serial.println("Siren activated on side: " + String(sideIndex ? "Left" : "Right"));
        activateSiren(side);

        // Start the corresponding timer
        if (side == RIGHT_SIDE)
        {
            timerWrite(_rightSideTimer, 0); // Reset timer
            timerStart(_rightSideTimer);
        }
        else
        {
            timerWrite(_leftSideTimer, 0); // Reset timer
            timerStart(_leftSideTimer);
        }

        _lastAlarmTime = millis();
    }
}

void AlarmSystem::stopAlarm(BuildingSide side)
{
    if (!isValidSide(side))
    {
        return;
    }

    uint8_t sideIndex = static_cast<uint8_t>(side);

    if (_alarmActive[sideIndex])
    {
        _alarmActive[sideIndex] = false;
        _alarmState[sideIndex] = false;

        // Turn off the siren
        Serial.println("Siren deactivated on side: " + String(sideIndex ? "Left" : "Right"));
        stopSiren(side);

        // Stop the corresponding timer
        if (side == RIGHT_SIDE)
        {
            timerStop(_rightSideTimer);
        }
        else
        {
            timerStop(_leftSideTimer);
        }
    }
}

bool AlarmSystem::isAlarmActive(BuildingSide side)
{
    if (!isValidSide(side))
    {
        return false;
    }

    uint8_t sideIndex = static_cast<uint8_t>(side);
    return _alarmActive[sideIndex];
}

bool AlarmSystem::checkWireCut(BuildingSide side, BoxPosition box)
{
    if (!isValidSide(side) || !isValidBox(box))
    {
        return false;
    }

    // Get the appropriate cutoff wire pin
    uint8_t pin = getCutoffWirePin(side, box);

    // Read the pin (LOW means wire is cut)
    bool wireCut = (digitalRead(pin) == WIRE_CUT_TRIGGER);

    // Update wire cut status
    if (side == BuildingSide::RIGHT_SIDE)
    {
        if (box == BoxPosition::RIGHT_BOX)
        {
            _wireCutStatus.rightSideRightBox = wireCut;
            _wireCutStatus.rightSideRightBoxEnabled = !wireCut;
        }
        else
        {
            _wireCutStatus.rightSideLeftBox = wireCut;
            _wireCutStatus.rightSideLeftBoxEnabled = !wireCut;
        }
    }
    else
    {
        if (box == BoxPosition::RIGHT_BOX)
        {
            _wireCutStatus.leftSideRightBox = wireCut;
            _wireCutStatus.leftSideRightBoxEnabled = !wireCut;
        }
        else
        {
            _wireCutStatus.leftSideLeftBox = wireCut;
            _wireCutStatus.leftSideLeftBoxEnabled = !wireCut;
        }
    }

    return wireCut;
}

bool AlarmSystem::checkDistributionWireCut(BuildingSide side)
{
    if (!isValidSide(side))
    {
        return false;
    }

    // Get the appropriate distribution wire pin
    uint8_t pin = getDistributionWirePin(side);

    // Read the pin (LOW means wire is cut)
    bool wireCut = (digitalRead(pin) == WIRE_CUT_TRIGGER);

    // Update wire cut status
    if (side == BuildingSide::RIGHT_SIDE)
    {
        _wireCutStatus.rightSideDistribution = wireCut;
        _wireCutStatus.rightSideDistributionEnabled = !wireCut;
    }
    else
    {
        _wireCutStatus.leftSideDistribution = wireCut;
        _wireCutStatus.leftSideDistributionEnabled = !wireCut;
    }

    return wireCut;
}

void AlarmSystem::resetWireCutStatus(BuildingSide side, BoxPosition box)
{
    if (!isValidSide(side) || !isValidBox(box))
    {
        return;
    }

    // Reset the wire cut status for the specified side and box
    if (side == BuildingSide::RIGHT_SIDE)
    {
        if (box == BoxPosition::RIGHT_BOX)
        {
            _wireCutStatus.rightSideRightBox = false;
            _wireCutStatus.rightSideRightBoxEnabled = true;
        }
        else
        {
            _wireCutStatus.rightSideLeftBox = false;
            _wireCutStatus.rightSideLeftBoxEnabled = true;
        }
    }
    else
    {
        if (box == BoxPosition::RIGHT_BOX)
        {
            _wireCutStatus.leftSideRightBox = false;
            _wireCutStatus.leftSideRightBoxEnabled = true;
        }
        else
        {
            _wireCutStatus.leftSideLeftBox = false;
            _wireCutStatus.leftSideLeftBoxEnabled = true;
        }
    }
}

void AlarmSystem::resetDistributionWireCutStatus(BuildingSide side)
{
    if (!isValidSide(side))
    {
        return;
    }

    // Reset the distribution wire cut status for the specified side
    if (side == RIGHT_SIDE)
    {
        _wireCutStatus.rightSideDistribution = false;
        _wireCutStatus.rightSideDistributionEnabled = true;
    }
    else
    {
        _wireCutStatus.leftSideDistribution = false;
        _wireCutStatus.leftSideDistributionEnabled = true;
    }
}

WireCutStatus AlarmSystem::getWireCutStatus()
{
    return _wireCutStatus;
}

void AlarmSystem::setAlarmDuration(uint32_t duration)
{
    _alarmDuration = duration;
}

void AlarmSystem::setAlarmInterval(uint32_t interval)
{
    _alarmInterval = interval;
    // Update the timers with the new interval
    timerAlarm(_rightSideTimer, _alarmInterval * 1000, true, 0); // Auto reload, unlimited
    timerAlarm(_leftSideTimer, _alarmInterval * 1000, true, 0);  // Auto reload, unlimited
}

void AlarmSystem::setSensorSettlingTime(uint32_t time)
{
    _sensorSettlingTime = time;
}

void AlarmSystem::enableAllSensors()
{
    for (uint8_t apartment = 1; apartment <= TOTAL_APARTMENTS; apartment++)
    {
        enableSensor(apartment);
    }
}

void AlarmSystem::disableAllSensors()
{
    for (uint8_t apartment = 1; apartment <= TOTAL_APARTMENTS; apartment++)
    {
        disableSensor(apartment);
    }
}

bool AlarmSystem::performSystemCheck()
{
    bool allSystemsOK = true;

    // Check all cutoff wire pins
    if (checkWireCut(RIGHT_SIDE, RIGHT_BOX) ||
        checkWireCut(RIGHT_SIDE, LEFT_BOX) ||
        checkWireCut(LEFT_SIDE, RIGHT_BOX) ||
        checkWireCut(LEFT_SIDE, LEFT_BOX))
    {
        allSystemsOK = false;
    }

    // Check distribution wire pins
    if (checkDistributionWireCut(RIGHT_SIDE) ||
        checkDistributionWireCut(LEFT_SIDE))
    {
        allSystemsOK = false;
    }

    // Check if alarms can be triggered
    activateAlarm(RIGHT_SIDE);
    delay(100);
    stopAlarm(RIGHT_SIDE);

    activateAlarm(LEFT_SIDE);
    delay(100);
    stopAlarm(LEFT_SIDE);

    return allSystemsOK;
}

String AlarmSystem::getSystemStatus()
{
    String status = "Alarm System Status:\n";
    status += "  System: " + String(_initialized ? "Initialized" : "Not Initialized") + "\n";

    switch (_systemStatus)
    {
    case AlarmSystemStatus::NORMAL:
        status += "  Status: Normal\n";
        break;
    case AlarmSystemStatus::THEFT_DETECTED:
        status += "  Status: Theft Detected\n";
        break;
    case AlarmSystemStatus::WIRE_CUT_DETECTED:
        status += "  Status: Wire Cut Detected\n";
        break;
    }

    status += "  Uptime: " + String(getUptime() / 1000) + " seconds\n";
    status += "  Last Alarm: " + String(getLastAlarmTime() / 1000) + " seconds ago\n";

    status += "  Wire Cut Status:\n";
    status += "    Right Side, Right Box: " + String(_wireCutStatus.rightSideRightBox ? "Cut" : "OK") + "\n";
    status += "    Right Side, Left Box: " + String(_wireCutStatus.rightSideLeftBox ? "Cut" : "OK") + "\n";
    status += "    Left Side, Right Box: " + String(_wireCutStatus.leftSideRightBox ? "Cut" : "OK") + "\n";
    status += "    Left Side, Left Box: " + String(_wireCutStatus.leftSideLeftBox ? "Cut" : "OK") + "\n";
    status += "    Right Side Distribution: " + String(_wireCutStatus.rightSideDistribution ? "Cut" : "OK") + "\n";
    status += "    Left Side Distribution: " + String(_wireCutStatus.leftSideDistribution ? "Cut" : "OK") + "\n";

    status += "  Alarms:\n";
    status += "    Right Side: " + String(_alarmActive[0] ? "Active" : "Inactive") + "\n";
    status += "    Left Side: " + String(_alarmActive[1] ? "Active" : "Inactive") + "\n";

    status += "  Last Error: " + _lastError + "\n";

    return status;
}

String AlarmSystem::getLastError()
{
    return _lastError;
}

uint32_t AlarmSystem::getUptime()
{
    return millis() - _startupTime;
}

uint32_t AlarmSystem::getLastAlarmTime()
{
    return _lastAlarmTime > 0 ? (millis() - _lastAlarmTime) : 0;
}

uint8_t AlarmSystem::getActiveAlarmCount()
{
    return (_alarmActive[0] ? 1 : 0) + (_alarmActive[1] ? 1 : 0);
}

// ===== PRIVATE METHODS =====

void AlarmSystem::initializeSensorStates()
{
    // Initialize all sensor states to false (not triggered)
    for (uint8_t i = 0; i < TOTAL_APARTMENTS; i++)
    {
        _sensorStates[i] = false;
        // Initially disable all apartments (they'll be enabled individually or through EEPROM)
        _enabledApartments[i] = false;
    }
}

void AlarmSystem::checkSensors()
{
    static BuildingSide currentSide = RIGHT_SIDE;
    static uint32_t sideStartTime = 0;
    static bool sideInitialized = false;

    // Start checking new side
    if (!sideInitialized)
    {
        // Power the current side and disable the other side
        powerSide(currentSide, true);
        powerSide((currentSide == RIGHT_SIDE) ? LEFT_SIDE : RIGHT_SIDE, false);

        // Record start time
        sideStartTime = millis();
        sideInitialized = true;

        // Wait for sensors to stabilize before first reading
        delay(_sensorSettlingTime);
        return;
    }

    // Check if we're still within the 500ms window
    if (millis() - sideStartTime < 500)
    {
        // Check all sensors for the current side
        for (uint8_t apartment = 1; apartment <= TOTAL_APARTMENTS; apartment++)
        {
            uint8_t index = getApartmentIndex(apartment);
            if (index == 0xFF)
                continue;

            // Skip if apartment is not enabled or on different side
            if (!_enabledApartments[index] || getApartmentSide(apartment) != currentSide)
            {
                continue;
            }

            // Read the sensor
            if (readSensor(apartment) && !_sensorStates[index])
            {
                _sensorStates[index] = true;
                Serial.println("Sensor triggered for apartment: " + String(apartment) +
                               " on side: " + String(currentSide));
                // Handle theft detection
                handleTheftDetection(apartment);
            }
        }
    }
    else
    {
        // 500ms window completed, switch to other side
        currentSide = (currentSide == RIGHT_SIDE) ? LEFT_SIDE : RIGHT_SIDE;
        sideInitialized = false; // Reset for next side

        // Reset sensor states that weren't triggered during this window
        for (uint8_t apartment = 1; apartment <= TOTAL_APARTMENTS; apartment++)
        {
            uint8_t index = getApartmentIndex(apartment);
            if (index != 0xFF && getApartmentSide(apartment) == currentSide)
            {
                _sensorStates[index] = false;
            }
        }
    }
}

void AlarmSystem::checkWireCutsAtStartup()
{
    checkWireCut(BuildingSide::RIGHT_SIDE, BoxPosition::RIGHT_BOX);
    checkWireCut(BuildingSide::RIGHT_SIDE, BoxPosition::LEFT_BOX);
    checkWireCut(BuildingSide::LEFT_SIDE, BoxPosition::RIGHT_BOX);
    checkWireCut(BuildingSide::LEFT_SIDE, BoxPosition::LEFT_BOX);
    checkDistributionWireCut(BuildingSide::RIGHT_SIDE);
    checkDistributionWireCut(BuildingSide::LEFT_SIDE);
}

void AlarmSystem::checkWireCuts()
{
    // Check each box wire
    if (_wireCutStatus.rightSideRightBoxEnabled && checkWireCut(RIGHT_SIDE, RIGHT_BOX))
    {
        handleWireCutDetection(RIGHT_SIDE, RIGHT_BOX);
    }

    if (_wireCutStatus.rightSideLeftBoxEnabled && checkWireCut(RIGHT_SIDE, LEFT_BOX))
    {
        handleWireCutDetection(RIGHT_SIDE, LEFT_BOX);
    }

    if (_wireCutStatus.leftSideRightBoxEnabled && checkWireCut(LEFT_SIDE, RIGHT_BOX))
    {
        handleWireCutDetection(LEFT_SIDE, RIGHT_BOX);
    }

    if (_wireCutStatus.leftSideLeftBoxEnabled && checkWireCut(LEFT_SIDE, LEFT_BOX))
    {
        handleWireCutDetection(LEFT_SIDE, LEFT_BOX);
    }

    // Check distribution wires
    if (_wireCutStatus.rightSideDistributionEnabled && checkDistributionWireCut(RIGHT_SIDE))
    {
        handleDistributionWireCutDetection(RIGHT_SIDE);
    }

    if (_wireCutStatus.leftSideDistributionEnabled && checkDistributionWireCut(LEFT_SIDE))
    {
        handleDistributionWireCutDetection(LEFT_SIDE);
    }
}

void AlarmSystem::updateAlarms()
{
    // Update right side alarm if active
    if (_alarmActive[0])
    {
        uint32_t alarmElapsed = millis() - _alarmStartTime[0];

        // Check if alarm duration has expired
        if (alarmElapsed >= _alarmDuration)
        {
            stopAlarm(RIGHT_SIDE);
        }
    }

    // Update left side alarm if active
    if (_alarmActive[1])
    {
        uint32_t alarmElapsed = millis() - _alarmStartTime[1];

        // Check if alarm duration has expired
        if (alarmElapsed >= _alarmDuration)
        {
            stopAlarm(LEFT_SIDE);
        }
    }
}

void AlarmSystem::handleTheftDetection(uint8_t apartmentNumber)
{
    if (!isValidApartment(apartmentNumber))
    {
        return;
    }

    // Send Telegram notifications
    telegramHandler.sendTheftAlertsToAll(apartmentNumber);

    // Activate the alarm on the side where theft was detected
    BuildingSide side = getApartmentSide(apartmentNumber);
    activateAlarm(side);

    // Update system status
    _systemStatus = AlarmSystemStatus::THEFT_DETECTED;
}

void AlarmSystem::handleWireCutDetection(BuildingSide side, BoxPosition box)
{
    // Activate the alarm on the affected side
    activateAlarm(side);

    // Send notifications
    telegramHandler.sendWireCutAlert(side, box);

    // Update system status
    _systemStatus = AlarmSystemStatus::WIRE_CUT_DETECTED;
}

void AlarmSystem::handleDistributionWireCutDetection(BuildingSide side)
{
    // Send notifications
    telegramHandler.sendDistributionWireCutAlert(side);

    // Activate the alarm on the affected side
    activateAlarm(side);

    // Update system status
    _systemStatus = AlarmSystemStatus::WIRE_CUT_DETECTED;
}

void AlarmSystem::updateAlarmState(BuildingSide side)
{
    if (!isValidSide(side))
    {
        return;
    }

    uint8_t sideIndex = static_cast<uint8_t>(side);

    if (_alarmActive[sideIndex] && shouldToggleAlarm(side))
    {
        toggleAlarm(side);
    }
}

void AlarmSystem::toggleAlarm(BuildingSide side)
{
    uint8_t sideIndex = static_cast<uint8_t>(side);

    // Toggle the alarm state
    _alarmState[sideIndex] = !_alarmState[sideIndex];

    // Control the physical siren based on the new state
    if (_alarmState[sideIndex])
    {
        activateSiren(side);
    }
    else
    {
        stopSiren(side);
    }
}

bool AlarmSystem::shouldToggleAlarm(BuildingSide side)
{
    if (!isValidSide(side))
    {
        return false;
    }

    uint8_t sideIndex = static_cast<uint8_t>(side);
    uint32_t now = millis();

    // Check if interval has elapsed since last toggle
    return (now - _lastAlarmToggle[sideIndex] >= _alarmInterval);
}

bool AlarmSystem::isValidApartment(uint8_t apartmentNumber)
{
    return (apartmentNumber >= 1 && apartmentNumber <= TOTAL_APARTMENTS);
}

bool AlarmSystem::isValidSide(BuildingSide side)
{
    return (side == RIGHT_SIDE || side == LEFT_SIDE);
}

bool AlarmSystem::isValidBox(BoxPosition box)
{
    return (box == RIGHT_BOX || box == LEFT_BOX);
}

bool AlarmSystem::readSensor(uint8_t apartmentNumber)
{
    if (!isValidApartment(apartmentNumber))
    {
        return false;
    }

    // Get the pin for this apartment's sensor
    uint8_t sensorPin = getVibrationSensorPin(apartmentNumber);

    // Read the sensor (LOW means vibration detected)
    return (digitalRead(sensorPin) == VIBRATION_TRIGGER_LEVEL);
}

void AlarmSystem::powerSide(BuildingSide side, bool enable)
{
    uint8_t pin = getVCCControlPin(side);

    // Send LOW to enable VCC on the side (NPN transistor)
    digitalWrite(pin, enable ? LOW : HIGH);
}

void AlarmSystem::setError(const String &error)
{
    _lastError = error;
    Serial.println("ERROR: " + error);
}

void AlarmSystem::clearError()
{
    _lastError = "";
}

void AlarmSystem::updateSystemStatus()
{
    // Check for active alarms or wire cuts
    if (_alarmActive[0] || _alarmActive[1])
    {
        // Keep current status (theft or wire cut)
    }
    else if (_wireCutStatus.rightSideRightBox ||
             _wireCutStatus.rightSideLeftBox ||
             _wireCutStatus.leftSideRightBox ||
             _wireCutStatus.leftSideLeftBox ||
             _wireCutStatus.rightSideDistribution ||
             _wireCutStatus.leftSideDistribution)
    {
        _systemStatus = AlarmSystemStatus::WIRE_CUT_DETECTED;
    }
    else
    {
        // Check for any triggered sensors
        bool anyTriggered = false;
        for (uint8_t i = 0; i < TOTAL_APARTMENTS; i++)
        {
            if (_sensorStates[i])
            {
                anyTriggered = true;
                break;
            }
        }

        if (anyTriggered)
        {
            _systemStatus = AlarmSystemStatus::THEFT_DETECTED;
        }
        else
        {
            _systemStatus = AlarmSystemStatus::NORMAL;
        }
    }
}

void AlarmSystem::saveApartmentState(uint8_t apartmentIndex)
{
    // Begin Preferences with namespace
    // Close any existing Preferences instance
    _prefs.end();

    // Try to begin new Preferences session
    if (!_prefs.begin(PREFERENCE_NAMESPACE, false))
    {
        setError("Failed to begin Preferences");
        return;
    }

    // Calculate which byte contains our apartment's bit
    uint8_t byteIndex = apartmentIndex / 8;
    uint8_t bitPosition = apartmentIndex % 8;

    // Read the current byte
    uint8_t states[(TOTAL_APARTMENTS + 7) / 8] = {0};
    _prefs.getBytes("apt_states", states, sizeof(states));

    // Update the specific bit for this apartment
    if (_enabledApartments[apartmentIndex])
    {
        states[byteIndex] |= (1 << bitPosition); // Set bit
    }
    else
    {
        states[byteIndex] &= ~(1 << bitPosition); // Clear bit
    }

    // Save the updated states back to Preferences
    _prefs.putBytes("apt_states", states, sizeof(states));

    // Close Preferences
    _prefs.end();
}

void AlarmSystem::loadApartmentsState()
{
    // Begin Preferences with namespace
    if (!_prefs.begin(PREFERENCE_NAMESPACE, true))
    { // true for readonly
        setError("Failed to begin Preferences");
        return;
    }

    // Calculate size needed for storing all apartments
    size_t stateSize = (TOTAL_APARTMENTS + 7) / 8;
    uint8_t states[stateSize];

    // Read the states from Preferences
    if (_prefs.getBytes("apt_states", states, stateSize))
    {
        // Unpack the bits into boolean array
        for (uint8_t i = 0; i < TOTAL_APARTMENTS; i++)
        {
            _enabledApartments[i] = (states[i / 8] & (1 << (i % 8))) != 0;
        }
    }

    // Close Preferences
    _prefs.end();
}

void ARDUINO_ISR_ATTR AlarmSystem::rightSideTimerISR()
{
    portENTER_CRITICAL_ISR(&_timerMux);
    toggleAlarm(RIGHT_SIDE);
    portEXIT_CRITICAL_ISR(&_timerMux);
}

void ARDUINO_ISR_ATTR AlarmSystem::leftSideTimerISR()
{
    portENTER_CRITICAL_ISR(&_timerMux);
    toggleAlarm(LEFT_SIDE);
    portEXIT_CRITICAL_ISR(&_timerMux);
}