#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
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

// Custom Characters
// Cookie Icon
extern uint8_t cookieIcon[8];
// Up Arrow Icon
extern uint8_t arrowUpIcon[8];
// Filled Block (for cursor)
extern uint8_t filledBlock[8];
// Star Icon
extern uint8_t starIcon[8];

// Game Variables
extern long cookies;
extern int cookiesPerClick;

// Cursor Variables
extern int cursorX;
extern int cursorY;
extern int prevCursorX;
extern int prevCursorY;

// Screen State Management
enum GameState {
  MAIN,
  SHOP,
  STATS,
  AUTOCLICK_SHOP,
  PRESTIGE_CONFIRM,
  MESSAGE_SCREEN
};
extern GameState currentScreen;
extern GameState lastScreen;

// Non-blocking Timers
extern unsigned long lastButtonPressTime;
const unsigned long DEBOUNCE_DELAY = 600;
extern unsigned long lastJoystickMoveTime;
const unsigned long JOYSTICK_DELAY = 200;

// Cursor Blinking
extern unsigned long lastBlinkTime;
extern bool cursorVisible;
const unsigned long BLINK_INTERVAL = 450;

// Message Screen Variables
extern char messageLine1[17];
extern char messageLine2[17];
extern GameState screenAfterMessage;
extern unsigned long messageDisplayStart;
extern unsigned long messageTimeout; // 0 = no timeout

// Prestige Bonuses
extern int prestigeClickLevel;
extern int prestigeAutoClickLevel;

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
extern long nextMilestone;

// Joystick Pins
const int JOY_CENTER = 2;
const int JOY_UP = 3;
const int JOY_DOWN = 4;
const int JOY_LEFT = 5;
const int JOY_RIGHT = 12;

// Statistics
extern long totalCookies;
extern long totalClicks;
extern long totalUpgrades;

// Auto-save
extern long lastSavedCookies;

// --- Gifts ---
const int GIFT_POSITIONS[4] = {8, 9, 10, 11};
const int GIFT_COUNT = 9;
extern const char* GIFT_TEXTS[GIFT_COUNT];

// Gift Variables
extern bool giftActive;
extern int giftType;
extern int giftPos;
extern unsigned long lastGiftTime;
const unsigned long GIFT_INTERVAL = 120000; // 2 minutes

// Congratulations Screen
extern bool congratsActive;
extern unsigned long congratsStart;
const unsigned long CONGRATS_TIME = 5000;

// Temporary Bonus
extern bool bonus573Active;
extern unsigned long bonus573Start;
const unsigned long BONUS573_TIME = 60000;

// --- Autoclicker Variables ---
extern int autoClickLevel; // Level 0 means not purchased
extern unsigned long lastAutoClickTime;
const unsigned long AUTOCLICK_INTERVAL = 1000; // 1 second

extern bool needRedraw;

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
extern ScreenState prevState;

// LCD object
extern LiquidCrystal lcd;

#endif // CONFIG_H 
