#ifndef POWER_H
#define POWER_H

#include <cstdint>

bool initPower();
void updateBatteryStatus();
uint8_t getBatteryPercent();

#endif