#include "PinsConfig.h"
#include <Arduino.h>

// Initialize all hardware components
void PinConfiguration::initializeAllPins() {
    initializeSirenPins();
    initializeVCCControlPins();
    initializeCutoffWirePins();
    initializeVibrationSensorPins();
}

// Initialize Siren Relay Pins
void PinConfiguration::initializeSirenPins() {
    pinMode(RIGHT_SIDE_SIREN_PIN, OUTPUT);
    pinMode(LEFT_SIDE_SIREN_PIN, OUTPUT);
    
    // Initialize sirens to OFF state
    digitalWrite(RIGHT_SIDE_SIREN_PIN, !SIREN_ACTIVATE);
    digitalWrite(LEFT_SIDE_SIREN_PIN, !SIREN_ACTIVATE);
}

// Initialize VCC Control Transistor Pins
void PinConfiguration::initializeVCCControlPins() {
    pinMode(RIGHT_SIDE_VCC_PIN, OUTPUT);
    pinMode(LEFT_SIDE_VCC_PIN, OUTPUT);
    Serial.println("VCC Control Pins Initialized");
    
    // Initialize both sides to OFF (HIGH for NPN transistors)
    digitalWrite(RIGHT_SIDE_VCC_PIN, HIGH);
    digitalWrite(LEFT_SIDE_VCC_PIN, HIGH);
    Serial.println("VCC Control Pins Set to HIGH (OFF)");
}

// Initialize Cutoff Wire Detection Pins
void PinConfiguration::initializeCutoffWirePins() {
    // Setup all cutoff wire pins as INPUT_PULLUP
    pinMode(CutoffWirePins::RIGHT_SIDE_RIGHT_BOX, INPUT_PULLUP);
    pinMode(CutoffWirePins::RIGHT_SIDE_LEFT_BOX, INPUT_PULLUP);
    pinMode(CutoffWirePins::LEFT_SIDE_RIGHT_BOX, INPUT_PULLUP);
    pinMode(CutoffWirePins::LEFT_SIDE_LEFT_BOX, INPUT_PULLUP);
    pinMode(CutoffWirePins::RIGHT_SIDE_DISTRIBUTION, INPUT_PULLUP);
    pinMode(CutoffWirePins::LEFT_SIDE_DISTRIBUTION, INPUT_PULLUP);
}

// Initialize Vibration Sensor Pins
void PinConfiguration::initializeVibrationSensorPins() {
    // Initialize all vibration sensor pins as INPUT
    for(uint8_t i = 0; i < sizeof(VIBRATION_SENSOR_PINS) / sizeof(VIBRATION_SENSOR_PINS[0]); i++) {
        pinMode(VIBRATION_SENSOR_PINS[i], INPUT);
    }
}

// Global function to initialize all pins
void initializeHardware() {
    PinConfiguration::initializeAllPins();
}

// Get pin number for a specific apartment's vibration sensor
uint8_t getVibrationSensorPin(uint8_t apartmentNumber) {
    uint8_t index = getApartmentIndex(apartmentNumber);
    if(index == 0xFF) return 0xFF;
    
    const ApartmentLocation& loc = APARTMENT_LOCATIONS[index];
    return VIBRATION_SENSOR_PINS[loc.sensorPinIndex];
}

// Get cutoff wire pin for a specific side and box
uint8_t getCutoffWirePin(BuildingSide side, BoxPosition box) {
    if(side == BuildingSide::RIGHT_SIDE) {
        return (box == BoxPosition::RIGHT_BOX) ? 
               CutoffWirePins::RIGHT_SIDE_RIGHT_BOX : 
               CutoffWirePins::RIGHT_SIDE_LEFT_BOX;
    } else {
        return (box == BoxPosition::RIGHT_BOX) ? 
               CutoffWirePins::LEFT_SIDE_RIGHT_BOX : 
               CutoffWirePins::LEFT_SIDE_LEFT_BOX;
    }
}

// Get distribution unit cutoff wire pin for a specific side
uint8_t getDistributionWirePin(BuildingSide side) {
    return (side == BuildingSide::RIGHT_SIDE) ? 
           CutoffWirePins::RIGHT_SIDE_DISTRIBUTION : 
           CutoffWirePins::LEFT_SIDE_DISTRIBUTION;
}

// Get VCC control pin for a specific side
uint8_t getVCCControlPin(BuildingSide side) {
    return (side == BuildingSide::RIGHT_SIDE) ? 
           RIGHT_SIDE_VCC_PIN : 
           LEFT_SIDE_VCC_PIN;
}

// Get siren pin for a specific side
uint8_t getSirenPin(BuildingSide side) {
    return (side == BuildingSide::RIGHT_SIDE) ? 
           RIGHT_SIDE_SIREN_PIN : 
           LEFT_SIDE_SIREN_PIN;
}

// Enable VCC for a specific side
void enableSideVCC(BuildingSide side) {
    uint8_t pin = getVCCControlPin(side);
    digitalWrite(pin, LOW);  // LOW enables the NPN transistor
    Serial.println("VCC Enabled for side: " + String(side) + " on pin: " + String(pin));
}

// Disable VCC for a specific side
void disableSideVCC(BuildingSide side) {
    uint8_t pin = getVCCControlPin(side);
    digitalWrite(pin, HIGH);  // HIGH disables the NPN transistor
    Serial.println("VCC Disabled for side: " + String(side) + " on pin: " + String(pin));
}

// Activate siren for a specific side
void activateSiren(BuildingSide side) {
    uint8_t pin = getSirenPin(side);
    digitalWrite(pin, SIREN_ACTIVATE);  // Activate the siren   
}

// Stop siren for a specific side
void stopSiren(BuildingSide side) {
    uint8_t pin = getSirenPin(side);
    digitalWrite(pin, !SIREN_ACTIVATE);  // Deactivate the siren
}

// Read vibration sensor for a specific apartment. Returns true if triggered.
bool readVibrationSensor(uint8_t apartmentNumber) {
    uint8_t pin = getVibrationSensorPin(apartmentNumber);
    if(pin == 0xFF) return false;
    
    return digitalRead(pin) == VIBRATION_TRIGGER_LEVEL;
}

// Read cutoff wire status. Returns true if the wire is cut.
bool readCutoffWire(BuildingSide side, BoxPosition box) {
    uint8_t pin = getCutoffWirePin(side, box);
    return digitalRead(pin) == WIRE_CUT_TRIGGER;  // HIGH means wire is cut
}

// Read distribution unit wire status. Returns true if the wire is cut.
bool readDistributionWire(BuildingSide side) {
    uint8_t pin = getDistributionWirePin(side);
    return digitalRead(pin) == WIRE_CUT_TRIGGER;  // HIGH means wire is cut
}
