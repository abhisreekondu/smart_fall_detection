// --- WiFi and MPU6050-based Fall Detection ---  
#include <WiFi.h>  // Include WiFi library for ESP32  
#include <Wire.h>  
#include <MPU6050.h>  
#include <math.h>  

MPU6050 mpu;

// --- WiFi Credentials ---  
char ssid[] = "Abhii";  
char pass[] = "abhisree";  

// --- Fall Detection Parameters ---  
const float HIGH_IMPACT_THRESHOLD = 1.7 * 9.81;  
const float SOFT_IMPACT_THRESHOLD = 1.2 * 9.81;  
const float LOW_ACCEL_THRESHOLD = 0.25 * 9.81;  
const unsigned long FALL_TIME_WINDOW = 500;  
const unsigned long INACTIVITY_TIME = 800;  

const float ROTATION_THRESHOLD = 80.0;  
const float SLEEP_ROTATION_THRESHOLD = 10.0;  

enum FallState {  
  NORMAL,  
  POSSIBLE_FALL,  
  IMPACT_DETECTED,  
  SITTING_DOWN,  
  FALL_CONFIRMED  
};

FallState currentFallState = NORMAL;  
unsigned long fallStartTime = 0;  
unsigned long impactTime = 0;  
unsigned long inactivityStartTime = 0;  

void setup() {  
  Serial.begin(115200);  
  Wire.begin();  

  Serial.println("Initializing MPU6050...");  
  mpu.initialize();  

  if (mpu.testConnection()) {  
    Serial.println("MPU6050 connection successful");  
  } else {  
    Serial.println("MPU6050 connection failed");  
    while (1);  
  }  

  mpu.setFullScaleAccelRange(MPU6050_ACCEL_FS_2);  
  mpu.setFullScaleGyroRange(MPU6050_GYRO_FS_250);  
}  

void loop() {  
  // Check MPU6050 Connection  
  if (!mpu.testConnection()) {  
    Serial.println("MPU6050 Disconnected! Trying to Reinitialize...");  
    mpu.initialize();  
    delay(1000);  
  }  

  int16_t ax, ay, az, gx, gy, gz;  
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);  

  float accel_x = ax * 9.81 / 16384.0;  
  float accel_y = ay * 9.81 / 16384.0;  
  float accel_z = az * 9.81 / 16384.0;  

  float gyro_x = gx / 131.0;  
  float gyro_y = gy / 131.0;  
  float gyro_z = gz / 131.0;  

  float totalAcceleration = sqrt(pow(accel_x, 2) + pow(accel_y, 2) + pow(accel_z, 2));  
  float rotationRate = sqrt(pow(gyro_x, 2) + pow(gyro_y, 2) + pow(gyro_z, 2));  

  switch (currentFallState) {  
    case NORMAL:  
      if (totalAcceleration < LOW_ACCEL_THRESHOLD) {  
        currentFallState = POSSIBLE_FALL;  
        fallStartTime = millis();  
        Serial.println("Possible Free-Fall Detected...");  
      }  
      break;  

    case POSSIBLE_FALL:  
      if (totalAcceleration > HIGH_IMPACT_THRESHOLD && millis() - fallStartTime < FALL_TIME_WINDOW) {  
        Serial.println("Hard Impact Detected! Possible Fall.");  
        impactTime = millis();  
        currentFallState = IMPACT_DETECTED;  
        inactivityStartTime = millis();  
      } else if (totalAcceleration > SOFT_IMPACT_THRESHOLD) {  
        Serial.println("Soft Impact Detected. Likely Sitting.");  
        currentFallState = SITTING_DOWN;  
      } else if (millis() - fallStartTime > FALL_TIME_WINDOW) {  
        Serial.println("No Impact Detected. Resetting to NORMAL.");  
        currentFallState = NORMAL;  
      }  
      break;  

    case IMPACT_DETECTED:  
      if (millis() - inactivityStartTime > INACTIVITY_TIME) {  
        if (rotationRate < SLEEP_ROTATION_THRESHOLD) {  
          Serial.println("Fall Confirmed! No movement detected.");  
          currentFallState = FALL_CONFIRMED;  
        } else {  
          Serial.println("Movement detected. Resetting.");  
          currentFallState = NORMAL;  
        }  
      }  
      break;  

    case SITTING_DOWN:  
      if (rotationRate > SLEEP_ROTATION_THRESHOLD) {  
        Serial.println("User is still moving. Resetting to NORMAL.");  
        currentFallState = NORMAL;  
      } else if (millis() - impactTime > 2000) {  
        Serial.println("Confirmed as sitting, no fall detected.");  
        currentFallState = NORMAL;  
      }  
      break;  

    case FALL_CONFIRMED:  
      currentFallState = NORMAL;  
      break;  
  }  

  Serial.print(accel_x);  
  Serial.print(",");  
  Serial.print(accel_y);  
  Serial.print(",");  
  Serial.print(accel_z);  
  Serial.print(",");  
  Serial.println(totalAcceleration);  

  delay(50);  
}  
