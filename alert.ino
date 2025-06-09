// --- Blynk Credentials (Ensure they are defined before including Blynk library) ---

#define BLYNK_AUTH_TOKEN "lDwFYPCE2ezonCoxW8-niiYjFy6P_JrW"
#define BLYNK_TEMPLATE_ID "TMPL3VkoW2p8D"
#define BLYNK_TEMPLATE_NAME "iot based fall detection"
#include <Wire.h>
#include <MPU6050.h>
#include <math.h> // For sqrt() and pow()
#include <BlynkSimpleEsp32.h> // Include Blynk Library for ESP32
#include <WiFi.h> // Needed for WiFi.status() and IP display


MPU6050 mpu;

// --- WiFi Credentials ---
char ssid[] = "Abhii";
char pass[] = "abhisree";

// --- Fall Detection Parameters ---
const float HIGH_IMPACT_THRESHOLD = 1.5 * 9.81;  // 1.5G for hard impact (real fall)
const float SOFT_IMPACT_THRESHOLD = 1.02 * 9.81;  // 1.02G for sitting down (softer impact)
const float LOW_ACCEL_THRESHOLD = 0.25 * 9.81;    // 0.25G (~2.94 m/s²) for free-fall detection
const unsigned long FALL_TIME_WINDOW = 200;      // 200ms to confirm fall
const unsigned long INACTIVITY_TIME = 100;      // 100ms inactivity to confirm no movement

// --- Gyroscope Thresholds ---
const float ROTATION_THRESHOLD = 80.0;   // Sudden rotation detection (fall rotation)
const float SLEEP_ROTATION_THRESHOLD = 10.0;  // Slow movement when lying down

enum FallState {
  NORMAL,
  POSSIBLE_FALL,
  IMPACT_DETECTED,
  SITTING_DOWN,
  FALL_CONFIRMED
};1

FallState currentFallState = NORMAL;
unsigned long fallStartTime = 0;
unsigned long impactTime = 0;
unsigned long inactivityStartTime = 0;

void setup() {
  Serial.begin(115200);
  Wire.begin();
  mpu.initialize();

  Serial.println("Initializing MPU6050 for Fall Detection...");
  if (mpu.testConnection()) {
    Serial.println("MPU6050 connection successful");
  } else {
    Serial.println("MPU6050 connection failed");
    while (1);
  }

  // Set accelerometer and gyroscope ranges
  mpu.setFullScaleAccelRange(MPU6050_ACCEL_FS_2);
  mpu.setFullScaleGyroRange(MPU6050_GYRO_FS_250);
  // Wait until Wi-Fi is connected
 WiFi.begin(ssid, pass);

Serial.print("Connecting to Wi-Fi");
while (WiFi.status() != WL_CONNECTED) {
  delay(500);
  Serial.print(".");
}
Serial.println("\nWi-Fi connected");
Serial.println(WiFi.localIP());
  // Initialize Blynk and Wi-Fi
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
}


 


void loop() {
    
  Blynk.run(); // Keep Blynk connected

  int16_t ax, ay, az, gx, gy, gz;
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  // Convert raw accelerometer readings to m/s²
  float accel_x = ax * 9.81 / 16384.0;
  float accel_y = ay * 9.81 / 16384.0;
  float accel_z = az * 9.81 / 16384.0;

  // Convert raw gyroscope readings to deg/sec
  float gyro_x = gx / 131.0;  
  float gyro_y = gy / 131.0;
  float gyro_z = gz / 131.0;

  // Compute total acceleration magnitude
  float totalAcceleration = sqrt(pow(accel_x, 2) + pow(accel_y, 2) + pow(accel_z, 2));

  // Compute total rotation rate (angular velocity)
  float rotationRate = sqrt(pow(gyro_x, 2) + pow(gyro_y, 2) + pow(gyro_z, 2));

  // Fall Detection Logic (State Machine)
  switch (currentFallState) {
    case NORMAL:
      if (totalAcceleration < LOW_ACCEL_THRESHOLD) {  // Check for free-fall first!  // 0.25G (~2.94 m/s²) for free-fall detection
        currentFallState = POSSIBLE_FALL;
        fallStartTime = millis();
        Serial.println("Possible Free-Fall Detected...");
      }
      break;

    case POSSIBLE_FALL:
      if (totalAcceleration > HIGH_IMPACT_THRESHOLD && millis() - fallStartTime < FALL_TIME_WINDOW) { // 1.2G for hard impact (real fall)
        Serial.println("Hard Impact Detected! Possible Fall.");
        impactTime = millis();
        currentFallState = IMPACT_DETECTED;
        inactivityStartTime = millis();
      } 
      else if (totalAcceleration > SOFT_IMPACT_THRESHOLD && totalAcceleration < HIGH_IMPACT_THRESHOLD) {
        Serial.println("Soft Impact Detected. Likely Sitting.");
        currentFallState = SITTING_DOWN;
      }
      else if (millis() - fallStartTime > FALL_TIME_WINDOW) {
        Serial.println("No Impact Detected. Resetting to NORMAL.");
        currentFallState = NORMAL;
      }
      break;

    case IMPACT_DETECTED:
      if (millis() - inactivityStartTime > INACTIVITY_TIME) {
        // if (rotationRate < SLEEP_ROTATION_THRESHOLD) {
          Serial.println("Fall Confirmed! No movement detected.");
          Blynk.logEvent("fall_alert", "PLEASE HELP immediately.");
          currentFallState = FALL_CONFIRMED;
        // } else {
        //   Serial.println("Movement detected. Resetting.");
        //   currentFallState = NORMAL;
        // }
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

  // Debugging Output
  Serial.print("State: ");
  if (currentFallState == NORMAL) Serial.print("NORMAL");
  if (currentFallState == POSSIBLE_FALL) Serial.print("POSSIBLE_FALL");
  if (currentFallState == IMPACT_DETECTED) Serial.print("IMPACT_DETECTED");
  if (currentFallState == SITTING_DOWN) Serial.print("SITTING_DOWN");
  if (currentFallState == FALL_CONFIRMED) Serial.print("FALL_CONFIRMED");
  Serial.print(", Accel: ");
  Serial.print(totalAcceleration);
  Serial.print(" m/s², Rotation: ");
  Serial.print(rotationRate);
  Serial.println(" deg/sec");

  delay(50);
}