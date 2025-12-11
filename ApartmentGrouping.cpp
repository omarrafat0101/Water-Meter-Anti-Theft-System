#include "ApartmentGrouping.h"

// Apartment Grouping Helpers
void getApartmentsInSameBox(uint8_t apartmentNumber, uint8_t *apartments, uint8_t &count)
{
  count = 0;
  uint8_t index = getApartmentIndex(apartmentNumber);
  if (index == 0xFF)
    return;

  BuildingSide side = APARTMENT_LOCATIONS[index].side;
  BoxPosition box = APARTMENT_LOCATIONS[index].box;

  for (uint8_t i = 0; i < TOTAL_APARTMENTS; i++)
  {
    if (APARTMENT_LOCATIONS[i].side == side &&
        APARTMENT_LOCATIONS[i].box == box)
    {
      for (uint8_t apt = 1; apt <= TOTAL_APARTMENTS; apt++)
      {
        if (getApartmentIndex(apt) == i)
        {
          apartments[count++] = apt;
          break;
        }
      }
    }
  }
}

void getApartmentsInAdjacentBox(uint8_t apartmentNumber, uint8_t *apartments, uint8_t &count)
{
  count = 0;
  uint8_t index = getApartmentIndex(apartmentNumber);
  if (index == 0xFF)
    return;

  BuildingSide side = APARTMENT_LOCATIONS[index].side;
  // Get opposite box on same side
  BoxPosition adjacentBox = (APARTMENT_LOCATIONS[index].box == BoxPosition::RIGHT_BOX) ? BoxPosition::LEFT_BOX : BoxPosition::RIGHT_BOX;

  for (uint8_t i = 0; i < TOTAL_APARTMENTS; i++)
  {
    if (APARTMENT_LOCATIONS[i].side == side &&
        APARTMENT_LOCATIONS[i].box == adjacentBox)
    {
      for (uint8_t apt = 1; apt <= TOTAL_APARTMENTS; apt++)
      {
        if (getApartmentIndex(apt) == i)
        {
          apartments[count++] = apt;
          break;
        }
      }
    }
  }
}

void getApartmentsInOtherSide(uint8_t apartmentNumber, uint8_t *apartments, uint8_t &count)
{
  count = 0;
  uint8_t index = getApartmentIndex(apartmentNumber);
  if (index == 0xFF)
    return;

  // Get opposite side
  BuildingSide otherSide = (APARTMENT_LOCATIONS[index].side == BuildingSide::RIGHT_SIDE) ? BuildingSide::LEFT_SIDE : BuildingSide::RIGHT_SIDE;

  for (uint8_t i = 0; i < TOTAL_APARTMENTS; i++)
  {
    if (APARTMENT_LOCATIONS[i].side == otherSide)
    {
      for (uint8_t apt = 1; apt <= TOTAL_APARTMENTS; apt++)
      {
        if (getApartmentIndex(apt) == i)
        {
          apartments[count++] = apt;
          break;
        }
      }
    }
  }
}

BuildingSide getApartmentSide(uint8_t apartmentNumber)
{
  uint8_t index = getApartmentIndex(apartmentNumber);
  if (index == 0xFF)
    return BuildingSide::RIGHT_SIDE; // Default

  return APARTMENT_LOCATIONS[index].side;
}

BoxPosition getApartmentBox(uint8_t apartmentNumber)
{
  uint8_t index = getApartmentIndex(apartmentNumber);
  if (index == 0xFF)
    return BoxPosition::RIGHT_BOX; // Default

  return APARTMENT_LOCATIONS[index].box;
}

uint8_t getApartmentIndex(uint8_t apartmentNumber)
{
  if (apartmentNumber < 1 || apartmentNumber > TOTAL_APARTMENTS)
    return 0xFF;

  switch (apartmentNumber)
  {
  case 1:
    return 0;
  case 5:
    return 1;
  case 9:
    return 2;
  case 13:
    return 3;
  case 17:
    return 4;
  case 21:
    return 5;
  case 2:
    return 6;
  case 6:
    return 7;
  case 10:
    return 8;
  case 14:
    return 9;
  case 18:
    return 10;
  case 22:
    return 11;
  case 3:
    return 12;
  case 7:
    return 13;
  case 11:
    return 14;
  case 15:
    return 15;
  case 19:
    return 16;
  case 23:
    return 17;
  case 4:
    return 18;
  case 8:
    return 19;
  case 12:
    return 20;
  case 16:
    return 21;
  case 20:
    return 22;
  case 24:
    return 23;
  default:
    return 0xFF;
  }
}
