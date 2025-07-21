#include <LiquidCrystal.h>
#include <EEPROM.h>
#include <stdlib.h>

// Подключение LCD-экрана
constexpr uint8_t PIN_RS = 6;
constexpr uint8_t PIN_EN = 7;
constexpr uint8_t PIN_DB4 = 8;
constexpr uint8_t PIN_DB5 = 9;
constexpr uint8_t PIN_DB6 = 10;
constexpr uint8_t PIN_DB7 = 11;
LiquidCrystal lcd(PIN_RS, PIN_EN, PIN_DB4, PIN_DB5, PIN_DB6, PIN_DB7);

// Пользовательские символы
// Значок печенья
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
// Стрелка вверх
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
// Залитый прямоугольник (черный квадрат)
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
// Звездочка (новый символ)
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

// Переменные игры
long cookies = 0;
int cookiesPerClick = 1;

// Переменные курсора
int cursorX = 15;
int cursorY = 1;

// Переменные экранов
enum GameState {
  MAIN,
  SHOP,
  STATS,
  AUTOCLICK_SHOP,
  PRESTIGE_CONFIRM,
  MESSAGE_SCREEN
};
GameState currentScreen = MAIN;

// Переменные для обработки нажатий без задержек
unsigned long lastButtonPressTime = 0;
const unsigned long DEBOUNCE_DELAY = 200;

// Для мигания курсора
unsigned long lastBlinkTime = 0;
bool cursorVisible = true;
const unsigned long BLINK_INTERVAL = 1000; // 1 секунда

// Глобальные переменные для экрана сообщений
char messageLine1[17] = "";
char messageLine2[17] = "";
GameState screenAfterMessage = MAIN;
unsigned long messageDisplayStart = 0;
unsigned long messageTimeout = 0; // 0 = no timeout

// Бонусы престижа
int prestigeClickLevel = 1;
int prestigeAutoClickLevel = 0;

// Для бонуса за разряд
long lastCookieMagnitude = 0;

// Пины джойстика (центральная кнопка и 4 вокруг)
const int JOY_CENTER = 2;
const int JOY_UP = 3;
const int JOY_DOWN = 4;
const int JOY_LEFT = 5;
const int JOY_RIGHT = 12;

// Статистика
long totalCookies = 0;
long totalClicks = 0;
long totalUpgrades = 0;

// Для автосохранения
long lastSavedCookies = 0;

// --- Подарки ---
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

// Для подарков
bool giftActive = false;
int giftType = 0;
int giftPos = 8;
unsigned long lastGiftTime = 0;
const unsigned long GIFT_INTERVAL = 120000; // 2 минуты

// Для экрана поздравления
bool congratsActive = false;
unsigned long congratsStart = 0;
const unsigned long CONGRATS_TIME = 5000;

// Для временного бонуса
bool bonus573Active = false;
unsigned long bonus573Start = 0;
const unsigned long BONUS573_TIME = 60000;

// --- Переменные автокликера ---
int autoClickLevel = 0; // Начальный уровень 0 - не куплен
unsigned long lastAutoClickTime = 0;
const unsigned long AUTOCLICK_INTERVAL = 1000; // 1 секунда

void setup() {
  // Устанавливаем размер экрана (16 столбцов и 2 строки)
  lcd.begin(16, 2);
  
  // Создаем пользовательские символы
  lcd.createChar(0, starIcon); // Теперь 0 — это звездочка
  lcd.createChar(1, arrowUpIcon);
  lcd.createChar(2, filledBlock); // Символ для выделения курсора
  
  // Настройка пинов джойстика для работы с внешним pull-down резистором
  pinMode(JOY_CENTER, INPUT);
  pinMode(JOY_UP, INPUT);
  pinMode(JOY_DOWN, INPUT);
  pinMode(JOY_LEFT, INPUT);
  pinMode(JOY_RIGHT, INPUT);
  
  // Очищаем экран
  lcd.clear();
  // Показываем начальный экран
  displayManager();

  // Загрузка прогресса из EEPROM
  EEPROM.get(0, cookies);
  EEPROM.get(4, cookiesPerClick);
  EEPROM.get(8, totalCookies);
  EEPROM.get(12, totalClicks);
  EEPROM.get(16, totalUpgrades);
  EEPROM.get(20, autoClickLevel); // добавлено
  if (autoClickLevel < 0) autoClickLevel = 0; // защита
  EEPROM.get(24, prestigeClickLevel);
  EEPROM.get(28, prestigeAutoClickLevel);
  if (prestigeClickLevel < 1) prestigeClickLevel = 1;
  if (prestigeAutoClickLevel < 0) prestigeAutoClickLevel = 0;
  
  lastSavedCookies = cookies;

  randomSeed(analogRead(0));
  lastCookieMagnitude = pow(10, getDigitCount(cookies));
}

void loop() {
  // Проверка на бонус за новый разряд
  long currentMagnitude = pow(10, getDigitCount(cookies));
  if (currentMagnitude > lastCookieMagnitude && cookies > 10) {
    cookies *= 2;
    lastCookieMagnitude = currentMagnitude;
    showMessage("YOU ARE COOL", "", MAIN, 2000);
  }

  // Таймаут для экрана сообщений
  if (currentScreen == MESSAGE_SCREEN && messageTimeout > 0 && millis() - messageDisplayStart > messageTimeout) {
      currentScreen = screenAfterMessage;
      messageTimeout = 0; // Сбрасываем таймаут
  }

  // Появление подарка раз в 2 минуты
  if (!giftActive && !congratsActive && currentScreen == MAIN && millis() - lastGiftTime > GIFT_INTERVAL) {
    giftActive = true;
    giftType = random(GIFT_COUNT);
    giftPos = GIFT_POSITIONS[random(4)];
    lastGiftTime = millis();
  }
  // Окончание временного бонуса
  if (bonus573Active && millis() - bonus573Start > BONUS573_TIME) {
    bonus573Active = false;
  }
  // Экран поздравления
  if (congratsActive && millis() - congratsStart > CONGRATS_TIME) {
    congratsActive = false;
  }
  // Автокликер работает только если куплен (уровень > 0)
  if (currentScreen == MAIN && autoClickLevel > 0 && millis() - lastAutoClickTime > AUTOCLICK_INTERVAL) {
    cookies += getAutoClickPower(autoClickLevel);
    lastAutoClickTime = millis();
  }
  handleJoystick();
  handleButtonPress();
  displayManager();
  delay(100); // Контролируем частоту обновления, чтобы избежать мерцания
  // Мигалка курсора
  if (millis() - lastBlinkTime > BLINK_INTERVAL) {
    cursorVisible = !cursorVisible;
    lastBlinkTime = millis();
  }
}

void displayManager() {
  lcd.clear();
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
  // Первая строка: количество печенья (число + звездочка)
  int cookieDigits = getDigitCount(cookies);
  if (cookieDigits > 7) cookieDigits = 7;
  int cookiePos = 0;
  lcd.setCursor(cookiePos, 0);
  lcd.print(cookies);
  lcd.write((byte)0); // Звездочка

  // Подарок
  if (giftActive) {
    lcd.setCursor(giftPos, 0);
    lcd.print("#");
  }

  // Вторая строка: кнопка SHOP и символ 'a'
  lcd.setCursor(0, 1);
  lcd.print(F("SHOP"));
  lcd.print("a*"); // Добавили кнопку престижа

  // Последние 4 клетки обеих строк — кнопки для добычи печенек
  for (int i = 12; i < 16; i++) {
    lcd.setCursor(i, 0);
    lcd.print("J");
    lcd.setCursor(i, 1);
    lcd.print("J");
  }
}

void displayShopScreen() {
  // Первая строка: кнопка улучшения, инфо и цена
  lcd.setCursor(0, 0);
  lcd.write((byte)1); // Кнопка улучшения - стрелка вверх

  // Очищаем область для цены
  lcd.setCursor(1, 0);
  lcd.print("           "); // 11 пробелов, с 1 по 11 позицию

  // Отображаем цену
  long long cost = calculateUpgradeCost();
  printBigNumber(cost, 1, 0);

  // Будущая сила клика справа в первой строке
  printRightAligned4(getNextClickPower(cookiesPerClick), 0);
  
  // Вторая строка: кнопка возврата, надпись и уровень
  lcd.setCursor(0, 1);
  lcd.print(F("<"));
  lcd.setCursor(2, 1);
  lcd.print(F("Your Level"));
  printRightAligned4(getLevel(cookiesPerClick), 1);
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
}

void displayStarScreen() {
  // Экран статистики
  // 1 строка: T: (total cookies)
  lcd.setCursor(0, 0);
  lcd.print(F("T:"));
  char buf[13];
  snprintf(buf, sizeof(buf), "%11ld", totalCookies);
  for (int i = 0; i < 11; i++) {
    lcd.setCursor(2 + i, 0);
    lcd.print(buf[i]);
  }
  // Кнопка S (ручное сохранение) в самой правой клетке первой строки
  lcd.setCursor(15, 0);
  if (cursorX == 15 && cursorY == 0 && currentScreen == STATS) {
    lcd.write((byte)2);
  } else {
    lcd.print("S");
  }
  // 2 строка: < (выход) и статистика сдвинута вправо
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
  // Кнопка R (reset) в самой правой клетке второй строки
  lcd.setCursor(15, 1);
  if (cursorX == 15 && cursorY == 1 && currentScreen == STATS) {
    lcd.write((byte)2);
  } else {
    lcd.print("R");
  }
}

void displayCongratsScreen() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("Congratulations"));
  lcd.setCursor(0, 1);
  lcd.print(GIFT_TEXTS[giftType]);
}

void handleJoystick() {
  // Универсальная обработка джойстика для всех экранов
  if (digitalRead(JOY_RIGHT)) {
    if (cursorX < 15) cursorX++;
    delay(200);
  }
  if (digitalRead(JOY_LEFT)) {
    if (cursorX > 0) cursorX--;
    delay(200);
  }
  if (digitalRead(JOY_UP) == HIGH) {
    if (cursorY > 0) cursorY--;
    delay(200);
  }
  if (digitalRead(JOY_DOWN) == HIGH) {
    if (cursorY < 1) cursorY++;
    delay(200);
  }
}

void handleButtonPress() {
  static bool lastState = false;
  static unsigned long lastAutoCraftTime = 0;
  bool pressed = digitalRead(JOY_CENTER);
  unsigned long now = millis();

  // --- Автокрафт при удержании кнопки в зоне J-кнопок ---
  if (currentScreen == MAIN && !congratsActive && pressed && cursorX >= 12) {
    if (now - lastAutoCraftTime > DEBOUNCE_DELAY) {
      int clickValue = bonus573Active ? (cookiesPerClick + 573) : cookiesPerClick;
      cookies += clickValue;
      totalCookies += clickValue;
      totalClicks++;
      lastAutoCraftTime = now;
    }
    lastState = pressed;
    return;
  }

  if (pressed && !lastState && now - lastButtonPressTime > DEBOUNCE_DELAY) {
    lastButtonPressTime = now;
    
    if (congratsActive) {
        lastState = pressed;
        return;
    }

    switch (currentScreen) {
      case MAIN:
        // --- Обработка нажатия на подарок ---
        if (giftActive && cursorY == 0 && cursorX == giftPos) {
          activateGift();
        }
        // Нажатие на SHOP
        else if (cursorY == 1 && cursorX < 4) {
          currentScreen = SHOP;
        }
        // Нажатие на символ 'a'
        else if (cursorY == 1 && cursorX == 4) {
          currentScreen = AUTOCLICK_SHOP;
          cursorX = 0; // Сбрасываем позицию курсора на кнопку выхода
          cursorY = 1; // Теперь всегда на кнопке выхода
        }
        // Нажатие на кнопку престижа '*'
        else if (cursorY == 1 && cursorX == 5) {
            if (cookies >= 1000000) {
                currentScreen = PRESTIGE_CONFIRM;
                cursorX = 0; // ставим курсор на NO
                cursorY = 1;
            } else {
                showMessage("YOU HAVEN'T", "ENOUGH COOKIES", MAIN, 5000);
            }
        }
        // Нажатие на звездочку
        else if (cursorY == 0 && cursorX == getDigitCount(cookies)) {
          currentScreen = STATS;
          cursorX = 0; // Кнопка выхода
          cursorY = 1;
        }
        // --- Клик по любой другой клетке ---
        else {
          int clickValue = bonus573Active ? (cookiesPerClick + 573) : cookiesPerClick;
          cookies += clickValue;
          totalCookies += clickValue;
          totalClicks++;
        }
        break;

      case AUTOCLICK_SHOP:
        // Кнопка выхода теперь на второй строке
        if (cursorY == 1 && cursorX == 0) {
          currentScreen = MAIN;
        }
        // Кнопка апгрейда теперь на (0,0)
        else if (cursorY == 0 && cursorX == 0) {
          long long cost = calculateAutoClickUpgradeCost();
          if (cookies >= cost) {
            cookies -= cost;
            autoClickLevel++;
            showMessage("BOUGHT", "", AUTOCLICK_SHOP, 2000);
          }
        }
        break;

      case SHOP:
        // Кнопка возврата
        if (cursorY == 1 && cursorX == 0) {
          currentScreen = MAIN;
        }
        // Кнопка апгрейда
        else if (cursorY == 0 && cursorX == 0) {
          long long cost = calculateUpgradeCost();
          if (cookies >= cost) {
            cookies -= cost;
            cookiesPerClick = getNextClickPower(cookiesPerClick);
            totalUpgrades++;
            showMessage("BOUGHT", "", SHOP, 2000);
          }
        }
        break;

      case STATS:
        // Кнопка выхода <
        if (cursorY == 1 && cursorX == 0) {
          currentScreen = MAIN;
        }
        // Кнопка сохранения S
        else if (cursorY == 0 && cursorX == 15) {
          manualSave();
        }
        // Кнопка сброса R
        else if (cursorY == 1 && cursorX == 15) {
          manualReset();
        }
        break;

      case PRESTIGE_CONFIRM:
        // NO
        if (cursorY == 1 && cursorX >= 0 && cursorX <= 2) {
            currentScreen = MAIN;
        }
        // YES
        else if (cursorY == 1 && cursorX >= 6 && cursorX <= 9) {
            activatePrestige();
        }
        break;

      case MESSAGE_SCREEN:
        // любое нажатие вернет на предыдущий экран
        currentScreen = screenAfterMessage;
        messageTimeout = 0; // Сбрасываем таймаут при ручном пропуске
        break;
    }
    
    lastState = pressed;
  }
}

void displayCursor() {
  if (!cursorVisible) return;
  // Логика теперь универсальна для всех экранов
  lcd.setCursor(cursorX, cursorY);
  lcd.write((byte)2);
}

long long calculateUpgradeCost() {
  int level = getLevel(cookiesPerClick);
  
  // Новый алгоритм: экспоненциальный рост.
  // Базовая стоимость 100, каждый уровень на 15% дороже.
  long long cost = 100;
  int growth_factor_scaled = 115; // 1.15 * 100

  for (int i = 1; i < level; i++) {
    // Умножаем на 1.15, используя целочисленную арифметику
    cost = (cost * growth_factor_scaled) / 100;
  }

  // Небольшая добавка, чтобы цена росла даже на первых уровнях
  if (level > 1) {
      cost += level * 10;
  }

  return cost;
}

int getDigitCount(long number) {
    if (number < 10) return 1;
    if (number < 100) return 2;
    if (number < 1000) return 3;
    if (number < 10000) return 4;
    if (number < 100000) return 5;
    if (number < 1000000) return 6;
    if (number < 10000000) return 7;
    return 7; // Ограничение в 7 символов
}

// Вспомогательная функция: вычисление уровня
int getLevel(int clickPower) {
    return clickPower / 2 + 1;
}

// Вспомогательная функция: вычисление будущей силы клика
int getNextClickPower(int clickPower) {
  return (clickPower == 1) ? 2 : (clickPower + 2);
}

// Универсальная функция: вывод числа справа в 4 клетки
void printRightAligned4(int value, int row) {
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
    EEPROM.put(0, cookies);
    EEPROM.put(4, cookiesPerClick);
    EEPROM.put(8, totalCookies);
    EEPROM.put(12, totalClicks);
    EEPROM.put(16, totalUpgrades);
    EEPROM.put(20, autoClickLevel); // добавлено
    EEPROM.put(24, prestigeClickLevel);
    EEPROM.put(28, prestigeAutoClickLevel);
    lastSavedCookies = cookies;
  }
}

void manualSave() {
  EEPROM.put(0, cookies);
  EEPROM.put(4, cookiesPerClick);
  EEPROM.put(8, totalCookies);
  EEPROM.put(12, totalClicks);
  EEPROM.put(16, totalUpgrades);
  EEPROM.put(20, autoClickLevel); // добавлено
  EEPROM.put(24, prestigeClickLevel);
  EEPROM.put(28, prestigeAutoClickLevel);
  lastSavedCookies = cookies;
}

void manualReset() {
  cookies = 0;
  cookiesPerClick = prestigeClickLevel;
  totalCookies = 0;
  totalClicks = 0;
  totalUpgrades = 0;
  autoClickLevel = prestigeAutoClickLevel; // добавлено
  manualSave();
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
}

void activateGift() {
  switch (giftType) {
    case 0: cookies += 100; totalCookies += 100; break;
    case 1: cookies += 500; totalCookies += 500; break;
    case 2: cookies += 1000; totalCookies += 1000; break;
    case 3: cookies += 5000; totalCookies += 5000; break;
    case 4: cookies += 10000; totalCookies += 10000; break;
    case 5: cookies += 15000; totalCookies += 15000; break;
    case 6: cookiesPerClick += 50; showMessage("BOUGHT", "+50 to click", MAIN, 2000); break;
    case 7: cookiesPerClick += 2; totalUpgrades++; showMessage("BOUGHT", "+1 level", MAIN, 2000); break;
    case 8: bonus573Active = true; bonus573Start = millis(); break;
  }
  tryAutoSave();
  giftActive = false;
  congratsActive = true;
  congratsStart = millis();
}

void displayAScreen() {
  lcd.clear();
  // Кнопка апгрейда
  lcd.setCursor(0, 0);
  lcd.write((byte)1); // стрелка вверх
  // Цена улучшения
  int safeLevel = autoClickLevel < 0 ? 0 : autoClickLevel;
  long long cost = calculateAutoClickUpgradeCost();
  printBigNumber(cost, 1, 0);
  
  // Доход в секунду (после покупки, последние 3 клетки)
  int income = getAutoClickPower(safeLevel + 1);
  char incbuf[4];
  snprintf(incbuf, sizeof(incbuf), "%3d", income);
  lcd.setCursor(13, 0);
  lcd.print(incbuf);
  // Вторая строка: кнопка выхода, Your Level и уровень автоклика
  lcd.setCursor(0, 1);
  lcd.print("<");
  lcd.setCursor(2, 1);
  lcd.print(F("Your Level"));
  char lvlbuf[4];
  snprintf(lvlbuf, sizeof(lvlbuf), "%3d", safeLevel);
  lcd.setCursor(13, 1);
  lcd.print(lvlbuf);
}

long long calculateAutoClickUpgradeCost() {
  int safeLevel = autoClickLevel < 0 ? 0 : autoClickLevel;
  // Для первой покупки - фиксированная цена
  if (safeLevel == 0) return 1000;
  int power = getAutoClickPower(safeLevel + 1); // считаем для следующего уровня
  long long cost = (long long)power * power * 100L;
  if (safeLevel + 1 > 15) cost *= (power / 2);
  return cost;
}

int getAutoClickPower(int level) {
  if (level == 0) return 0; // Если не куплен - мощность 0
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
