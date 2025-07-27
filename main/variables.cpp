#include <Arduino.h>
#include "config.h"

// Custom Characters
// Cookie Icon
uint8_t cookieIcon[8] = {
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
uint8_t arrowUpIcon[8] = {
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
uint8_t filledBlock[8] = {
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
uint8_t starIcon[8] = {
  B00100,
  B10101,
  B01110,
  B11111,
  B01110,
  B10101,
  B00100,
  B00000
};

// LCD object
LiquidCrystal lcd(PIN_RS, PIN_EN, PIN_DB4, PIN_DB5, PIN_DB6, PIN_DB7);

// Game Variables
long cookies = 0;
int cookiesPerClick = 1;

// Cursor Variables
int cursorX = 15;
int cursorY = 1;
int prevCursorX = 15;
int prevCursorY = 1;

// Screen State Management
GameState currentScreen = MAIN;
GameState lastScreen = MAIN;

// Non-blocking Timers
unsigned long lastButtonPressTime = 0;
unsigned long lastJoystickMoveTime = 0;

// Cursor Blinking
unsigned long lastBlinkTime = 0;
bool cursorVisible = true;

// Message Screen Variables
char messageLine1[17] = "";
char messageLine2[17] = "";
GameState screenAfterMessage = MAIN;
unsigned long messageDisplayStart = 0;
unsigned long messageTimeout = 0; // 0 = no timeout

// Prestige Bonuses
int prestigeClickLevel = 1;
int prestigeAutoClickLevel = 0;

// Milestone Bonus Variable
long nextMilestone = 100;

// Statistics
long totalCookies = 0;
long totalClicks = 0;
long totalUpgrades = 0;

// Auto-save
long lastSavedCookies = 0;

// --- Gifts ---
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

// Congratulations Screen
bool congratsActive = false;
unsigned long congratsStart = 0;

// Temporary Bonus
bool bonus573Active = false;
unsigned long bonus573Start = 0;

// --- Autoclicker Variables ---
int autoClickLevel = 0; // Level 0 means not purchased
unsigned long lastAutoClickTime = 0;

bool needRedraw = true;

// Screen state structure
ScreenState prevState = {-1, -1, false, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}; 
