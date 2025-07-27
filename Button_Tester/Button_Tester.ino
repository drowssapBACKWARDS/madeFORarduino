#include <LiquidCrystal.h>

// --- ПИНЫ ---
// Подключение LCD-экрана
constexpr uint8_t PIN_RS = 6;
constexpr uint8_t PIN_EN = 7;
constexpr uint8_t PIN_DB4 = 8;
constexpr uint8_t PIN_DB5 = 9;
constexpr uint8_t PIN_DB6 = 10;
constexpr uint8_t PIN_DB7 = 11;
LiquidCrystal lcd(PIN_RS, PIN_EN, PIN_DB4, PIN_DB5, PIN_DB6, PIN_DB7);

// Пины джойстика
const int JOY_CENTER = 2;
const int JOY_UP = 3;
const int JOY_DOWN = 4;
const int JOY_LEFT = 5;
const int JOY_RIGHT = 12;

void setup() {
  // Инициализация LCD
  lcd.begin(16, 2);
  
  // Настройка пинов джойстика для работы с внешним pull-down резистором
  pinMode(JOY_CENTER, INPUT);
  pinMode(JOY_UP, INPUT);
  pinMode(JOY_DOWN, INPUT);
  pinMode(JOY_LEFT, INPUT);
  pinMode(JOY_RIGHT, INPUT);
  
  lcd.print("Press a button");
}

void loop() {
  lcd.setCursor(0, 1);
  lcd.print("                "); // Очищаем вторую строку
  lcd.setCursor(0, 1);

  // Теперь проверяем на HIGH, так как кнопки подключены к 5V
  if (digitalRead(JOY_UP) == HIGH) {
    lcd.print("UP");
  } else if (digitalRead(JOY_DOWN) == HIGH) {
    lcd.print("DOWN");
  } else if (digitalRead(JOY_LEFT) == HIGH) {
    lcd.print("LEFT");
  } else if (digitalRead(JOY_RIGHT) == HIGH) {
    lcd.print("RIGHT");
  } else if (digitalRead(JOY_CENTER) == HIGH) {
    lcd.print("CENTER");
  } else {
    // Ничего не нажато
  }
  
  delay(100); // Небольшая задержка для стабильности
} 
