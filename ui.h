/*
 * ui.h
 * Display and user interface functions with auto-scaling for M5StickC and M5StickC Plus2
 */

#ifndef UI_H
#define UI_H

// UI variables
int currentScreen = 0;

// GPIO variables
unsigned long lastPinPress[3] = {0, 0, 0}; // For G0, G26, G36
unsigned long startupTime = 0;

// External GPIO delay variable (defined in main sketch)
extern int gpioDelay;

// Auto-detection and scaling variables
bool isPlus2 = false;
float scaleFactor = 1.0;
int baseIconSize = 32;
int scaledIconSize = 32;
int baseTextSize = 1;
int scaledTextSize = 1;

// Screen layout constants (will be scaled)
struct ScreenLayout {
  int iconX, iconY;
  int textX, textY;
  int statusX, statusY;
  int dotsY, dotsSpacing, dotsStartX;
  int instructX, instructY;
  int connectionX, connectionY, connectionRadius;
};

ScreenLayout layout;

void detectDeviceAndSetScale() {
  // Detect device by screen dimensions
  int screenWidth = M5.Lcd.width();
  int screenHeight = M5.Lcd.height();
  
  Serial.print("Screen dimensions: ");
  Serial.print(screenWidth);
  Serial.print("x");
  Serial.println(screenHeight);
  
  if (screenWidth == 240 && screenHeight == 135) {
    // M5StickC Plus/Plus2
    isPlus2 = true;
    scaleFactor = 1.5;
    Serial.println("Detected: M5StickC Plus/Plus2 (240x135)");
  } else {
    // Original M5StickC or similar
    isPlus2 = false;
    scaleFactor = 1.0;
    Serial.println("Detected: M5StickC (original) (160x80)");
  }
  
  // Keep icons at original size but scale text
  scaledIconSize = baseIconSize;  // Always 32x32
  scaledTextSize = (scaleFactor >= 1.5) ? 2 : 1;
  
  // Set up layout based on device
  if (isPlus2) {
    // M5StickC Plus/Plus2 layout (240x135)
    layout.iconX = (240 - baseIconSize) / 2;  // Center 32px icon in 240px width
    layout.iconY = 30;
    layout.textX = 240 / 2;  // True center of screen width for text calculation
    layout.textY = layout.iconY + baseIconSize + 10;
    layout.statusX = 220;
    layout.statusY = 12;
    layout.connectionRadius = 7;
    layout.dotsY = 120;
    layout.dotsSpacing = 25;
    layout.dotsStartX = 45;
    layout.instructX = 8;
    layout.instructY = 8;
  } else {
    // Original M5StickC layout (160x80)
    layout.iconX = (160 - baseIconSize) / 2;  // Center 32px icon in 160px width
    layout.iconY = 20;
    layout.textX = 160 / 2;  // True center of screen width for text calculation
    layout.textY = layout.iconY + baseIconSize + 4;
    layout.statusX = 150;
    layout.statusY = 8;
    layout.connectionRadius = 5;
    layout.dotsY = 72;
    layout.dotsSpacing = 17;
    layout.dotsStartX = 30;
    layout.instructX = 5;
    layout.instructY = 5;
  }
  
  Serial.print("Scale factor: ");
  Serial.print(scaleFactor);
  Serial.print(", Icon size: ");
  Serial.print(scaledIconSize);
  Serial.print(", Text size: ");
  Serial.println(scaledTextSize);
}

void drawBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color) {
  int16_t byteWidth = (w + 7) / 8;
  uint8_t byte = 0;

  // Draw bitmap at original size (no scaling)
  for (int16_t j = 0; j < h; j++) {
    for (int16_t i = 0; i < w; i++) {
      if (i & 7) {
        byte <<= 1;
      } else {
        byte = bitmap[j * byteWidth + i / 8];
      }
      if (byte & 0x80) {
        M5.Lcd.drawPixel(x + i, y + j, color);
      }
    }
  }
}

void drawConnectionStatus() {
  // Draw connection status circle in top-right corner
  if (deviceConnected) {
    M5.Lcd.fillCircle(layout.statusX, layout.statusY, layout.connectionRadius, GREEN);
  } else if (pairingMode) {
    M5.Lcd.fillCircle(layout.statusX, layout.statusY, layout.connectionRadius, YELLOW);
  } else {
    M5.Lcd.fillCircle(layout.statusX, layout.statusY, layout.connectionRadius, RED);
  }
}

// Helper function to get text width for proper centering
int getTextWidth(String text, int textSize) {
  // Approximate character width based on text size
  int charWidth = (textSize == 1) ? 6 : 12;  // Size 1 = ~6px, Size 2 = ~12px per char
  return text.length() * charWidth;
}

void updateDisplay() {
  
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(scaledTextSize);
  
  // Draw connection status
  drawConnectionStatus();
  
  // Draw screen indicator dots at bottom
  for (int i = 0; i < NUM_SCREENS; i++) {
    int x = layout.dotsStartX + (i * layout.dotsSpacing);
    int y = layout.dotsY;
    int dotRadius = isPlus2 ? 4 : 3;
    int dotRadiusInactive = isPlus2 ? 3 : 2;
    
    if (i == currentScreen) {
      M5.Lcd.fillCircle(x, y, dotRadius, WHITE);
    } else {
      M5.Lcd.drawCircle(x, y, dotRadiusInactive, DARKGREY);
    }
  }
  
  // Draw current screen content with properly centered text
  String displayText = "";
  uint16_t iconColor = WHITE;
  
  switch (currentScreen) {
    case 0: // Connect New Camera
      drawBitmap(layout.iconX, layout.iconY, bluetooth_icon, 32, 32, ICON_BLUE);
      displayText = "CONNECT";
      iconColor = ICON_BLUE;
      break;
      
    case 1: // Shutter
      drawBitmap(layout.iconX, layout.iconY, shutter_icon, 32, 32, ICON_RED);
      displayText = "SHUTTER";
      iconColor = ICON_RED;
      break;
      
    case 2: // Switch Mode
      drawBitmap(layout.iconX, layout.iconY, switch_icon, 32, 32, ICON_ORANGE);
      displayText = "MODE";
      iconColor = ICON_ORANGE;
      break;
      
    case 3: // Screen Off
      drawBitmap(layout.iconX, layout.iconY, screen_icon, 32, 32, ICON_PINK);
      displayText = "SCREEN";
      iconColor = ICON_PINK;
      break;
      
    case 4: // Sleep
      drawBitmap(layout.iconX, layout.iconY, sleep_icon, 32, 32, ICON_PURPLE);
      displayText = "SLEEP";
      iconColor = ICON_PURPLE;
      break;
      
    case 5: // Wake
      drawBitmap(layout.iconX, layout.iconY, wake_icon, 32, 32, ICON_YELLOW);
      displayText = "WAKE";
      iconColor = ICON_YELLOW;
      break;
  }
  
  // Calculate centered text position
  int textWidth = getTextWidth(displayText, scaledTextSize);
  int centeredTextX = layout.textX - (textWidth / 2);
  
  // Draw the text centered
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setCursor(centeredTextX, layout.textY);
  M5.Lcd.print(displayText);
  
  // Show instructions hint (small text)
  M5.Lcd.setTextColor(DARKGREY);
  M5.Lcd.setTextSize(1); // Always size 1 for instructions
  M5.Lcd.setCursor(layout.instructX, layout.instructY);
  M5.Lcd.print("A:Run B:Next");

  displayCameraMode();
}

void showNotConnectedMessage() {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(scaledTextSize);
  int msgX = isPlus2 ? 40 : 30;
  int msgY = isPlus2 ? 55 : 35;
  M5.Lcd.setCursor(msgX, msgY);
  M5.Lcd.setTextColor(RED);
  M5.Lcd.println("Not Connected!");
  delay(1500);
  updateDisplay();
}

void showNoCameraMessage() {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(scaledTextSize);
  int msgX = isPlus2 ? 25 : 25;
  int msgY1 = isPlus2 ? 45 : 30;
  int msgY2 = isPlus2 ? 70 : 45;
  
  M5.Lcd.setCursor(msgX, msgY1);
  M5.Lcd.setTextColor(RED);
  M5.Lcd.println("No camera paired!");
  M5.Lcd.setCursor(msgX + (isPlus2 ? 10 : 5), msgY2);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.println("Connect first");
  delay(2000);
  updateDisplay();
}

void checkGPIOPins() {
  unsigned long currentTime = millis();
  
  // Skip GPIO checks during startup delay
  if (currentTime - startupTime < startupDelay) {
    return; // GPIO input disabled during startup
  }
  
  // One-time message when GPIO becomes active
  static bool gpioActivationMessageShown = false;
  if (!gpioActivationMessageShown) {
    Serial.println("GPIO input now active!");
    gpioActivationMessageShown = true;
  }
  
  // Check G0 (SHUTTER_PIN) - Function #2 - Trigger on LOW (to GND)
  static bool lastShutterState = HIGH; // G0 defaults to HIGH due to hardware pullup
  bool currentShutterState = digitalRead(SHUTTER_PIN);
  
  if (currentShutterState == LOW && lastShutterState == HIGH && (currentTime - lastPinPress[0] > debounceDelay)) {
    lastPinPress[0] = currentTime;
    Serial.print("GPIO Pin G0 activated (pulled to GND) - Delaying ");
    Serial.print(gpioDelay);
    Serial.println("ms then executing Shutter");
    delay(gpioDelay);  // Apply unique delay before executing
    executeShutter();
  }
  lastShutterState = currentShutterState;
  
  // Check G26 (SLEEP_PIN) - Function #5 - Trigger on HIGH (to 3.3V)
  static bool lastSleepState = LOW;
  bool currentSleepState = digitalRead(SLEEP_PIN);
  
  if (currentSleepState == HIGH && lastSleepState == LOW && (currentTime - lastPinPress[1] > debounceDelay)) {
    lastPinPress[1] = currentTime;
    Serial.print("GPIO Pin G26 activated - Delaying ");
    Serial.print(gpioDelay);
    Serial.println("ms then executing Sleep");
    delay(gpioDelay);  // Apply unique delay before executing
    executeSleep();
  }
  lastSleepState = currentSleepState;
  
  // Check G36 (WAKE_PIN) - Function #6 - Trigger on HIGH (to 3.3V)
  static bool lastWakeState = LOW;
  bool currentWakeState = digitalRead(WAKE_PIN);
  
  if (currentWakeState == HIGH && lastWakeState == LOW && (currentTime - lastPinPress[2] > debounceDelay)) {
    lastPinPress[2] = currentTime;
    Serial.print("GPIO Pin G36 activated - Delaying ");
    Serial.print(gpioDelay);
    Serial.println("ms then executing Wake");
    delay(gpioDelay);  // Apply unique delay before executing
    executeWake();
  }
  lastWakeState = currentWakeState;
}

#endif // UI_H