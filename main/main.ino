#include "config.h"
#include "lcd_helpers.h"
#include "game_logic.h"
#include "ui_screens.h"
#include "input_handler.h"
#include "save_system.h"

void setup() {
  // Init LCD
  lcd.begin(16, 2);
  
  // Create Custom Characters
  lcd.createChar(0, starIcon);
  lcd.createChar(1, arrowUpIcon);
  lcd.createChar(2, filledBlock); // For the cursor
  
  // Init Joystick Pins
  pinMode(JOY_CENTER, INPUT);
  pinMode(JOY_UP, INPUT);
  pinMode(JOY_DOWN, INPUT);
  pinMode(JOY_LEFT, INPUT);
  pinMode(JOY_RIGHT, INPUT);
  
  // Clear screen and display initial screen
  lcd.clear();
  displayManager();

  // Load progress from EEPROM
  GameData data;
  EEPROM.get(0, data);
  cookies = data.cookies;
  cookiesPerClick = data.cookiesPerClick;
  totalCookies = data.totalCookies;
  totalClicks = data.totalClicks;
  totalUpgrades = data.totalUpgrades;
  autoClickLevel = data.autoClickLevel;
  prestigeClickLevel = data.prestigeClickLevel;
  prestigeAutoClickLevel = data.prestigeAutoClickLevel;
  
  // Sanity checks for loaded data
  if (autoClickLevel < 0) autoClickLevel = 0;
  if (prestigeClickLevel < 1) prestigeClickLevel = 1;
  if (prestigeAutoClickLevel < 0) prestigeAutoClickLevel = 0;
  
  lastSavedCookies = cookies;

  randomSeed(analogRead(0));
  
  // Initialize milestone for the "new digit" bonus
  nextMilestone = 100;
  while (nextMilestone <= cookies && nextMilestone > 0) {
    nextMilestone *= 10;
  }
}

void loop() {
  // Check for the milestone bonus
  if (cookies >= nextMilestone && nextMilestone > 0) {
      cookies *= 2;
      showMessage("MILESTONE!", "x2 Cookies!", MAIN, 2000);
      nextMilestone *= 10;
      if (nextMilestone == 0) nextMilestone = -1; // overflow, disable
  }

  // Timeout for the message screen
  if (currentScreen == MESSAGE_SCREEN && messageTimeout > 0 && millis() - messageDisplayStart > messageTimeout) {
      currentScreen = screenAfterMessage;
      messageTimeout = 0; // Reset timeout
  }

  // Gift spawning
  if (!giftActive && !congratsActive && currentScreen == MAIN && millis() - lastGiftTime > GIFT_INTERVAL) {
    giftActive = true;
    giftType = random(GIFT_COUNT);
    giftPos = GIFT_POSITIONS[random(4)];
    lastGiftTime = millis();
  }
  // End of temporary bonus
  if (bonus573Active && millis() - bonus573Start > BONUS573_TIME) {
    bonus573Active = false;
  }
  // End of congratulations screen
  if (congratsActive && millis() - congratsStart > CONGRATS_TIME) {
    congratsActive = false;
  }
  // Autoclicker runs only if purchased (level > 0)
  if (currentScreen == MAIN && autoClickLevel > 0 && millis() - lastAutoClickTime > AUTOCLICK_INTERVAL) {
    cookies += getAutoClickPower(autoClickLevel);
    lastAutoClickTime = millis();
  }
  handleJoystick();
  handleButtonPress();
  
  // Clear LCD and redraw everything if screen changed
  if (currentScreen != lastScreen) {
    lcd.clear();
    lastScreen = currentScreen;
    needRedraw = true;
    resetPrevScreenVars();
    // Reset cursor position for new screen
    prevCursorX = -1;
    prevCursorY = -1;
  }
  
  // Cursor blinking - always update
  if (millis() - lastBlinkTime > BLINK_INTERVAL) {
    cursorVisible = !cursorVisible;
    lastBlinkTime = millis();
    needRedraw = true;
  }
  
  // Always redraw to ensure constant updates
  displayManager();
  
  // Always update cursor for blinking effect - ensure it's visible on symbols
  displayCursor();
  needRedraw = false;
} 
