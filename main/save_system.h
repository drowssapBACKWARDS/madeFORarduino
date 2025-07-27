#ifndef SAVE_SYSTEM_H
#define SAVE_SYSTEM_H

#include "config.h"

// Forward declarations
void manualSave();
void showMessage(const char* line1, const char* line2, GameState nextScreen, unsigned long timeout);

void tryAutoSave() {
  long saveStep = (long)cookiesPerClick * 200;
  if (saveStep < 1) saveStep = 1;
  if ((cookies / saveStep) != (lastSavedCookies / saveStep)) {
    manualSave();
    lastSavedCookies = cookies;
  }
  needRedraw = true;
}

void manualSave() {
  GameData data = {
    cookies,
    cookiesPerClick,
    totalCookies,
    totalClicks,
    totalUpgrades,
    autoClickLevel,
    prestigeClickLevel,
    prestigeAutoClickLevel
  };
  EEPROM.put(0, data);
  lastSavedCookies = cookies;
  needRedraw = true;
}

void manualReset() {
  cookies = 0;
  cookiesPerClick = prestigeClickLevel;
  totalCookies = 0;
  totalClicks = 0;
  totalUpgrades = 0;
  autoClickLevel = prestigeAutoClickLevel;
  manualSave();
  needRedraw = true;
}

void activatePrestige() {
    if (cookies >= 1750000) {
        prestigeClickLevel = 10;
        prestigeAutoClickLevel = 5;
    } else if (cookies >= 1000000) {
        prestigeClickLevel = 8;
        prestigeAutoClickLevel = 3;
    }
    manualReset();
    currentScreen = MAIN;
    needRedraw = true;
}

void activateGift() {
  static const long cookieRewards[] = {100, 500, 1000, 5000, 10000, 15000};
  if (giftType < 6) {
    cookies += cookieRewards[giftType];
    totalCookies += cookieRewards[giftType];
  } else if (giftType == 6) {
    cookiesPerClick += 50;
    showMessage("BOUGHT", "+50 to click", MAIN, 2000);
  } else if (giftType == 7) {
    cookiesPerClick += 2;
    totalUpgrades++;
    showMessage("BOUGHT", "+1 level", MAIN, 2000);
  } else if (giftType == 8) {
    bonus573Active = true;
    bonus573Start = millis();
  }
  tryAutoSave();
  giftActive = false;
  congratsActive = true;
  congratsStart = millis();
  needRedraw = true;
}

#endif // SAVE_SYSTEM_H 
