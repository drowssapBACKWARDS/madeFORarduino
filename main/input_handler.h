#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include "config.h"
#include "game_logic.h"
#include "save_system.h"
#include "ui_screens.h"

void handleJoystick() {
  unsigned long now = millis();
  if (now - lastJoystickMoveTime < JOYSTICK_DELAY) {
    return;
  }
  
  // Read joystick state once
  bool rightPressed = digitalRead(JOY_RIGHT) == HIGH;
  bool leftPressed = digitalRead(JOY_LEFT) == HIGH;
  bool upPressed = digitalRead(JOY_UP) == HIGH;
  bool downPressed = digitalRead(JOY_DOWN) == HIGH;
  
  // Only move if exactly one direction is pressed
  if (rightPressed && !leftPressed && !upPressed && !downPressed && cursorX < 15) {
    cursorX++;
    lastJoystickMoveTime = now;
  } else if (leftPressed && !rightPressed && !upPressed && !downPressed && cursorX > 0) {
    cursorX--;
    lastJoystickMoveTime = now;
  } else if (upPressed && !rightPressed && !leftPressed && !downPressed && cursorY > 0) {
    cursorY--;
    lastJoystickMoveTime = now;
  } else if (downPressed && !rightPressed && !leftPressed && !upPressed && cursorY < 1) {
    cursorY++;
    lastJoystickMoveTime = now;
  }
}

void handleButtonPress() {
  static bool lastState = false;
  static unsigned long lastAutoCraftTime = 0;
  bool pressed = digitalRead(JOY_CENTER);
  unsigned long now = millis();
  
  // This block handles HELD presses, specifically for autocrafting на главном экране.
  // Теперь работает и для верхней, и для нижней строки с J.
  if (currentScreen == MAIN && !congratsActive && pressed && cursorX >= BUTTON_FARM_X_START && cursorX <= BUTTON_FARM_X_END && (cursorY == 0 || cursorY == 1)) {
    if (now - lastAutoCraftTime > DEBOUNCE_DELAY) {
      int clickValue = bonus573Active ? (cookiesPerClick + 573) : cookiesPerClick;
      cookies += clickValue;
      totalCookies += clickValue;
      totalClicks++;
      lastAutoCraftTime = now;
      needRedraw = true;
    }
    return;
  }
  
  // This block handles SINGLE presses, with debouncing.
  if (pressed && !lastState && now - lastButtonPressTime > DEBOUNCE_DELAY) {
    lastButtonPressTime = now;
    
    // Handle message screen - any press closes it (before congrats check)
    if (currentScreen == MESSAGE_SCREEN) {
        currentScreen = screenAfterMessage;
        messageTimeout = 0;
        needRedraw = true;
        lastState = pressed;
        return;
    }
    
    if (congratsActive) {
        lastState = pressed;
        return;
    }

    switch (currentScreen) {
      case MAIN:
        // Gift press handling
        if (giftActive && cursorY == 0 && cursorX == giftPos) {
          activateGift();
          needRedraw = true;
          lastState = pressed;
          return;
        }
        // SHOP button
        else if (cursorY == 1 && cursorX >= BUTTON_SHOP_X_START && cursorX <= BUTTON_SHOP_X_END) {
          currentScreen = SHOP;
          needRedraw = true;
        }
        // 'a' button (autoclick shop)
        else if (cursorY == 1 && cursorX == BUTTON_AUTOCLICK_X) {
          currentScreen = AUTOCLICK_SHOP;
          cursorX = 0;
          cursorY = 1;
          needRedraw = true;
        }
        // '*' button (prestige)
        else if (cursorY == 1 && cursorX == BUTTON_PRESTIGE_X) {
            if (cookies >= 1000000) {
                currentScreen = PRESTIGE_CONFIRM;
                cursorX = 0;
                cursorY = 1;
            } else {
                showMessage("YOU HAVEN'T", "ENOUGH COOKIES", MAIN, 5000);
            }
            needRedraw = true;
        }
        // Кнопка S - статистика (после количества печенек)
        else if (cursorY == 0 && cursorX == getDigitCount(cookies)) {
          currentScreen = STATS;
          cursorX = 0;
          cursorY = 1;
          needRedraw = true;
        }
        // Any other click is a regular cookie click
        else {
          int clickValue = bonus573Active ? (cookiesPerClick + 573) : cookiesPerClick;
          cookies += clickValue;
          totalCookies += clickValue;
          totalClicks++;
          needRedraw = true;
        }
        break;
      
      case PRESTIGE_CONFIRM:
        // NO button
        if (cursorY == 1 && cursorX >= BUTTON_PRESTIGE_NO_X_START && cursorX <= BUTTON_PRESTIGE_NO_X_END) {
            currentScreen = MAIN;
            needRedraw = true;
        }
        // YES button
        else if (cursorY == 1 && cursorX >= BUTTON_PRESTIGE_YES_X_START && cursorX <= BUTTON_PRESTIGE_YES_X_END) {
            activatePrestige();
            needRedraw = true;
        }
        break;



      case AUTOCLICK_SHOP:
        // Exit button is now on the second line
        if (cursorY == 1 && cursorX == BUTTON_BACK_X) {
          currentScreen = MAIN;
          needRedraw = true;
        }
        // Upgrade button is at (0,0)
        else if (cursorY == 0 && cursorX == BUTTON_UPGRADE_X) {
          long long cost = calculateAutoClickUpgradeCost();
          if (cookies >= cost) {
            cookies -= cost;
            autoClickLevel++;
            showMessage("BOUGHT", "", AUTOCLICK_SHOP, 2000);
            needRedraw = true;
          }
        }
        break;

      case SHOP:
        // Back button
        if (cursorY == 1 && cursorX == BUTTON_BACK_X) {
          currentScreen = MAIN;
          needRedraw = true;
        }
        // Upgrade button
        else if (cursorY == 0 && cursorX == BUTTON_UPGRADE_X) {
          long long cost = calculateUpgradeCost();
          if (cookies >= cost) {
            cookies -= cost;
            cookiesPerClick = getNextClickPower(cookiesPerClick);
            totalUpgrades++;
            showMessage("BOUGHT", "", SHOP, 2000);
            needRedraw = true;
          }
        }
        break;

      case STATS:
        // Exit button <
        if (cursorY == 1 && cursorX == BUTTON_BACK_X) {
          currentScreen = MAIN;
          needRedraw = true;
        }
        // Save button S (moved to top right)
        // This is now handled above
        // Reset button R
        else if (cursorY == 1 && cursorX == BUTTON_RESET_X) {
          manualReset();
          needRedraw = true;
        }
        // Save button S (top right)
        else if (cursorY == 0 && cursorX == 15) {
          manualSave();
          needRedraw = true;
        }
        break;
    }
    
    lastState = pressed;
  }
}

#endif // INPUT_HANDLER_H 
