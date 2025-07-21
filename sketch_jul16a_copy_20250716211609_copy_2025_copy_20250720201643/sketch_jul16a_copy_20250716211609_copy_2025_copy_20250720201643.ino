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
const unsigned long DEBOUNCE_DELAY = 200;
unsigned long lastJoystickMoveTime = 0;
const unsigned long JOYSTICK_DELAY = 150;

// Cursor Blinking
unsigned long lastBlinkTime = 0;
bool cursorVisible = true;
const unsigned long BLINK_INTERVAL = 1000;

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

// === Оптимизация: константы и структура состояния экрана ===
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
    int cookieDigits = getDigitCount(cookies);
    if (cookieDigits > MAX_DIGITS) cookieDigits = MAX_DIGITS;
    int cookiePos = 0;
    lcd.setCursor(cookiePos, 0);
    lcd.print("        "); // Clear area
    lcd.setCursor(cookiePos, 0);
    lcd.print(cookies);
    lcd.write((byte)0);
    prevState.cookies = cookies;
  }
  if (giftActive != prevState.giftActive || giftPos != prevState.giftPos) {
    if (giftActive) {
      lcd.setCursor(giftPos, 0);
      lcd.print("#");
    } else if (prevState.giftActive) {
      lcd.setCursor(prevState.giftPos, 0);
      lcd.print(" ");
    }
    prevState.giftActive = giftActive;
    prevState.giftPos = giftPos;
  }
  lcd.setCursor(0, 1);
  lcd.print(F("SHOP"));
  lcd.print("a*");
  drawFarmButtons();
}

void displayShopScreen() {
  long long cost = calculateUpgradeCost();
  int nextClick = getNextClickPower(cookiesPerClick);
  int level = getLevel(cookiesPerClick);
  if (cost != prevState.shopCost || nextClick != prevState.shopNextClick || level != prevState.shopLevel) {
    lcd.setCursor(0, 0);
    drawButton(0, 0, "^", cursorX == 0 && cursorY == 0 && currentScreen == SHOP);
    lcd.setCursor(1, 0);
    lcd.print("           ");
    printRightAligned(cost, 1, 0, 4);
    drawRightAlignedLabel(nextClick, 0);
    drawButton(0, 1, "<", cursorX == 0 && cursorY == 1 && currentScreen == SHOP);
    lcd.setCursor(2, 1);
    lcd.print(F("Your Level"));
    drawRightAlignedLabel(level, 1);
    prevState.shopCost = cost;
    prevState.shopNextClick = nextClick;
    prevState.shopLevel = level;
  }
}

void displayPrestigeConfirmScreen() {
    lcd.setCursor(0, 0);
    lcd.print(F("ARE YOU SURE?"));
    lcd.setCursor(0, 1);
    lcd.print(F("NO    YES"));
}

void displayMessageScreen() {
    lcd.setCursor(0, 0);
    lcd.print(messageLine1);
    lcd.setCursor(0, 1);
    lcd.print(messageLine2);
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
    lcd.setCursor(0, 0);
    lcd.print(F("T:"));
    char buf[13];
    snprintf(buf, sizeof(buf), "%11ld", totalCookies);
    for (int i = 0; i < 11; i++) {
      lcd.setCursor(2 + i, 0);
      lcd.print(buf[i]);
    }
    lcd.setCursor(15, 0);
    if (cursorX == 15 && cursorY == 0 && currentScreen == STATS) {
      lcd.write((byte)2);
    } else {
      lcd.print("S");
    }
    lcd.setCursor(0, 1);
    if (cursorX == 0 && cursorY == 1 && currentScreen == STATS) {
      lcd.write((byte)2);
    } else {
      lcd.print("<");
    }
    lcd.setCursor(1, 1);
    lcd.print(F("L:"));
    lcd.print(getLevel(cookiesPerClick));
    lcd.print(F(" U:"));
    lcd.print(totalUpgrades);
    lcd.print(F(" C:"));
    lcd.print(totalClicks);
    lcd.setCursor(15, 1);
    if (cursorX == 15 && cursorY == 1 && currentScreen == STATS) {
      lcd.write((byte)2);
    } else {
      lcd.print("R");
    }
    prevState.totalCookies = totalCookies;
    prevState.statsLevel = getLevel(cookiesPerClick);
    prevState.totalUpgrades = totalUpgrades;
    prevState.totalClicks = totalClicks;
  }
}

void displayCongratsScreen() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("Congratulations"));
  lcd.setCursor(0, 1);
  lcd.print(GIFT_TEXTS[giftType]);
  needRedraw = true;
}

void handleJoystick() {
  unsigned long now = millis();
  if (now - lastJoystickMoveTime < JOYSTICK_DELAY) {
    return;
  }
  if (digitalRead(JOY_RIGHT) && cursorX < 15) {
    cursorX++;
    lastJoystickMoveTime = now;
  } else if (digitalRead(JOY_LEFT) && cursorX > 0) {
    cursorX--;
    lastJoystickMoveTime = now;
  } else if (digitalRead(JOY_UP) == HIGH && cursorY > 0) {
    cursorY--;
    lastJoystickMoveTime = now;
  } else if (digitalRead(JOY_DOWN) == HIGH && cursorY < 1) {
    cursorY++;
    lastJoystickMoveTime = now;
  }
}

void handleButtonPress() {
  static bool lastState = false;
  static unsigned long lastAutoCraftTime = 0;
  bool pressed = digitalRead(JOY_CENTER);
  unsigned long now = millis();

  // This block handles HELD presses, specifically for autocrafting on the main screen.
  // It has its own timer and returns immediately, separate from the single-press logic below.
  if (currentScreen == MAIN && !congratsActive && pressed && cursorX >= BUTTON_FARM_X_START) {
    if (now - lastAutoCraftTime > DEBOUNCE_DELAY) {
      int clickValue = bonus573Active ? (cookiesPerClick + 573) : cookiesPerClick;
      cookies += clickValue;
      totalCookies += clickValue;
      totalClicks++;
      lastAutoCraftTime = now;
      needRedraw = true;
    }
    lastState = pressed;
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
        // Star button (stats screen)
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
        // Save button S
        else if (cursorY == 0 && cursorX == BUTTON_SAVE_X) {
          manualSave();
          needRedraw = true;
        }
        // Reset button R
        else if (cursorY == 1 && cursorX == BUTTON_RESET_X) {
          manualReset();
          needRedraw = true;
        }
        break;
    }
    
    lastState = pressed;
  }
}

void displayCursor() {
  if (!cursorVisible) return;
  // Logic is now universal for all screens
  lcd.setCursor(cursorX, cursorY);
  lcd.write((byte)2);
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

// Оптимизированная getDigitCount
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

// Универсальная функция вывода числа справа
void printRightAligned(int value, int row, int col, int width) {
  char buf[8];
  snprintf(buf, sizeof(buf), "%*d", width, value);
  for (int i = 0; i < width; i++) {
    lcd.setCursor(col + i, row);
    lcd.print(buf[i]);
  }
}

// Универсальная функция для кнопки
void drawButton(int x, int y, const char* label, bool highlight) {
  lcd.setCursor(x, y);
  highlight ? lcd.write((byte)2) : lcd.print(label);
}

// Helper: Draw a right-aligned label in 4 cells
void drawRightAlignedLabel(int value, int row) {
  char buf[5];
  snprintf(buf, sizeof(buf), "%4d", value);
  for (int i = 0; i < 4; i++) {
    lcd.setCursor(12 + i, row);
    lcd.print(buf[i]);
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
    lcd.setCursor(0, 0);
    drawButton(0, 0, "^", cursorX == 0 && cursorY == 0 && currentScreen == AUTOCLICK_SHOP);
    printRightAligned(cost, 1, 0, 4);
    char incbuf[4];
    snprintf(incbuf, sizeof(incbuf), "%3d", income);
    lcd.setCursor(13, 0);
    lcd.print(incbuf);
    drawButton(0, 1, "<", cursorX == 0 && cursorY == 1 && currentScreen == AUTOCLICK_SHOP);
    lcd.setCursor(2, 1);
    lcd.print(F("Your Level"));
    char lvlbuf[4];
    snprintf(lvlbuf, sizeof(lvlbuf), "%3d", level);
    lcd.setCursor(13, 1);
    lcd.print(lvlbuf);
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
  
  lcd.setCursor(x, y);
  lcd.print(&buf[i + 1]);
} 

// Универсальная функция для отрисовки фермерских кнопок
void drawFarmButtons() {
  for (int i = 12; i < 16; i++) {
    for (int j = 0; j < 2; j++) {
      lcd.setCursor(i, j);
      lcd.print("J");
    }
  }
} 

// Универсальная функция для очистки области экрана
void clearArea(int x, int y, int width) {
  lcd.setCursor(x, y);
  for (int i = 0; i < width; i++) lcd.print(" ");
}

// Универсальная функция для проверки положения курсора на кнопке
inline bool isCursorOnButton(int x, int y) {
  return cursorX == x && cursorY == y;
}

// Сброс состояния экрана
void resetPrevScreenVars() {
  memset(&prevState, -1, sizeof(prevState));
} 
