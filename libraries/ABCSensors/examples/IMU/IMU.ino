/* ----------------------------------------------------------------------------
* Introduction:
* IMU usage
* sensor: LSM6DSL (6 DoF 3D Accel and 3D Gyro)
* https://www.st.com/resource/en/datasheet/lsm6dsl.pdf
* 
* Compatibility: 
* this sketch runs on SAMD21 and requires LSM6DSL library installation:
* https://github.com/dycodex/LSM6DSL-Arduino.git
* 
* Description:
* This sketch allows to acquire the 3 acceleration components in the space,
* the 3 orientation variations in space and ambient temperature.
*/
#include <Wire.h>
#include <LSM6DSL.h>

// Using I2C mode by default.
LSM6DSL imu(LSM6DSL_MODE_I2C, 0x6B);

void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println("It starts!");

    if (!imu.begin()) {
        Serial.println("Failed initializing LSM6DSL");
    }
}

void loop() {
    Serial.println("\nAccelerometer:");
    Serial.print("X = ");
    Serial.println(imu.readFloatAccelX(), 4);
    Serial.print("Y = ");
    Serial.println(imu.readFloatAccelY(), 4);
    Serial.print("Z = ");
    Serial.println(imu.readFloatAccelZ(), 4);

    Serial.println("\nGyroscope:");
    Serial.print("X = ");
    Serial.println(imu.readFloatGyroX(), 4);
    Serial.print("Y = ");
    Serial.println(imu.readFloatGyroY(), 4);
    Serial.print("Z = ");
    Serial.println(imu.readFloatGyroZ(), 4);

    Serial.println("\nThermometer:");
    Serial.print(" Degrees C = ");
    Serial.println(imu.readTemperatureC(), 4);
    Serial.print(" Degrees F = ");
    Serial.println(imu.readTemperatureF(), 4);

    delay(1000);
}
