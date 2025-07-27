#include <LiquidCrystal.h>
#include <EEPROM.h>
#include <stdlib.h>
#include <string.h>

// LCD Screen Connection
constexpr uint8_t PIN_RS = 6;
constexpr uint8_t PIN_EN = 7;
constexpr uint8_t PIN_DB4 = 8;
constexpr uint8_t PIN_DB5 = 9;
constexpr uint8_t PIN_DB6 = 10;
constexpr uint8_t PIN_DB7 = 11;
LiquidCrystal lcd(PIN_RS, PIN_EN, PIN_DB4, PIN_DB5, PIN_DB6, PIN_DB7);

// Custom Characters
// Cookie Icon
byte cookieIcon[8] = {
  B00000,
  B01110,
  B11111,
  B11111,
  B11111,
  B01110,
  B00000,
  B00000
};
// Up Arrow Icon
byte arrowUpIcon[8] = {
  B00100,
  B01110,
  B10101,
  B00100,
  B00100,
  B00100,
  B00100,
  B00000
};
// Filled Block (for cursor)
byte filledBlock[8] = {
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111
};
// Star Icon
byte starIcon[8] = {
  B00100,
  B10101,
  B01110,
  B11111,
  B01110,
  B10101,
  B00100,
  B00000
};

// Game Variables
long cookies = 0;
int cookiesPerClick = 1;

// Cursor Variables
int cursorX = 15;
int cursorY = 1;
int prevCursorX = 15;
int prevCursorY = 1;

// Screen State Management
enum GameState {
  MAIN,
  SHOP,
  STATS,
  AUTOCLICK_SHOP,
  PRESTIGE_CONFIRM,
  MESSAGE_SCREEN
};
GameState currentScreen = MAIN;
GameState lastScreen = MAIN;

// Non-blocking Timers
unsigned long lastButtonPressTime = 0;
const unsigned long DEBOUNCE_DELAY = 600;
unsigned long lastJoystickMoveTime = 0;
const unsigned long JOYSTICK_DELAY = 200;

// Cursor Blinking
unsigned long lastBlinkTime = 0;
bool cursorVisible = true;
const unsigned long BLINK_INTERVAL = 450;

// Message Screen Variables
char messageLine1[17] = "";
char messageLine2[17] = "";
GameState screenAfterMessage = MAIN;
unsigned long messageDisplayStart = 0;
unsigned long messageTimeout = 0; // 0 = no timeout

// Prestige Bonuses
int prestigeClickLevel = 1;
int prestigeAutoClickLevel = 0;

// Data Structure for EEPROM
struct GameData {
  long cookies;
  int cookiesPerClick;
  long totalCookies;
  long totalClicks;
  long totalUpgrades;
  int autoClickLevel;
  int prestigeClickLevel;
  int prestigeAutoClickLevel;
};

// UI Layout Constants
const int BUTTON_SHOP_X_START = 0;
const int BUTTON_SHOP_X_END = 3;
const int BUTTON_AUTOCLICK_X = 4;
const int BUTTON_PRESTIGE_X = 5;
const int BUTTON_FARM_X_START = 12;
const int BUTTON_FARM_X_END = 15;

const int BUTTON_PRESTIGE_NO_X_START = 0;
const int BUTTON_PRESTIGE_NO_X_END = 2;
const int BUTTON_PRESTIGE_YES_X_START = 6;
const int BUTTON_PRESTIGE_YES_X_END = 9;

const int BUTTON_BACK_X = 0;
const int BUTTON_UPGRADE_X = 0;
const int BUTTON_SAVE_X = 15;
const int BUTTON_RESET_X = 15;

// Milestone Bonus Variable
long nextMilestone = 100;

// Joystick Pins
const int JOY_CENTER = 2;
const int JOY_UP = 3;
const int JOY_DOWN = 4;
const int JOY_LEFT = 5;
const int JOY_RIGHT = 12;

// Statistics
long totalCookies = 0;
long totalClicks = 0;
long totalUpgrades = 0;

// Auto-save
long lastSavedCookies = 0;

// --- Gifts ---
const int GIFT_POSITIONS[4] = {8, 9, 10, 11};
const int GIFT_COUNT = 9;
const char* GIFT_TEXTS[GIFT_COUNT] = {
  "+100 cookies",
  "+500 cookies",
  "+1000 cookies",
  "+5000 cookies",
  "+10000 cookies",
  "+15000 cookies",
  "+50 click power",
  "+1 level",
  "60s: +573/click"
};

// Gift Variables
bool giftActive = false;
int giftType = 0;
int giftPos = 8;
unsigned long lastGiftTime = 0;
const unsigned long GIFT_INTERVAL = 120000; // 2 minutes

// Congratulations Screen
bool congratsActive = false;
unsigned long congratsStart = 0;
const unsigned long CONGRATS_TIME = 5000;

// Temporary Bonus
bool bonus573Active = false;
unsigned long bonus573Start = 0;
const unsigned long BONUS573_TIME = 60000;

// --- Autoclicker Variables ---
int autoClickLevel = 0; // Level 0 means not purchased
unsigned long lastAutoClickTime = 0;
const unsigned long AUTOCLICK_INTERVAL = 1000; // 1 second

bool needRedraw = true;

// === Optimization: constants and screen state structure ===
constexpr int LCD_WIDTH = 16;
constexpr int LCD_HEIGHT = 2;
constexpr int MAX_DIGITS = 7;
constexpr float UPGRADE_GROWTH = 1.15f;

struct ScreenState {
  long cookies;
  int cookiesPerClick;
  bool giftActive;
  int giftPos;
  long shopCost;
  int shopNextClick;
  int shopLevel;
  long autoCost;
  int autoIncome;
  int autoLevel;
  long totalCookies;
  int statsLevel;
  long totalUpgrades;
  long totalClicks;
};
ScreenState prevState = {-1, -1, false, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

// --- LCD HELPER FUNCTIONS ---
inline void lcdPrintAt(int x, int y, const char* str) {
  lcd.setCursor(x, y);
  lcd.print(str);
}
inline void lcdPrintAt(int x, int y, const __FlashStringHelper* str) {
  lcd.setCursor(x, y);
  lcd.print(str);
}
inline void lcdPrintAt(int x, int y, long value) {
  lcd.setCursor(x, y);
  lcd.print(value);
}
inline void lcdWriteAt(int x, int y, uint8_t ch) {
  lcd.setCursor(x, y);
  lcd.write(ch);
}

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
  
  // Only clear LCD and redraw everything if screen changed
  if (currentScreen != lastScreen) {
    lcd.clear();
    lastScreen = currentScreen;
    needRedraw = true;
    resetPrevScreenVars();
  }
  if (needRedraw) {
    displayManager();
    needRedraw = false;
  }

  // Cursor blinking
  if (millis() - lastBlinkTime > BLINK_INTERVAL) {
    cursorVisible = !cursorVisible;
    lastBlinkTime = millis();
    needRedraw = true;
  }
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
        else if (cursorY == 1 && cursorX == 5) {
            if (cookies >= 1000000) {
                currentScreen = PRESTIGE_CONFIRM;
                cursorX = 0;
                cursorY = 1;
            } else {
                showMessage("YOU HAVEN'T", "ENOUGH COOKIES", MAIN, 5000);
            }
            needRedraw = true;
        }
        // Кнопка S - статистика после количества печенек
        else if (cursorY == 0 && cursorX == getDigitCount(cookies) + 1) {
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

      case MESSAGE_SCREEN:
        // Any press will return to the previous screen
        currentScreen = screenAfterMessage;
        messageTimeout = 0;
        needRedraw = true;
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
  lcdWriteAt(cursorX, cursorY, (byte)2);
  prevCursorX = cursorX;
  prevCursorY = cursorY;
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

// Universal function to print a number right-aligned
void printRightAligned(int value, int row, int col, int width) {
  char buf[8];
  snprintf(buf, sizeof(buf), "%*d", width, value);
  for (int i = 0; i < width; i++) {
    lcdPrintAt(col + i, row, buf[i]);
  }
}

// Universal function for a button
inline void drawButton(int x, int y, const char* label, bool highlight) {
  if (highlight) {
    lcdWriteAt(x, y, 2);
  } else {
    lcdPrintAt(x, y, label);
  }
}

// Helper: Draw a right-aligned label in 4 cells
void drawRightAlignedLabel(int value, int row) {
  char buf[5];
  snprintf(buf, sizeof(buf), "%4d", value);
  for (int i = 0; i < 4; i++) {
    lcdPrintAt(12 + i, row, buf[i]);
  }
}

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

void printBigNumber(long long number, int x, int y) {
  char buf[22];
  long long value_to_print;
  char suffix = '\0';

  if (number < 1000) {
    value_to_print = number;
  } else if (number < 1000000) {
    value_to_print = number / 1000;
    suffix = 'K';
  } else if (number < 1000000000) {
    value_to_print = number / 1000000;
    suffix = 'M';
  } else {
    value_to_print = number / 1000000000;
    suffix = 'B';
  }

  // Convert value_to_print to string (manual lltoa)
  int i = sizeof(buf) - 1;
  buf[i--] = '\0';

  if (suffix != '\0') {
    buf[i--] = suffix;
  }

  if (value_to_print == 0) {
    buf[i--] = '0';
  } else {
    while (value_to_print > 0) {
      buf[i--] = (value_to_print % 10) + '0';
      value_to_print /= 10;
    }
  }
  
  lcdPrintAt(x, y, &buf[i + 1]);
} 

// Universal function for drawing farm buttons
void drawFarmButtons() {
  for (int i = 12; i < 16; i++) {
    for (int j = 0; j < 2; j++) {
      lcdPrintAt(i, j, "J");
    }
  }
} 

// Universal function for clearing a screen area
inline void clearArea(int x, int y, int width) {
  lcdPrintAt(x, y, "        "); // Clear a 16-character area
}

// Universal function for checking if the cursor is on a button
inline bool isCursorOnButton(int x, int y) {
  return cursorX == x && cursorY == y;
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
          lcdWriteAt(x, y, (byte)0); // cookie icon
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
