/*
 * ble_handlers.h
 * BLE callbacks, advertising, and communication functions
 */

#ifndef BLE_HANDLERS_H
#define BLE_HANDLERS_H

// No camera mode yet
String mode_str = "Unknown";

// BLE variables
BLEServer* pServer = nullptr;
BLEService* pService = nullptr;
BLECharacteristic* pWriteCharacteristic = nullptr;
BLECharacteristic* pNotifyCharacteristic = nullptr;
BLEScan* pBLEScan = nullptr;
bool deviceConnected = false;
bool oldDeviceConnected = false;

// Prefix before unique mode signatures
const uint8_t MODE_STATUS_PREFIX[] = {
  0xFE, 0xEF, 0xFE, 0x10, 0x80, 0x09, 0x01
};

// Recognize various modes by Camera responses
const uint8_t MODE_CAMERA[] = {
  0x20, 0x39, 0x39, 0x39, 0x2B
};

const uint8_t MODE_VIDEO[] = {
  0x35, 0x68, 0x33, 0x35, 0x6D
};

const uint8_t MODE_TIMESHIFT[] = {
  0x35, 0x68, 0x30, 0x33, 0x6D
};

const uint8_t MODE_LOOP_RECORDING[] = {
  0x39, 0x68, 0x34, 0x32, 0x6D
};

const uint8_t HEARTBEAT[] = {
  0xFE, 0xEF, 0xFE, 0x02, 0x80, 0x05, 0x01, 0x54 
};

void displayCameraMode(void) {

    // If a mode was detected, show it
    if (mode_str.length() > 0) {

        String mode_line = "Mode: " + mode_str;

        M5.Lcd.setTextSize(2);
        M5.Lcd.fillRect(0, 90, 240, 25, BLACK);
        M5.Lcd.setCursor(10, 95);
        M5.Lcd.setTextColor(YELLOW);
        M5.Lcd.print(mode_line);

        // Reset text size
        M5.Lcd.setTextSize(1);
    }
}

// BLE Scan callback to capture camera info during pairing mode
class MyScanCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {

      if (!pairingMode) 
        return; // Only process during pairing mode
      
      // Look for Insta360 cameras
      if (advertisedDevice.haveName()) {
        String deviceName = advertisedDevice.getName().c_str();
        String deviceAddress = advertisedDevice.getAddress().toString().c_str();
        
        Serial.print("Scan found: ");
        Serial.print(deviceName);
        Serial.print(" @ ");
        Serial.println(deviceAddress);
        
        // Check if this is an Insta360 camera
        if (deviceName.startsWith("X3 ") || 
            deviceName.startsWith("X4 ") || 
            deviceName.startsWith("X5 ") ||
            deviceName.startsWith("RS ") ||
            deviceName.startsWith("ONE ")) {
          
          // Found an Insta360 camera - save its info
          detectedCameraName = deviceName;
          detectedCameraAddress = deviceAddress;
          
          Serial.print("Found Insta360 camera: ");
          Serial.println(detectedCameraName);
        }
      }
    }
};

class MyServerCallbacks: public BLEServerCallbacks {

    void onConnect(BLEServer* pServer, esp_ble_gatts_cb_param_t *param) {

      deviceConnected = true;
      
      // Get the connected device's address
      char addressStr[18];
      sprintf(addressStr, "%02x:%02x:%02x:%02x:%02x:%02x",
              param->connect.remote_bda[0],
              param->connect.remote_bda[1],
              param->connect.remote_bda[2],
              param->connect.remote_bda[3],
              param->connect.remote_bda[4],
              param->connect.remote_bda[5]);

      connectedDeviceAddress = String(addressStr);
      
      Serial.print("Device connected from address: ");
      Serial.println(connectedDeviceAddress);
      
      // Check if we're in pairing mode and have detected a camera
      if (pairingMode && detectedCameraName.length() > 0) {

        // Stop scanning
        if (pBLEScan) {
          pBLEScan->stop();
        }

        pairingMode = false;
        
        Serial.print("Pairing with detected camera: ");
        Serial.println(detectedCameraName);
        
        // Validate camera name format
        bool validFormat = false;
        if (detectedCameraName.length() >= 9) { // "X5 " + 6 chars minimum
          int spaceIndex = detectedCameraName.indexOf(' ');
          if (spaceIndex > 0 && detectedCameraName.length() - spaceIndex > 6) {
            validFormat = true;
          }
        }
        
        if (validFormat) {

          M5.Lcd.fillScreen(BLACK);
          M5.Lcd.setCursor(10, 10);
          M5.Lcd.setTextColor(GREEN);
          M5.Lcd.println("CAMERA PAIRED!");
          M5.Lcd.setCursor(10, 35);
          M5.Lcd.setTextColor(WHITE);
          M5.Lcd.println("Camera:");
          M5.Lcd.setCursor(10, 45);
          M5.Lcd.setTextColor(YELLOW);
          M5.Lcd.println(detectedCameraName);
          mode_str = "Unknown";
          delay(400);
          
          saveCurrentCamera(detectedCameraName, detectedCameraAddress);
          
          M5.Lcd.fillScreen(BLACK);
          M5.Lcd.setCursor(10, 20);
          M5.Lcd.setTextColor(GREEN);
          M5.Lcd.println("Camera Saved!");
          M5.Lcd.setCursor(10, 40);
          M5.Lcd.setTextColor(YELLOW);
          M5.Lcd.println(currentCamera.name);
          delay(500);
        } else {
          // Invalid format
          M5.Lcd.fillScreen(BLACK);
          M5.Lcd.setCursor(10, 10);
          M5.Lcd.setTextColor(RED);
          M5.Lcd.println("ERROR:");
          M5.Lcd.setCursor(10, 30);
          M5.Lcd.setTextColor(WHITE);
          M5.Lcd.println("Invalid camera");
          mode_str = "Unknown";
          delay(3000);
          
          // Disconnect
          pServer->disconnect(pServer->getConnId());
        }
      } else if (pairingMode) {

        // In pairing mode but no camera detected yet
        M5.Lcd.fillScreen(BLACK);
        M5.Lcd.setCursor(10, 10);
        M5.Lcd.setTextColor(YELLOW);
        M5.Lcd.println("Camera connected");
        M5.Lcd.setCursor(10, 25);
        M5.Lcd.setTextColor(WHITE);
        M5.Lcd.setTextSize(1);
        M5.Lcd.println("Not identified.");
        M5.Lcd.setCursor(10, 40);
        M5.Lcd.println("Please retry.");
        mode_str = "Unknown";
        delay(3000);
        
        // Stop pairing mode
        pairingMode = false;
        if (pBLEScan) {
          pBLEScan->stop();
        }
        
        // Disconnect
        pServer->disconnect(pServer->getConnId());
      } else if (currentCamera.isValid) {

        // Known camera reconnected - just update display, no popup
        Serial.print("Known camera reconnected: ");
        Serial.println(currentCamera.name);
        mode_str = "Unknown";
        updateDisplay();
      } else {
        // Not in pairing mode and no known camera
        M5.Lcd.fillScreen(BLACK);
        M5.Lcd.setCursor(10, 20);
        M5.Lcd.setTextColor(YELLOW);
        M5.Lcd.println("Unknown camera");
        M5.Lcd.setCursor(10, 40);
        M5.Lcd.setTextColor(WHITE);
        M5.Lcd.println("Use Connect to pair");
        mode_str = "Unknown";
        delay(3000);
        
        // Disconnect
        pServer->disconnect(pServer->getConnId());
      }
      
      updateDisplay();
    }

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      connectedDeviceAddress = "";
      
      Serial.println("Camera disconnected");
      mode_str = "Unknown";
      updateDisplay();
      
      // Return to normal advertising
      setNormalAdvertising();
    }
};

class MyCharacteristicCallbacks: public BLECharacteristicCallbacks {

    void onWrite(BLECharacteristic* pCharacteristic) {

      String rxValue = pCharacteristic->getValue().c_str();
      
      bool is_heartbeat = false;

      if (rxValue.length() > 0) {

        const uint8_t *data = (const uint8_t*)rxValue.c_str();

        if (rxValue.length() == 8 && memcmp(data, HEARTBEAT, 8) == 0) {
          Serial.printf("Heartbeat");
          Serial.println();
          is_heartbeat = true;
        }
        else if (rxValue.length() == 15 && memcmp(data, MODE_STATUS_PREFIX, 7) == 0) {

          const uint8_t *cmd = data + 10;

          // Camera mode?
          if (memcmp(cmd, MODE_CAMERA, 5) == 0) {
            mode_str = "Camera";
          }
          else if (memcmp(cmd, MODE_VIDEO, 5) == 0) {
            // Seems to apply to several video modes (e.g., PureVideo)
            mode_str = "Video";
          }
          else if (memcmp(cmd, MODE_TIMESHIFT, 5) == 0) {
            // Timeshift
            mode_str = "Timeshift";
          }
          else if (memcmp(cmd, MODE_LOOP_RECORDING, 5) == 0) {
            // Loop Recording
            mode_str = "Loop Record";
          }
          else {
            Serial.printf("MODE unhandled returned: ");
            for (int i = 10; i < rxValue.length(); i++) {
              Serial.printf("%02X ", (uint8_t)rxValue[i]);
            }
            Serial.println();
            mode_str = "Unhandled";
          }

          // We may know the mode now, if so, show it
          displayCameraMode();
        }

        if (!is_heartbeat)
        {
            Serial.print("RX: ");
            for (int i = 0; i < rxValue.length(); i++) {
              Serial.printf("%02X ", (uint8_t)rxValue[i]);
            }
            Serial.println();
        }
      }
    }
};

void setWakeAdvertising(uint8_t* wakePayload) {

  Serial.print("Setting wake advertising with payload: ");
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X ", wakePayload[i]);
  }
  Serial.println();
  
  // Stop current advertising
  BLEDevice::stopAdvertising();
  delay(100);
  
  // Create manufacturer data for wake-up (iBeacon format)
  uint8_t manufacturerData[26];
  
  // Apple company ID and iBeacon format
  manufacturerData[0] = 0x4c;  // Apple company ID (low byte)
  manufacturerData[1] = 0x00;  // Apple company ID (high byte)
  manufacturerData[2] = 0x02;  // iBeacon format identifier
  manufacturerData[3] = 0x15;  // iBeacon format identifier
  
  // iBeacon UUID (16 bytes) - specific pattern for Insta360 wake
  manufacturerData[4] = 0x09;
  manufacturerData[5] = 0x4f;
  manufacturerData[6] = 0x52;
  manufacturerData[7] = 0x42;
  manufacturerData[8] = 0x49;
  manufacturerData[9] = 0x54;
  manufacturerData[10] = 0x09;
  manufacturerData[11] = 0xff;
  manufacturerData[12] = 0x0f;
  manufacturerData[13] = 0x00;
  
  // Camera-specific wake payload (6 bytes)
  memcpy(&manufacturerData[14], wakePayload, 6);
  
  // Additional iBeacon data
  manufacturerData[20] = 0x00;  // Major (high byte)
  manufacturerData[21] = 0x00;  // Major (low byte)
  manufacturerData[22] = 0x00;  // Minor (high byte)
  manufacturerData[23] = 0x00;  // Minor (low byte)
  manufacturerData[24] = 0xe4;  // TX Power
  manufacturerData[25] = 0x01;  // Additional byte
  
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(GPS_REMOTE_SERVICE_UUID);
  
  // Convert manufacturer data to Arduino String
  String mfgDataString = "";
  for (int i = 0; i < 26; i++) {
    mfgDataString += (char)manufacturerData[i];
  }
  
  // Set manufacturer data using BLEAdvertisementData
  BLEAdvertisementData adData;
  adData.setManufacturerData(mfgDataString);
  
  // Create unique device name with identifier
  String deviceName = "Insta360 GPS Remote " + String(REMOTE_IDENTIFIER);
  adData.setName(deviceName);
  
  pAdvertising->setAdvertisementData(adData);
  
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);
  
  wakeMode = true;
  memcpy(currentWakePayload, wakePayload, 6);
  
  pAdvertising->start();
  Serial.println("Wake advertising started");
}

void setNormalAdvertising() {

  Serial.println("Setting normal advertising");
  
  // Stop current advertising
  BLEDevice::stopAdvertising();
  delay(100);
  
  // Create fresh advertising without manufacturer data
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  
  // Clear any previous advertisement data
  BLEAdvertisementData adData;
  
  // Create unique device name with identifier
  String deviceName = "Insta360 GPS Remote " + String(REMOTE_IDENTIFIER);
  adData.setName(deviceName);
  adData.setCompleteServices(BLEUUID(GPS_REMOTE_SERVICE_UUID));
  
  // Set the clean advertisement data (no manufacturer data)
  pAdvertising->setAdvertisementData(adData);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);
  
  wakeMode = false;
  memset(currentWakePayload, 0, 6);
  
  pAdvertising->start();
  Serial.print("Normal advertising started with name: ");
  Serial.println(deviceName);
}

void sendCommand(uint8_t* command, size_t length, const char* commandName) {

  if (!deviceConnected || !pServer || pServer->getConnectedCount() == 0) {
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(40, 35);
    M5.Lcd.setTextColor(RED);
    M5.Lcd.println("Not Connected!");
    delay(1500);
    updateDisplay();
    return;
  }

  Serial.print("TX ");
  Serial.print(commandName);
  Serial.print(": ");
  for (int i = 0; i < length; i++) {
    Serial.printf("%02X ", command[i]);
  }
  Serial.println();

  pNotifyCharacteristic->setValue(command, length);
  pNotifyCharacteristic->notify();
  
  // Brief visual feedback
  M5.Lcd.fillRect(50, 30, 60, 20, GREEN);
  M5.Lcd.setCursor(55, 35);
  M5.Lcd.setTextColor(BLACK);
  M5.Lcd.print("SENT!");
  delay(400);

  updateDisplay();
}

#endif // BLE_HANDLERS_H