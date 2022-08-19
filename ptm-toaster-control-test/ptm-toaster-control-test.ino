/**
 * Tests control of the toaster.
 * 
 * Author: Shawn Hymel
 * Date: August 18, 2022
 * License: 0BSD
 */

// LCD library: comes with Wio Terminal package
#include "TFT_eSPI.h" 

// Pin settings
#define TOASTING_PIN    D2
#define CANCEL_PIN      D3
#define BTN_PIN         WIO_KEY_C

// Global objects
static TFT_eSPI tft;                  // Wio Terminal LCD
static TFT_eSprite spr = TFT_eSprite(&tft); // Sprite buffer

void setup() {

  // Configure toaster pins
  pinMode(TOASTING_PIN, INPUT);
  pinMode(CANCEL_PIN, OUTPUT);
  pinMode(BTN_PIN, INPUT_PULLUP);

  // Configure LCD
  tft.begin();
  tft.setRotation(3);
  spr.createSprite(TFT_HEIGHT, TFT_WIDTH);
  spr.setTextColor(TFT_WHITE);
  spr.setFreeFont(&FreeSansBoldOblique24pt7b);
  spr.fillRect(0, 0, TFT_HEIGHT, TFT_WIDTH, TFT_BLACK);
}

void loop() {

  // See if we're toasting
  int toasting = digitalRead(TOASTING_PIN);
  if (toasting) {

    // Print to LCD
    spr.fillRect(0, 0, TFT_HEIGHT, TFT_WIDTH, TFT_BLACK);
    spr.drawString("Toasting", 50, 100);
    spr.pushSprite(0, 0);

    // Cancel if button is pressed
    if (digitalRead(BTN_PIN) == 0) {
      digitalWrite(CANCEL_PIN, HIGH);
      delay(1000);
      digitalWrite(CANCEL_PIN, LOW);
    }

  // If not toasting, clear LCD
  } else {
    spr.fillRect(0, 0, TFT_HEIGHT, TFT_WIDTH, TFT_BLACK);
    spr.pushSprite(0, 0);
  }

  delay(10);
}
