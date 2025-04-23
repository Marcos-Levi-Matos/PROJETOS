#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif

#define RGB_PIN 13
#define LED_COUNT 20

Adafruit_NeoPixel leds(LED_COUNT, RGB_PIN, NEO_GRB + NEO_KHZ800);
LiquidCrystal_I2C lcd(0x27, 16, 2);

Servo servo1;
Servo servo2;

const int clk1 = 2, dt1 = 3, sw1 = 4, clk2 = 5, dt2 = 6, sw2 = 7, pinLose = 10, pinWin = 11, pinBuzz = 12;
int pos1 = 83, lastClk1, pos2 = 83, lastClk2;

int gameState = 0;
unsigned long timerNow = 0, timerEnc = 0, timerLCD = 0, timerLose = 0, timerWin = 0, timerLed = 0, timerPlay = 0, elapsed = 0;
int blinkState = 0, pixelIdx = 0, hue = 0;
unsigned long blinkStart = 0;

void beepStart() {
  delay(250);
  int notes[] = { 262, 330, 392, 523 };
  int times[] = { 150, 150, 150, 300 };
  for (int i = 0; i < 4; i++) {
    tone(pinBuzz, notes[i], times[i]);
    delay(times[i] + 50);
  }
  noTone(pinBuzz);
}

void beepGo() {
  delay(250);
  int notes[] = { 400, 600 };
  int times[] = { 180, 230 };
  for (int i = 0; i < 2; i++) {
    tone(pinBuzz, notes[i], times[i]);
    delay(times[i]);
  }
  noTone(pinBuzz);
}

void beepLose() {
  delay(250);
  int notes[] = { 880, 830, 784, 740, 698, 659, 622 };
  int times[] = { 200, 200, 200, 200, 200, 250, 400 };
  for (int i = 0; i < 7; i++) {
    tone(pinBuzz, notes[i], times[i]);
    delay(times[i] + 50);
  }
  noTone(pinBuzz);
}

void beepWin() {
  delay(250);
  int notes[] = { 523, 659, 784, 1047, 880, 988, 1047 };
  int times[] = { 150, 150, 150, 300, 150, 150, 400 };
  for (int i = 0; i < 7; i++) {
    tone(pinBuzz, notes[i], times[i]);
    delay(times[i] + 50);
  }
  noTone(pinBuzz);
}

void showStart() {
  static int lastOff = -1;
  if (millis() - timerPlay >= 4) {
    if (gameState % 2 == 0) {
      while (gameState % 2 == 0) {
        if (millis() - blinkStart >= 500) {
          blinkStart = millis();
          blinkState = !blinkState;
          lcd.setCursor(5, 0);
          lcd.print(blinkState ? "START!" : "      ");
        }
        if (millis() - timerLed >= 20) {
          timerLed = millis();
          int led = pixelIdx % leds.numPixels();
          int hueVal = hue + led * 65536L / leds.numPixels();
          int offLed = (pixelIdx + 1) % 3 == 0 ? led : -1;
          uint32_t color = leds.gamma32(leds.ColorHSV(hueVal));
          leds.setPixelColor(led, color);
          if (lastOff >= 0 && lastOff != offLed) leds.setPixelColor(lastOff, 0);
          if (offLed >= 0) {
            leds.setPixelColor(offLed, 0);
            lastOff = offLed;
          }
          leds.show();
          pixelIdx++;
          hue += 65536 / 90;
        }
        if (digitalRead(sw1) == LOW || digitalRead(sw2) == LOW) {
          lcd.clear();
          gameState++;
          timerNow = millis();
          leds.clear();
          leds.fill(leds.Color(255, 255, 255));
          leds.show();
          beepGo();
          break;
        }
      }
    }
  }
}

void updateTimer() {
  if (millis() - timerNow >= 1000) {
    elapsed++;
    timerNow = millis();
  }
}

void updateLCD() {
  if (millis() - timerLCD >= 200) {
    lcd.setCursor(0, 0);
    lcd.print("TIME: ");
    lcd.setCursor(6, 0);
    lcd.print(elapsed);
    lcd.print("s ");
    timerLCD = millis();
  }
}

void readEncoders() {
  if (millis() - timerEnc >= 5) {
    int clkNow1 = digitalRead(clk1);
    int clkNow2 = digitalRead(clk2);

    if (clkNow1 != lastClk1) {
      bool cw1 = (digitalRead(dt1) != clkNow1);
      pos1 += cw1 ? 1 : -1;
      pos1 = constrain(pos1, 75, 93);
      servo1.write(pos1);
    }

    if (clkNow2 != lastClk2) {
      bool cw2 = (digitalRead(dt2) != clkNow2);
      pos2 += cw2 ? 1 : -1;
      pos2 = constrain(pos2, 75, 93);
      servo2.write(pos2);
    }

    lastClk1 = clkNow1;
    lastClk2 = clkNow2;
    timerEnc = millis();
  }
}

void checkLose() {
  static bool shown = false;
  static int finalTime = 0;
  static int lastOff = -1;
  if (millis() - timerLose >= 100) {
    if (digitalRead(pinLose) == LOW && !shown) {
      shown = true;
      finalTime = elapsed;
      lcd.clear();
      lcd.setCursor(3, 0);
      lcd.print("GAME OVER!");
      lcd.setCursor(3, 1);
      lcd.print("TIME: ");
      lcd.print(finalTime);
      lcd.print("s");
      leds.clear();
      leds.fill(leds.Color(255, 0, 0));
      leds.show();
      beepLose();
      while (true) {
        if (millis() - timerLose >= 450) {
          timerLose = millis();
          blinkState = !blinkState;
          lcd.setCursor(3, 0);
          lcd.print(blinkState ? "GAME OVER!" : "           ");
          lcd.setCursor(3, 1);
          lcd.print(blinkState ? "TIME: " : "      ");
          lcd.print(blinkState ? String(finalTime) + "s" : "    ");
        }
        if (millis() - timerLed >= 5) {
          timerLed = millis();
          int led = pixelIdx % leds.numPixels();
          int offLed = (pixelIdx + 1) % 3 == 0 ? led : -1;
          uint32_t red[] = { leds.Color(255, 0, 0), leds.Color(255, 50, 50), leds.Color(190, 0, 0) };
          leds.setPixelColor(led, red[led % 3]);
          if (lastOff >= 0 && lastOff != offLed) leds.setPixelColor(lastOff, 0);
          if (offLed >= 0) {
            leds.setPixelColor(offLed, 0);
            lastOff = offLed;
          }
          leds.show();
          pixelIdx++;
        }
        servo1.write(100);
        servo2.write(100);
      }
    }
  }
}

void checkWin() {
  static bool shown = false;
  static int finalTime = 0;
  static int lastOff = -1;
  if (digitalRead(pinWin) == LOW && !shown) {
    shown = true;
    finalTime = elapsed;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("CONGRATULATIONS!");
    lcd.setCursor(3, 1);
    lcd.print("TIME: ");
    lcd.print(finalTime);
    lcd.print("s");
    leds.clear();
    leds.fill(leds.Color(0, 255, 0));
    leds.show();
    beepWin();
    while (true) {
      if (millis() - timerWin >= 450) {
        timerWin = millis();
        blinkState = !blinkState;
        lcd.setCursor(0, 0);
        lcd.print(blinkState ? "CONGRATULATIONS!" : "                ");
        lcd.setCursor(3, 1);
        lcd.print(blinkState ? "TIME: " : "      ");
        lcd.print(blinkState ? String(finalTime) + "s" : "    ");
      }
      if (millis() - timerLed >= 5) {
        timerLed = millis();
        int led = pixelIdx % leds.numPixels();
        int offLed = (pixelIdx + 1) % 3 == 0 ? led : -1;
        uint32_t green[] = { leds.Color(0, 255, 0), leds.Color(50, 255, 50), leds.Color(0, 180, 0) };
        leds.setPixelColor(led, green[led % 3]);
        if (lastOff >= 0 && lastOff != offLed) leds.setPixelColor(lastOff, 0);
        if (offLed >= 0) {
          leds.setPixelColor(offLed, 0);
          lastOff = offLed;
        }
        leds.show();
        pixelIdx++;
      }
      servo1.write(100);
      servo2.write(100);
    }
  }
}

void setup() {
  lcd.init();
  lcd.setBacklight(HIGH);
  lcd.setCursor(5, 0);
  lcd.print("START!");
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
#endif
  leds.begin();
  leds.setBrightness(255);
  for (int i = 0; i < LED_COUNT; i++) leds.setPixelColor(i, leds.ColorHSV(i * 65536L / LED_COUNT));
  leds.show();
  beepStart();
  pinMode(clk1, INPUT);
  pinMode(dt1, INPUT);
  pinMode(sw1, INPUT_PULLUP);
  lastClk1 = digitalRead(clk1);
  pinMode(clk2, INPUT);
  pinMode(dt2, INPUT);
  pinMode(sw2, INPUT_PULLUP);
  lastClk2 = digitalRead(clk2);
  pinMode(pinLose, INPUT_PULLUP);
  pinMode(pinWin, INPUT_PULLUP);
  servo1.attach(8);
  servo2.attach(9);
  servo1.write(90);
  servo2.write(90);
  timerNow = millis();
  timerEnc = millis();
  timerLCD = millis();
}

void loop() {
  showStart();
  updateTimer();
  updateLCD();
  readEncoders();
  checkLose();
  checkWin();
}
