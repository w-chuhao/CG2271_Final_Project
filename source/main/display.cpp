#include "display.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define I2C_SDA_PIN 10          
#define I2C_SCL_PIN 11          

// 0x27 is the most common I2C address; try 0x3F if the display is blank.
static LiquidCrystal_I2C lcd(0x27, 16, 2);

// ── Init ──────────────────────────────────────────────────────────
void displayInit() {
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("ESP32 System");
  lcd.setCursor(0, 1);
  lcd.print("Initialized...");
}

// ── Public: Normal operating screen ───────────────────────────────
void displayUpdate(const DeskState &state, uint8_t finalState) {
  // Line 0: Temperature + State label
  lcd.setCursor(0, 0);
  lcd.print("T:");
  if (isnan(state.temp)) {
    lcd.print("ERR  ");
  } else {
    lcd.print(state.temp, 1);
    lcd.print("C");
  }

  lcd.setCursor(8, 0);
  switch (finalState) {
    case NORMAL:   lcd.print(" S:NORM "); break;
    case WARNING:  lcd.print(" S:WARN "); break;
    case CRITICAL: lcd.print(" S:CRIT "); break;
    default:       lcd.print("        "); break;
  }

  // Line 1: Distance + Light
  lcd.setCursor(0, 1);
  lcd.print("D:");
  if (state.distance < 0) {
    lcd.print("ERR ");
  } else {
    lcd.print((int)state.distance);
    lcd.print("cm ");
  }

  lcd.setCursor(8, 1);
  lcd.print("L:");
  lcd.print(state.light);
  lcd.print("   ");  // Overwrite any leftover digits
}

void displayInactive() {
  lcd.setCursor(0, 0);
  lcd.print("SYSTEM INACTIVE ");
  lcd.setCursor(0, 1);
  lcd.print("                ");
}
