/*
 * commands.h
 * Camera command execution functions
 */

#ifndef COMMANDS_H
#define COMMANDS_H

// Forward declarations needed
void executeShutter();
void executeSleep();
void executeWake();

void connectNewCamera() {

  // Clear existing camera data
  Serial.println("Starting new camera pairing process");
  
  memset(&currentCamera, 0, sizeof(currentCamera));
  currentCamera.isValid = false;
  
  preferences.begin("camera", false);
  preferences.clear();
  preferences.end();
  
  // Reset detection variables
  detectedCameraName = "";
  detectedCameraAddress = "";
  
  M5.Lcd.fillScreen(BLACK);
  // Draw pairing icon
  drawBitmap(64, 15, pairing_icon, 32, 32, ICON_CYAN);
  M5.Lcd.setCursor(35, 50);
  M5.Lcd.setTextColor(YELLOW);
  M5.Lcd.println("PAIRING...");
  M5.Lcd.setCursor(25, 65);
  M5.Lcd.setTextColor(CYAN);
  M5.Lcd.setTextSize(1);
  M5.Lcd.println("B:Cancel");
  delay(2000);
  
  // Start scanning for cameras
  pairingMode = true;
  Serial.println("Starting scan for Insta360 cameras");
  
  M5.Lcd.fillScreen(BLACK);
  // Draw pairing icon again
  drawBitmap(64, 10, pairing_icon, 32, 32, ICON_CYAN);
  M5.Lcd.setCursor(35, 45);
  M5.Lcd.setTextColor(YELLOW);
  M5.Lcd.println("Scanning...");
  M5.Lcd.setCursor(40, 65);
  M5.Lcd.setTextColor(CYAN);
  M5.Lcd.println("B:Cancel");
  
  // Start continuous scanning
  if (pBLEScan) {
    pBLEScan->start(0, nullptr, false); // Continuous scan
  }
  
  // Ensure advertising is on
  setNormalAdvertising();
  
  // Wait for camera to be detected and connect
  unsigned long startTime = millis();
  while (pairingMode && millis() - startTime < 30000) { // 30 second timeout
    M5.update();
    
    if (M5.BtnB.wasReleased()) {
      Serial.println("Pairing cancelled by user");
      pairingMode = false;
      if (pBLEScan) {
        pBLEScan->stop();
      }
      updateDisplay();
      return;
    }
    
    // Update display with detected camera if found
    static String lastDetected = "";
    if (detectedCameraName.length() > 0 && detectedCameraName != lastDetected) {
      lastDetected = detectedCameraName;
      M5.Lcd.fillRect(15, 45, 130, 15, BLACK);
      M5.Lcd.setCursor(35, 45);
      M5.Lcd.setTextColor(GREEN);
      M5.Lcd.print("Found!");
    }
    
    delay(100);
  }
  
  // Timeout - stop scanning
  if (pairingMode) {
    pairingMode = false;
    if (pBLEScan) {
      pBLEScan->stop();
    }
    
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(40, 30);
    M5.Lcd.setTextColor(YELLOW);
    M5.Lcd.println("Timeout");
    M5.Lcd.setCursor(35, 45);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.println("Try again");
    delay(2000);
  }
  
  updateDisplay();
}

void executeShutter() {
  sendCommand(SHUTTER_CMD, sizeof(SHUTTER_CMD), "SHUTTER");
}

void executeSwitchMode() {
  sendCommand(MODE_CMD, sizeof(MODE_CMD), "MODE");
  mode_str = "Unknown";
  updateDisplay();
}

void executeScreenOff() {
  sendCommand(TOGGLE_SCREEN_CMD, sizeof(TOGGLE_SCREEN_CMD), "SCREEN");
}

void executeSleep() {
  sendCommand(POWER_OFF_CMD, sizeof(POWER_OFF_CMD), "SLEEP");
}

void executeWake() {

  if (!currentCamera.isValid) {

    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(25, 30);
    M5.Lcd.setTextColor(RED);
    M5.Lcd.println("No camera saved!");
    M5.Lcd.setCursor(25, 45);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.println("Connect first");
    delay(2000);

    updateDisplay();

    return;
  }
  
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(35, 30);
  M5.Lcd.setTextColor(YELLOW);
  M5.Lcd.println("Waking...");
  M5.Lcd.setCursor(20, 45);
  M5.Lcd.setTextColor(WHITE);
  if (strlen(currentCamera.name) > 12) {
    // Show abbreviated name
    String shortName = String(currentCamera.name).substring(0, 12);
    M5.Lcd.println(shortName);
  } else {
    M5.Lcd.println(currentCamera.name);
  }
  
  setWakeAdvertising(currentCamera.wakePayload);
  delay(3000); // Send wake signal for 3 seconds
  setNormalAdvertising();
  
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(35, 35);
  M5.Lcd.setTextColor(GREEN);
  M5.Lcd.println("Wake sent!");
  delay(1000);
  updateDisplay();
}

#endif // COMMANDS_H