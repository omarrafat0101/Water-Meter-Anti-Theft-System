// pins_config.h

#ifndef PINS_CONFIG_H
#define PINS_CONFIG_H

#include <Arduino.h>
#include "ApartmentGrouping.h"

// Alarm System Pins
#define RIGHT_SIDE_SIREN_PIN 13
#define LEFT_SIDE_SIREN_PIN 0

// Siren Configuration
#define SIREN_ACTIVATE LOW // Relay triggers on LOW
#define ALARM_DURATION 20000   // 20 seconds
#define ALARM_INTERVAL 1000    // 1 second ON, 1 second OFF

// VCC Control Transistor Pins
#define RIGHT_SIDE_VCC_PIN 32
#define LEFT_SIDE_VCC_PIN 21
#define VCC_SETTLING_TIME 100  // 100ms to let sensors stabilize

// Cutoff Wire Detection Pins
enum CutoffWirePins {
    RIGHT_SIDE_RIGHT_BOX = 22,
    RIGHT_SIDE_LEFT_BOX = 14,
    LEFT_SIDE_RIGHT_BOX = 27,
    LEFT_SIDE_LEFT_BOX = 26,
    RIGHT_SIDE_DISTRIBUTION = 25,
    LEFT_SIDE_DISTRIBUTION = 33
};
#define WIRE_CUT_TRIGGER HIGH  // Wire cut triggers on HIGH  

// Vibration Sensor Pins (Each pin connects to two sensors)
const uint8_t VIBRATION_SENSOR_PINS[] = {
    // Right Side (Left Box) and Left Side (Left Box) sensors
    15,  // Apt 1 (Right Side, Left Box, Bottom) and Apt 3 (Left Side, Left Box, Bottom)
    2,   // Apt 5 (Right Side, Left Box) and Apt 7 (Left Side, Left Box)
    4,   // Apt 9 (Right Side, Left Box) and Apt 11 (Left Side, Left Box)
    16,   // Apt 13 (Right Side, Left Box) and Apt 15 (Left Side, Left Box)
    17,  // Apt 17 (Right Side, Left Box) and Apt 19 (Left Side, Left Box)
    5,  // Apt 21 (Right Side, Left Box, Top) and Apt 23 (Left Side, Left Box, Top)

    // Right Side (Right Box) and Left Side (Right Box) sensors
    19,   // Apt 2 (Right Side, Right Box, Bottom) and Apt 4 (Left Side, Right Box, Bottom)
    23,  // Apt 6 (Right Side, Right Box) and Apt 8 (Left Side, Right Box)
    36,  // Apt 10 (Right Side, Right Box) and Apt 12 (Left Side, Right Box)
    39,  // Apt 14 (Right Side, Right Box) and Apt 16 (Left Side, Right Box)
    34,  // Apt 18 (Right Side, Right Box) and Apt 20 (Left Side, Right Box)
    35   // Apt 22 (Right Side, Right Box, Top) and Apt 24 (Left Side, Right Box, Top)
};

// Sensor Configuration
#define VIBRATION_TRIGGER_LEVEL HIGH  // Sensor triggers on HIGH
#define NUM_SENSORS_PER_SIDE 12



// Class Declaration
class PinConfiguration {
public:
    static void initializeAllPins();

private:
    static void initializeSirenPins();
    static void initializeVCCControlPins();
    static void initializeCutoffWirePins();
    static void initializeVibrationSensorPins();
};

// Pin Getter Functions
uint8_t getVibrationSensorPin(uint8_t apartmentNumber);
uint8_t getCutoffWirePin(BuildingSide side, BoxPosition box);
uint8_t getDistributionWirePin(BuildingSide side);
uint8_t getVCCControlPin(BuildingSide side);
uint8_t getSirenPin(BuildingSide side);

// Control Functions
void enableSideVCC(BuildingSide side);
void disableSideVCC(BuildingSide side);
void activateSiren(BuildingSide side);
void stopSiren(BuildingSide side);

// Sensor Reading Functions
bool readVibrationSensor(uint8_t apartmentNumber);
bool readCutoffWire(BuildingSide side, BoxPosition box);
bool readDistributionWire(BuildingSide side);



#endif // PINS_CONFIG_H
