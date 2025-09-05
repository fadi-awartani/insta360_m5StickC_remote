/*
-----------------------------------------------------------------------------
MIT License

Copyright (c) Hillel Steinberg

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
-----------------------------------------------------------------------------

// Original code from forked repo on Aug 29, 2025 was also under the MIT license 
// as follows:

By Cameron Coward
serialhobbyism.com
8/15/25

Insta360 Remote for M5Stack devices. Tested on original M5StickC and M5StickC Plus2. Should also work on M5StickC Plus. 

Tested on Insta360 X5. Should work on other X-series models and RS.

Supports "wake" of camera.

Make sure you set REMOTE_IDENTIFIER below. Just select three alphanumeric characters of your choice to prevent interference with multiple remotes.

Make sure you have the other files in the same folder: config.h, icons.h, camera.h, ble_handlers.h, ui.h, and commands.h
-----------------------------------------------------------------------------
*/

#include <M5Unified.h>
#include "BLEDevice.h"
#include "BLEUtils.h"
#include "BLEServer.h"
#include "BLE2902.h"
#include "Preferences.h"

// *** CONFIGURE YOUR UNIQUE REMOTE IDENTIFIER HERE ***
// Change this 3-character identifier for each remote to prevent interference
// Examples: "001", "A01", "XYZ", "Bob", etc.
const char REMOTE_IDENTIFIER[4] = "A01";  // Maximum 3 characters + null terminator

// Calculate unique GPIO delay based on REMOTE_IDENTIFIER (0-100ms)
int calculateGPIODelay() {
  int hash = 0;
  for (int i = 0; i < 3 && REMOTE_IDENTIFIER[i] != '\0'; i++) {
    hash += (REMOTE_IDENTIFIER[i] * (i + 1));  // Weight by position
  }
  return hash % 101;  // Returns 0-100ms
}

// GPIO delay for this remote (calculated once at startup)
int gpioDelay = 0;

// Include all module headers in correct order
#include "config.h"
#include "icons.h"
#include "camera.h"

// Forward declarations for cross-dependencies
void updateDisplay();
void drawBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color);
void setNormalAdvertising();
void setWakeAdvertising(uint8_t* wakePayload);
void sendCommand(uint8_t* command, size_t length, const char* commandName);
void executeShutter();
void executeSleep();
void executeWake();
void executeSwitchMode();
void executeScreenOff();
void connectNewCamera();
void showNotConnectedMessage();
void showNoCameraMessage();
void checkGPIOPins();

// Now include the implementation headers
#include "ble_handlers.h"
#include "ui.h"
#include "commands.h"

void setup() {
  M5.begin();
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(1);
  detectDeviceAndSetScale();
  
  Serial.begin(115200);
  Serial.println("M5StickC Insta360 Camera Remote");
  Serial.print("Remote ID: ");
  Serial.println(REMOTE_IDENTIFIER);
  
  // Calculate unique GPIO delay for this remote
  gpioDelay = calculateGPIODelay();
  Serial.print("GPIO delay for this remote: ");
  Serial.print(gpioDelay);
  Serial.println("ms");
  
  // Record startup time for GPIO delay
  startupTime = millis();
  
  // Setup GPIO pins - G0 uses hardware pullup, others use pulldown
  pinMode(SHUTTER_PIN, INPUT);           // G0 has hardware pullup - trigger on LOW (to GND)
  pinMode(SLEEP_PIN, INPUT_PULLDOWN);    // G26 for Sleep (#5) - trigger on HIGH (to 3.3V)
  pinMode(WAKE_PIN, INPUT_PULLDOWN);     // G25 for Wake (#6) - trigger on HIGH (to 3.3V)
  
  Serial.println("GPIO pins configured:");
  Serial.println("G0 (Shutter) - INPUT (hardware pullup, trigger on GND)");
  Serial.println("G26 (Sleep) - INPUT_PULLDOWN (trigger on 3.3V)");
  Serial.println("G25 (Wake) - INPUT_PULLDOWN (trigger on 3.3V)");
  Serial.println("GPIO input disabled for 2 seconds after startup...");
  
  // Load saved camera
  loadCurrentCamera();
  
  // Initialize BLE
  String deviceName = "Insta360 GPS Remote " + String(REMOTE_IDENTIFIER);
  BLEDevice::init(deviceName.c_str());

  // Create BLE Scanner for camera detection
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyScanCallbacks());
  pBLEScan->setActiveScan(true); // Active scan uses more power but gets names
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  pService = pServer->createService(GPS_REMOTE_SERVICE_UUID);

  // Create characteristics
  pWriteCharacteristic = pService->createCharacteristic(
                      GPS_REMOTE_WRITE_CHAR_UUID,
                      BLECharacteristic::PROPERTY_WRITE
                    );
  pWriteCharacteristic->setCallbacks(new MyCharacteristicCallbacks());

  pNotifyCharacteristic = pService->createCharacteristic(
                      GPS_REMOTE_NOTIFY_CHAR_UUID,
                      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
                    );
  pDescriptor2902 = new BLE2902();
  pDescriptor2902->setNotifications(true);
  pDescriptor2902->setIndications(true);
  pNotifyCharacteristic->addDescriptor(pDescriptor2902);

  // Start the service
  pService->start();

  // Start with normal advertising
  setNormalAdvertising();
  
  Serial.println("Ready!");
  updateDisplay();
}

void loop() {

  M5.update();

  bool connected = deviceConnected && pServer && (pServer->getConnectedCount() > 0);
  
  // Check GPIO pins for external button presses
  checkGPIOPins();
  
  // Handle connection changes
  if (!connected && oldDeviceConnected) {
    delay(500);
    if (!wakeMode && !pairingMode) {
      pServer->startAdvertising();
    }
    oldDeviceConnected = false;
  }
  
  if (connected && !oldDeviceConnected) {
    oldDeviceConnected = true;
  }

  // Button B - Navigate to next screen
  if (M5.BtnB.wasReleased()) {
    currentScreen = (currentScreen + 1) % NUM_SCREENS;
    Serial.print("Switched to screen: ");
    Serial.println(currentScreen);
    updateDisplay();
  }

  // Button A - Execute current screen's function
  if (M5.BtnA.wasReleased()) {
    Serial.print("Executing function for screen: ");
    Serial.println(currentScreen);
    
    switch (currentScreen) {
      case SCREEN_CONNECT_CAMERA: // Connect New Camera
        connectNewCamera();
        break;
        
      case SCREEN_SHUTTER: // Shutter
        if (!deviceConnected) {
          showNotConnectedMessage();
        } else {
          executeShutter();
        }
        break;
        
      case SCREEN_SWITCH_MODE: // Switch Mode
        if (!deviceConnected) {
          showNotConnectedMessage();
        } else {
          executeSwitchMode();
        }
        break;
        
      case SCREEN_CAMERA_SCREEN_OFF: // Screen Off
        if (!deviceConnected) {
          showNotConnectedMessage();
        } else {
          executeScreenOff();
        }
        break;
        
      case SCREEN_CAMERA_SLEEP: // Sleep
        if (!deviceConnected) {
          showNotConnectedMessage();
        } else {
          executeSleep();
        }
        break;
        
      case SCREEN_CAMERA_WAKE: // Wake
        if (!currentCamera.isValid) {
          showNoCameraMessage();
        } else {
          executeWake();
        }
        break;

      default:
        Serial.println("Error: Wrong screen mode");
        break;
    }
    
    delay(200);
  }
  
  delay(50);
}
