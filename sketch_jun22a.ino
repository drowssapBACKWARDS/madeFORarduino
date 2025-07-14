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
bool inShop = false;
bool inStarScreen = false;
bool inAScreen = false;

// Переменные для обработки нажатий без задержек
unsigned long lastButtonPressTime = 0;
const unsigned long DEBOUNCE_DELAY = 200;

// Для мигания курсора
unsigned long lastBlinkTime = 0;
bool cursorVisible = true;
const unsigned long BLINK_INTERVAL = 1000; // 1 секунда

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
  lastSavedCookies = cookies;

  randomSeed(analogRead(0));
}

void loop() {
  // Появление подарка раз в 2 минуты
  if (!giftActive && !congratsActive && !inShop && !inStarScreen && !inAScreen && millis() - lastGiftTime > GIFT_INTERVAL) {
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
  if (!inShop && !inStarScreen && !inAScreen && autoClickLevel > 0 && millis() - lastAutoClickTime > AUTOCLICK_INTERVAL) {
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
  } else if (inStarScreen) {
    displayStarScreen();
  } else if (inShop) {
    displayShopScreen();
  } else if (inAScreen) {
    displayAScreen();
  } else {
    displayMainScreen();
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
  lcd.print("a");

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

  lcd.setCursor(2, 0);
  lcd.write((byte)0); // Значок печенья
  lcd.print(F("L FOR"));

  long cost = calculateUpgradeCost();
  char costbuf[8];
  int rightMargin = 13; // 3 cells from right edge (16 - 3 = 13)
  int costStart = 7; // Position after "L FOR"
  int costSpace = rightMargin - costStart;
  
  if (cost > 999999) {
    snprintf(costbuf, sizeof(costbuf), "MAX");
  } else {
    snprintf(costbuf, sizeof(costbuf), "%ld", cost);
  }
  
  // Center the cost in available space
  int costLen = strlen(costbuf);
  int padding = (costSpace - costLen) / 2;
  lcd.setCursor(costStart + padding, 0);
  lcd.print(costbuf);

  // Будущая сила клика справа в первой строке
  printRightAligned4(getNextClickPower(cookiesPerClick), 0);

  // Вторая строка: кнопка возврата и надпись Your Level
  lcd.setCursor(0, 1);
  lcd.print(F("<Your Level"));

  // Уровень справа во второй строке
  printRightAligned4(getLevel(cookiesPerClick), 1);
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
  lcd.print("S");
  // 2 строка: < (выход) и статистика сдвинута вправо
  lcd.setCursor(0, 1);
  lcd.print(F("<"));
  lcd.setCursor(1, 1);
  lcd.print(F("L:"));
  lcd.print(getLevel(cookiesPerClick));
  lcd.print(F(" U:"));
  lcd.print(totalUpgrades);
  lcd.print(F(" C:"));
  lcd.print(totalClicks);
  // Кнопка R (reset) в самой правой клетке второй строки
  lcd.setCursor(15, 1);
  lcd.print("R");
}

void displayCongratsScreen() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("Congratulations"));
  lcd.setCursor(0, 1);
  lcd.print(GIFT_TEXTS[giftType]);
}

void handleJoystick() {
  if (inAScreen) {
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
    return;
  }

  // Обработка джойстика на главном экране
  if (!inShop && !inStarScreen && !inAScreen) {
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
  // ... существующий код для других экранов ...
}

void handleButtonPress() {
  static bool lastState = false;
  static unsigned long lastAutoCraftTime = 0;
  bool pressed = digitalRead(JOY_CENTER);
  unsigned long now = millis();

  // --- Автокрафт при удержании кнопки в зоне J-кнопок ---
  if (!inShop && !inStarScreen && !inAScreen && !congratsActive && pressed && cursorX >= 12) {
    if (now - lastAutoCraftTime > DEBOUNCE_DELAY) {
      cookies += cookiesPerClick;
      totalCookies += cookiesPerClick;
      totalClicks++;
      lastAutoCraftTime = now;
    }
    lastState = pressed;
    return;
  }

  if (pressed && !lastState && now - lastButtonPressTime > DEBOUNCE_DELAY) {
    lastButtonPressTime = now;
    // На главном экране
    if (!inShop && !inStarScreen && !inAScreen && !congratsActive) {
      // Нажатие на SHOP
      if (cursorY == 1 && cursorX < 4) {
        inShop = true;
        lastState = pressed;
        return;
      }
      // Нажатие на символ 'a'
      if (cursorY == 1 && cursorX == 4) {
        inAScreen = true;
        cursorX = 0; // Сбрасываем позицию курсора на кнопку выхода
        cursorY = 0;
        lastState = pressed;
        return;
      }
      // Нажатие на звездочку
      if (cursorY == 0 && cursorX == getDigitCount(cookies)) {
        inStarScreen = true;
        lastState = pressed;
        return;
      }
      // Нажатие на J-кнопки (обрабатывается выше)
    }
    // В магазине автоклика
    if (inAScreen) {
      // Кнопка выхода теперь на второй строке
      if (cursorY == 1 && cursorX == 0) {
        inAScreen = false;
        lastState = pressed;
        return;
      }
      // Кнопка апгрейда теперь на (0,0)
      if (cursorY == 0 && cursorX == 0) {
        long cost = calculateAutoClickUpgradeCost();
        if (cookies >= cost && cost < 999999) {
          cookies -= cost;
          autoClickLevel++;
        }
        lastState = pressed;
        return;
      }
    }
    // ... остальной существующий код ...
  }
  lastState = pressed;
}

void displayCursor() {
  if (!cursorVisible) return;

  if (inAScreen) {
    lcd.setCursor(cursorX, cursorY);
    lcd.write((byte)2);
    return;
  }

  if (inShop) {
    lcd.setCursor(cursorX, cursorY);
    lcd.write((byte)2);
    return;
  }

  if (inStarScreen) {
    // ... существующий код для экрана звезды ...
  } else {
    // На главном экране — подсвечиваем любую клетку
    lcd.setCursor(cursorX, cursorY);
    lcd.write((byte)2);
  }
}

long calculateUpgradeCost() {
  if (cookiesPerClick < 15) {
    return (long)cookiesPerClick * cookiesPerClick * 100;
  } else {
    return (long)cookiesPerClick * cookiesPerClick * 100 * (cookiesPerClick / 2);
  }
}

int getDigitCount(int number) {
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
    lastSavedCookies = cookies;
  }
}

void manualSave() {
  EEPROM.put(0, cookies);
  EEPROM.put(4, cookiesPerClick);
  EEPROM.put(8, totalCookies);
  EEPROM.put(12, totalClicks);
  EEPROM.put(16, totalUpgrades);
  lastSavedCookies = cookies;
}

void manualReset() {
  cookies = 0;
  cookiesPerClick = 1;
  totalCookies = 0;
  totalClicks = 0;
  totalUpgrades = 0;
  manualSave();
}

void activateGift() {
  switch (giftType) {
    case 0: cookies += 100; totalCookies += 100; break;
    case 1: cookies += 500; totalCookies += 500; break;
    case 2: cookies += 1000; totalCookies += 1000; break;
    case 3: cookies += 5000; totalCookies += 5000; break;
    case 4: cookies += 10000; totalCookies += 10000; break;
    case 5: cookies += 15000; totalCookies += 15000; break;
    case 6: cookiesPerClick += 50; break;
    case 7: cookiesPerClick += 2; totalUpgrades++; break;
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
  long cost = calculateAutoClickUpgradeCost();
  char costbuf[8];
  if (cost > 999999) {
    snprintf(costbuf, sizeof(costbuf), "MAX");
  } else {
    snprintf(costbuf, sizeof(costbuf), "%ld", cost);
  }
  lcd.setCursor(1, 0);
  lcd.print(costbuf);
  // Доход в секунду (после покупки, последние 3 клетки)
  int income = getAutoClickPower(autoClickLevel + 1);
  char incbuf[4];
  snprintf(incbuf, sizeof(incbuf), "%3d", income);
  lcd.setCursor(13, 0);
  lcd.print(incbuf);
  // Вторая строка: кнопка выхода, Your Level и уровень автоклика
  lcd.setCursor(0, 1);
  lcd.print("<");
  lcd.setCursor(1, 1);
  lcd.print(F("Your Level"));
  char lvlbuf[4];
  snprintf(lvlbuf, sizeof(lvlbuf), "%3d", autoClickLevel);
  lcd.setCursor(13, 1);
  lcd.print(lvlbuf);
}

long calculateAutoClickUpgradeCost() {
  // Для первой покупки - фиксированная цена
  if (autoClickLevel == 0) return 1000;
  
  int power = getAutoClickPower(autoClickLevel);
  long cost = (power * power * 100L);
  if (autoClickLevel > 15) cost *= (power / 2);
  return cost;
}

int getAutoClickPower(int level) {
  if (level == 0) return 0; // Если не куплен - мощность 0
  if (level == 1) return 1;
  if (level <= 15) return 1 + (level - 1) * 2;
  return 1 + (14 * 2) + (level - 15) * 4;
} 
