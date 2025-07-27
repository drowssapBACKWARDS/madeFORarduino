#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

#include "config.h"

// Forward declarations
int getAutoClickPower(int level);

// Optimized getDigitCount
int getDigitCount(long number) {
  int count = 1;
  while (number >= 10 && count < MAX_DIGITS) {
    number /= 10;
    count++;
  }
  return count;
}

// Helper function: calculate level
int getLevel(int clickPower) {
    return clickPower / 2 + 1;
}

// Helper function: calculate next click power
int getNextClickPower(int clickPower) {
  return (clickPower == 1) ? 2 : (clickPower + 2);
}

long long calculateUpgradeCost() {
  int level = getLevel(cookiesPerClick);
  
  // New algorithm: exponential growth.
  // Base cost 100, each level is 15% more expensive.
  long long cost = 100;
  int growth_factor_scaled = 115; // 1.15 * 100

  for (int i = 1; i < level; i++) {
    // Multiply by 1.15 using integer arithmetic
    cost = (cost * growth_factor_scaled) / 100;
  }

  // A small addition so the price grows even at early levels
  if (level > 1) {
      cost += level * 10;
  }

  return cost;
}

long long calculateAutoClickUpgradeCost() {
  int safeLevel = autoClickLevel < 0 ? 0 : autoClickLevel;
  // Fixed price for the first purchase
  if (safeLevel == 0) return 1000;
  int power = getAutoClickPower(safeLevel + 1); // Calculate for the next level
  long long cost = (long long)power * power * 100L;
  if (safeLevel + 1 > 15) cost *= (power / 2);
  return cost;
}

int getAutoClickPower(int level) {
  if (level == 0) return 0; // If not purchased, power is 0
  if (level == 1) return 1;
  if (level <= 15) return 1 + (level - 1) * 2;
  return 1 + (14 * 2) + (level - 15) * 4;
}

#endif // GAME_LOGIC_H 
