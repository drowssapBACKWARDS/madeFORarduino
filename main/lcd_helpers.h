#ifndef LCD_HELPERS_H
#define LCD_HELPERS_H

#include "config.h"

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

#endif // LCD_HELPERS_H 
