#ifndef UI_SCREENS_H
#define UI_SCREENS_H

#include "config.h"
#include "lcd_helpers.h"
#include "game_logic.h"

// Forward declarations
void displayMainScreen();
void displayShopScreen();
void displayStarScreen();
void displayAScreen();
void displayPrestigeConfirmScreen();
void displayMessageScreen();
void displayCongratsScreen();
void displayCursor();
void redrawElementAt(int x, int y);
void resetPrevScreenVars();

void showMessage(const char* line1, const char* line2, GameState nextScreen, unsigned long timeout) {
    strncpy(messageLine1, line1, 16);
    messageLine1[16] = '\0';
    strncpy(messageLine2, line2, 16);
    messageLine2[16] = '\0';
    currentScreen = MESSAGE_SCREEN;
    screenAfterMessage = nextScreen;
    messageTimeout = timeout;
    if (timeout > 0) {
        messageDisplayStart = millis();
    }
    needRedraw = true;
}

void displayManager() {
  if (congratsActive) {
    displayCongratsScreen();
  } else {
    switch (currentScreen) {
      case MAIN:
        displayMainScreen();
        break;
      case SHOP:
        displayShopScreen();
        break;
      case STATS:
        displayStarScreen();
        break;
      case AUTOCLICK_SHOP:
        displayAScreen();
        break;
      case PRESTIGE_CONFIRM:
        displayPrestigeConfirmScreen();
        break;
      case MESSAGE_SCREEN:
        displayMessageScreen();
        break;
    }
  }
  displayCursor();
}

void displayMainScreen() {
  if (cookies != prevState.cookies) {
    // Draw cookies and stats button together
    char buf[12];
    snprintf(buf, sizeof(buf), "%ld", cookies);
    int cookieDigits = strlen(buf);
    if (cookieDigits > MAX_DIGITS) cookieDigits = MAX_DIGITS;
    int cookiePos = 0;
    // Clear the whole area first
    lcdPrintAt(cookiePos, 0, "                ");
    // Print cookies
    lcd.setCursor(cookiePos, 0);
    lcd.print(buf);
    lcd.write((byte)0); // cookie icon
    lcd.print("S"); // Только S после печенек
    prevState.cookies = cookies;
  }
  if (giftActive != prevState.giftActive || giftPos != prevState.giftPos) {
    if (giftActive) {
      lcdPrintAt(giftPos, 0, "#");
    } else if (prevState.giftActive) {
      lcdPrintAt(prevState.giftPos, 0, " ");
    }
    prevState.giftActive = giftActive;
    prevState.giftPos = giftPos;
  }
  lcdPrintAt(0, 1, F("SHOP"));
  lcdPrintAt(4, 1, "a");
  lcdPrintAt(5, 1, "*"); // Звездочка для престижа
  drawFarmButtons();
}

void displayShopScreen() {
  long long cost = calculateUpgradeCost();
  int nextClick = getNextClickPower(cookiesPerClick);
  int level = getLevel(cookiesPerClick);
  if (cost != prevState.shopCost || nextClick != prevState.shopNextClick || level != prevState.shopLevel) {
    lcdPrintAt(0, 0, "^");
    lcdPrintAt(1, 0, "           ");
    printRightAligned(cost, 1, 0, 4);
    drawRightAlignedLabel(nextClick, 0);
    lcdPrintAt(0, 1, "<");
    lcdPrintAt(2, 1, F("Your Level"));
    drawRightAlignedLabel(level, 1);
    prevState.shopCost = cost;
    prevState.shopNextClick = nextClick;
    prevState.shopLevel = level;
  }
}

void displayPrestigeConfirmScreen() {
    lcdPrintAt(0, 0, F("ARE YOU SURE?"));
    lcdPrintAt(0, 1, F("NO"));
    lcdPrintAt(6, 1, F("YES"));
}

void displayMessageScreen() {
    lcdPrintAt(0, 0, messageLine1);
    lcdPrintAt(0, 1, messageLine2);
}

void displayStarScreen() {
  if (totalCookies != prevState.totalCookies ||
      getLevel(cookiesPerClick) != prevState.statsLevel ||
      totalUpgrades != prevState.totalUpgrades ||
      totalClicks != prevState.totalClicks) {
    lcdPrintAt(0, 0, F("T:"));
    char buf[13];
    snprintf(buf, sizeof(buf), "%11ld", totalCookies);
    for (int i = 0; i < 11; i++) {
      lcdPrintAt(2 + i, 0, buf[i]);
    }
    lcdPrintAt(15, 0, "S");
    lcdPrintAt(0, 1, "<");
    lcdPrintAt(1, 1, F("L:"));
    lcdPrintAt(3, 1, getLevel(cookiesPerClick));
    lcdPrintAt(5, 1, F(" U:"));
    lcdPrintAt(7, 1, totalUpgrades);
    lcdPrintAt(9, 1, F(" C:"));
    lcdPrintAt(11, 1, totalClicks);
    lcdPrintAt(15, 1, "R");
    prevState.totalCookies = totalCookies;
    prevState.statsLevel = getLevel(cookiesPerClick);
    prevState.totalUpgrades = totalUpgrades;
    prevState.totalClicks = totalClicks;
  }
}

void displayCongratsScreen() {
  lcd.clear();
  lcdPrintAt(0, 0, F("Congratulations"));
  lcdPrintAt(0, 1, GIFT_TEXTS[giftType]);
  needRedraw = true;
}

void displayAScreen() {
  long long cost = calculateAutoClickUpgradeCost();
  int income = getAutoClickPower(autoClickLevel < 0 ? 1 : autoClickLevel + 1);
  int level = autoClickLevel < 0 ? 0 : autoClickLevel;
  if (cost != prevState.autoCost || income != prevState.autoIncome || level != prevState.autoLevel) {
    lcdPrintAt(0, 0, "^");
    printRightAligned(cost, 1, 0, 4);
    char incbuf[4];
    snprintf(incbuf, sizeof(incbuf), "%3d", income);
    lcdPrintAt(13, 0, incbuf);
    lcdPrintAt(0, 1, "<");
    lcdPrintAt(2, 1, F("Your Level"));
    char lvlbuf[4];
    snprintf(lvlbuf, sizeof(lvlbuf), "%3d", level);
    lcdPrintAt(13, 1, lvlbuf);
    prevState.autoCost = cost;
    prevState.autoIncome = income;
    prevState.autoLevel = level;
  }
}

void displayCursor() {
  // Redraw previous cursor position
  if (prevCursorX != cursorX || prevCursorY != cursorY) {
    redrawElementAt(prevCursorX, prevCursorY);
  }
  // Don't show cursor if it's not visible (blinking)
  if (!cursorVisible) {
    prevCursorX = cursorX;
    prevCursorY = cursorY;
    return;
  }
  lcdWriteAt(cursorX, cursorY, (uint8_t)2);
  prevCursorX = cursorX;
  prevCursorY = cursorY;
}

// Reset screen state
void resetPrevScreenVars() {
  memset(&prevState, -1, sizeof(prevState));
}

// Redraw the element at the given position depending on the current screen
void redrawElementAt(int x, int y) {
  switch (currentScreen) {
    case MAIN:
      if (y == 0) {
        // Top row: cookies, gift, or farm buttons
        if (giftActive && x == giftPos) {
          lcdPrintAt(x, y, "#");
        } else if (x >= 0 && x < getDigitCount(cookies)) {
          // Redraw cookie digit
          char buf[8];
          snprintf(buf, sizeof(buf), "%ld", cookies);
          int len = strlen(buf);
          if (x < len) {
            char c[2] = {buf[x], '\0'};
            lcdPrintAt(x, y, c);
          } else {
            lcdPrintAt(x, y, " ");
          }
        } else if (x == getDigitCount(cookies)) {
          lcdWriteAt(x, y, (uint8_t)0); // cookie icon
        } else if (x == getDigitCount(cookies) + 1) {
          lcdPrintAt(x, y, "S"); // Только S после печенек
        } else if (x >= BUTTON_FARM_X_START && x <= BUTTON_FARM_X_END) {
          lcdPrintAt(x, y, "J"); // Farm buttons on top row
        } else {
          lcdPrintAt(x, y, " ");
        }
      } else if (y == 1) {
        // Bottom row: buttons
        if (x >= BUTTON_SHOP_X_START && x <= BUTTON_SHOP_X_END) {
          const char* shopText = "SHOP";
          lcdPrintAt(x, y, shopText + (x - BUTTON_SHOP_X_START));
        } else if (x == BUTTON_AUTOCLICK_X) {
          lcdPrintAt(x, y, "a");
        } else if (x == 5) {
          lcdPrintAt(x, y, "*"); // Звездочка для престижа
        } else if (x >= BUTTON_FARM_X_START && x <= BUTTON_FARM_X_END) {
          lcdPrintAt(x, y, "J"); // Farm buttons on bottom row
        } else {
          lcdPrintAt(x, y, " ");
        }
      }
      break;
    case SHOP:
      if (y == 0 && x == 0) drawButton(0, 0, "^", false);
      else if (y == 1 && x == 0) drawButton(0, 1, "<", false);
      else if (y == 0 && x > 0 && x < 16) lcdPrintAt(x, 0, " ");
      else if (y == 1 && x > 0 && x < 16) lcdPrintAt(x, 1, " ");
      break;
    case AUTOCLICK_SHOP:
      if (y == 0 && x == 0) drawButton(0, 0, "^", false);
      else if (y == 1 && x == 0) drawButton(0, 1, "<", false);
      else if (y == 0 && x > 0 && x < 16) lcdPrintAt(x, 0, " ");
      else if (y == 1 && x > 0 && x < 16) lcdPrintAt(x, 1, " ");
      break;
    case STATS:
      if (y == 0) {
        if (x == 0) lcdPrintAt(0, 0, "T");
        else if (x == 1) lcdPrintAt(1, 0, ":");
        else if (x >= 2 && x < 13) {
          // Redraw total cookies digits
          char buf[13];
          snprintf(buf, sizeof(buf), "%11ld", totalCookies);
          lcdPrintAt(x, 0, buf[x-2]);
        }
        else if (x == 15) lcdPrintAt(15, 0, "S");
        else lcdPrintAt(x, 0, " ");
      } else if (y == 1) {
        if (x == 0) lcdPrintAt(0, 1, "<");
        else if (x == 1) lcdPrintAt(1, 1, "L");
        else if (x == 2) lcdPrintAt(2, 1, ":");
        else if (x == 3) lcdPrintAt(3, 1, getLevel(cookiesPerClick));
        else if (x == 5) lcdPrintAt(5, 1, "U");
        else if (x == 6) lcdPrintAt(6, 1, ":");
        else if (x == 7) lcdPrintAt(7, 1, totalUpgrades);
        else if (x == 9) lcdPrintAt(9, 1, "C");
        else if (x == 10) lcdPrintAt(10, 1, ":");
        else if (x == 11) lcdPrintAt(11, 1, totalClicks);
        else if (x == 15) lcdPrintAt(15, 1, "R");
        else lcdPrintAt(x, 1, " ");
      }
      break;
    case PRESTIGE_CONFIRM:
      if (y == 0) {
        const char* sureText = "ARE YOU SURE?";
        if (x < strlen(sureText)) {
          char c[2] = {sureText[x], '\0'};
          lcdPrintAt(x, 0, c);
        } else {
          lcdPrintAt(x, 0, " ");
        }
      } else if (y == 1) {
        if (x >= BUTTON_PRESTIGE_NO_X_START && x <= BUTTON_PRESTIGE_NO_X_END) {
          const char* noText = "NO";
          lcdPrintAt(x, 1, noText + (x - BUTTON_PRESTIGE_NO_X_START));
        } else if (x >= BUTTON_PRESTIGE_YES_X_START && x <= BUTTON_PRESTIGE_YES_X_END) {
          const char* yesText = "YES";
          lcdPrintAt(x, 1, yesText + (x - BUTTON_PRESTIGE_YES_X_START));
        } else {
          lcdPrintAt(x, 1, " ");
        }
      }
      break;
    case MESSAGE_SCREEN:
      if (y == 0) {
        if (x < strlen(messageLine1) && messageLine1[x] != '\0') {
          char c[2] = {messageLine1[x], '\0'};
          lcdPrintAt(x, 0, c);
        } else {
          lcdPrintAt(x, 0, " ");
        }
      } else if (y == 1) {
        if (x < strlen(messageLine2) && messageLine2[x] != '\0') {
          char c[2] = {messageLine2[x], '\0'};
          lcdPrintAt(x, 1, c);
        } else {
          lcdPrintAt(x, 1, " ");
        }
      }
      break;
    default:
      lcdPrintAt(x, y, " ");
      break;
  }
}

#endif // UI_SCREENS_H 
