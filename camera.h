/*
 * camera.h
 * Camera structure and management functions
 */

#ifndef CAMERA_H
#define CAMERA_H

// Camera info structure
struct CameraInfo {
  char name[30];
  char address[20];
  uint8_t wakePayload[6];
  bool isValid;
};

// Global camera variables
CameraInfo currentCamera;
Preferences preferences;

// Pairing mode variables
bool pairingMode = false;
String detectedCameraName = "";
String detectedCameraAddress = "";
String connectedDeviceAddress = "";

// Wake-up variables
bool wakeMode = false;
uint8_t currentWakePayload[6] = {0};

void loadCurrentCamera() {
  
  Serial.println("Loading camera from preferences...");
  
  preferences.begin("camera", false);
  
  preferences.getString("name", currentCamera.name, 30);
  preferences.getString("address", currentCamera.address, 20);
  
  Serial.print("Loaded name from preferences: ");
  Serial.println(currentCamera.name);
  Serial.print("Loaded address from preferences: ");
  Serial.println(currentCamera.address);
  
  size_t len = preferences.getBytesLength("wake");
  Serial.print("Wake payload length: ");
  Serial.println(len);
  
  if (len == 6) {
    preferences.getBytes("wake", currentCamera.wakePayload, 6);
    currentCamera.isValid = (strlen(currentCamera.name) > 0);
    
    Serial.print("Wake payload loaded: ");
    for (int i = 0; i < 6; i++) {
      Serial.printf("%02X ", currentCamera.wakePayload[i]);
    }
    Serial.println();
  } else {
    currentCamera.isValid = false;
    Serial.println("No valid wake payload found");
  }
  
  preferences.end();
  
  Serial.print("Camera isValid: ");
  Serial.println(currentCamera.isValid);
  
  if (currentCamera.isValid) {
    Serial.print("Final loaded camera: ");
    Serial.println(currentCamera.name);
  } else {
    Serial.println("No valid camera saved");
  }
}

void saveCurrentCamera(String cameraName, String cameraAddress) {
  Serial.print("Saving camera: ");
  Serial.print(cameraName);
  Serial.print(" @ ");
  Serial.println(cameraAddress);
  
  // Extract wake payload from camera name (last 6 characters)
  if (cameraName.length() >= 6) {
    String nameEnd = cameraName.substring(cameraName.length() - 6);
    Serial.print("Wake payload suffix: ");
    Serial.println(nameEnd);
    
    // Convert to ASCII bytes
    for (int i = 0; i < 6; i++) {
      currentCamera.wakePayload[i] = (uint8_t)nameEnd[i];
    }
    
    // Save camera info
    snprintf(currentCamera.name, 30, "%s", cameraName.c_str());
    snprintf(currentCamera.address, 20, "%s", cameraAddress.c_str());
    currentCamera.isValid = true;
    
    // Store in preferences
    preferences.begin("camera", false);
    preferences.putString("name", currentCamera.name);
    preferences.putString("address", currentCamera.address);
    preferences.putBytes("wake", currentCamera.wakePayload, 6);
    preferences.end();
    
    Serial.print("Wake payload bytes: ");
    for (int i = 0; i < 6; i++) {
      Serial.printf("%02X ", currentCamera.wakePayload[i]);
    }
    Serial.println();
    Serial.println("Camera saved successfully");
  } else {
    Serial.println("Camera name too short for valid wake payload");
    currentCamera.isValid = false;
  }
}

#endif // CAMERA_H