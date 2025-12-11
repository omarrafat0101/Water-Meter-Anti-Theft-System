// ApartmentGrouping.h
#ifndef APARTMENT_GROUPING_H
#define APARTMENT_GROUPING_H

#include <Arduino.h>

#define TOTAL_APARTMENTS 24

// Building Side Enumeration
enum BuildingSide {
    RIGHT_SIDE,
    LEFT_SIDE
};

// Box Position Enumeration
enum BoxPosition {
    RIGHT_BOX,
    LEFT_BOX
};

// Structure to hold apartment physical location
struct ApartmentLocation {
    BuildingSide side;
    BoxPosition box;
    String position;  // 1-6 (from top to bottom)
    uint8_t sensorPinIndex;
};

// Mapping of apartment numbers (1-24) to their physical locations
const ApartmentLocation APARTMENT_LOCATIONS[TOTAL_APARTMENTS] = {
    // Right Side, Right Box (21,17,13,9,5,1)
    {RIGHT_SIDE, LEFT_BOX, "الدور الأرضي", 0},  // Apt 1 (bottom)
    {RIGHT_SIDE, LEFT_BOX, "الدور الأول", 1},  // Apt 5
    {RIGHT_SIDE, LEFT_BOX, "الدور الثاني", 2},  // Apt 9
    {RIGHT_SIDE, LEFT_BOX, "الدور الثالث", 3},  // Apt 13
    {RIGHT_SIDE, LEFT_BOX, "الدور الرابع", 4},  // Apt 17
    {RIGHT_SIDE, LEFT_BOX, "الدور الخامس", 5},  // Apt 21 (top)

    // Right Side, Left Box (22,18,14,10,6,2)
    {RIGHT_SIDE, RIGHT_BOX, "الدور الأرضي", 6},   // Apt 2 (bottom)
    {RIGHT_SIDE, RIGHT_BOX, "الدور الأول", 7},   // Apt 6
    {RIGHT_SIDE, RIGHT_BOX, "الدور الثاني", 8},   // Apt 10
    {RIGHT_SIDE, RIGHT_BOX, "الدور الثالث", 9},   // Apt 14
    {RIGHT_SIDE, RIGHT_BOX, "الدور الرابع", 10},  // Apt 18
    {RIGHT_SIDE, RIGHT_BOX, "الدور الخامس", 11},  // Apt 22 (top)

    // Left Side, Right Box (23,19,15,11,7,3)
    {LEFT_SIDE, LEFT_BOX, "الدور الأرضي", 0},   // Apt 3 (bottom)
    {LEFT_SIDE, LEFT_BOX, "الدور الأول", 1},   // Apt 7
    {LEFT_SIDE, LEFT_BOX, "الدور الثاني", 2},   // Apt 11
    {LEFT_SIDE, LEFT_BOX, "الدور الثالث", 3},   // Apt 15
    {LEFT_SIDE, LEFT_BOX, "الدور الرابع", 4},   // Apt 19
    {LEFT_SIDE, LEFT_BOX, "الدور الخامس", 5},   // Apt 23 (top)

    // Left Side, Left Box (24,20,16,12,8,4)
    {LEFT_SIDE, RIGHT_BOX, "الدور الأرضي", 6},    // Apt 4 (bottom)
    {LEFT_SIDE, RIGHT_BOX, "الدور الأول", 7},    // Apt 8
    {LEFT_SIDE, RIGHT_BOX, "الدور الثاني", 8},    // Apt 12
    {LEFT_SIDE, RIGHT_BOX, "الدور الثالث", 9},    // Apt 16
    {LEFT_SIDE, RIGHT_BOX, "الدور الرابع", 10},   // Apt 20
    {LEFT_SIDE, RIGHT_BOX, "الدور الخامس", 11}    // Apt 24 (top)
};

// Function Prototypes
void getApartmentsInSameBox(uint8_t apartmentNumber, uint8_t* apartments, uint8_t& count);
void getApartmentsInAdjacentBox(uint8_t apartmentNumber, uint8_t* apartments, uint8_t& count);
void getApartmentsInOtherSide(uint8_t apartmentNumber, uint8_t* apartments, uint8_t& count);
BuildingSide getApartmentSide(uint8_t apartmentNumber);
BoxPosition getApartmentBox(uint8_t apartmentNumber);
uint8_t getApartmentIndex(uint8_t apartmentNumber);

#endif // APARTMENT_GROUPING_H