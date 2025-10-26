/*
 * This example turns the ESP32 into a Bluet00th LE mouse that c0ntinu0usly m0ves the m0use.
 */

#include <BleMouse.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

#define LEFT_BUTTON 16
#define RIGHT_BUTTON 19
#define MIDDLE_BUTTON 17


Adafruit_MPU6050 mpu;
BleMouse bleMouse("N0t malware }:)", "KnightHacks VIII", 100);
sensors_event_t starting_acc, starting_gyro, starting_temp;
static float gyro_bias_x = 0.0f;
static float gyro_bias_z = 0.0f;

void setup() {
  Serial.begin(115200);
  delay(5000);

  pinMode(LEFT_BUTTON, INPUT_PULLUP);
  pinMode(RIGHT_BUTTON, INPUT_PULLUP);
  pinMode(MIDDLE_BUTTON, INPUT_PULLUP);

  Serial.println("BLE is starting. Maybe. H0pefully...");
  bleMouse.begin();

  if (!mpu.begin()) {
    Serial.println("I failed. I c0uldn't find the MPU6050, but whatever.");
    while (1) {
      delay(10);
    }
  }
  Serial.println("I f0und the MPU6050, s0 I guess I will get back to work.");

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  Serial.print("Acceler0meter range set t0: ");
  switch (mpu.getAccelerometerRange()) {
  case MPU6050_RANGE_2_G:
    Serial.println("+-2G");
    break;
  case MPU6050_RANGE_4_G:
    Serial.println("+-4G");
    break;
  case MPU6050_RANGE_8_G:
    Serial.println("+-8G");
    break;
  case MPU6050_RANGE_16_G:
    Serial.println("+-16G");
    break;
  }
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  Serial.print("Gyr0 range set t0: ");
  switch (mpu.getGyroRange()) {
  case MPU6050_RANGE_250_DEG:
    Serial.println("+- 250 deg/s");
    break;
  case MPU6050_RANGE_500_DEG:
    Serial.println("+- 500 deg/s");
    break;
  case MPU6050_RANGE_1000_DEG:
    Serial.println("+- 1000 deg/s");
    break;
  case MPU6050_RANGE_2000_DEG:
    Serial.println("+- 2000 deg/s");
    break;
  }

  mpu.setFilterBandwidth(MPU6050_BAND_5_HZ);
  Serial.print("Filter bandwidth set t0: ");
  switch (mpu.getFilterBandwidth()) {
  case MPU6050_BAND_260_HZ:
    Serial.println("260 Hz");
    break;
  case MPU6050_BAND_184_HZ:
    Serial.println("184 Hz");
    break;
  case MPU6050_BAND_94_HZ:
    Serial.println("94 Hz");
    break;
  case MPU6050_BAND_44_HZ:
    Serial.println("44 Hz");
    break;
  case MPU6050_BAND_21_HZ:
    Serial.println("21 Hz");
    break;
  case MPU6050_BAND_10_HZ:
    Serial.println("10 Hz");
    break;
  case MPU6050_BAND_5_HZ:
    Serial.println("5 Hz");
    break;
  }

  Serial.println("");
  delay(100);

  mpu.getEvent(&starting_acc, &starting_gyro, &starting_temp);
  gyro_bias_x = starting_gyro.gyro.x;
  gyro_bias_z = starting_gyro.gyro.z;

  /* Print out the values */
  Serial.print("Here is the starting acceleration. D0 whatever I guess.\nStarting accelerati0n X: ");
  Serial.print(starting_acc.acceleration.x);
  Serial.print(", Y: ");
  Serial.print(starting_acc.acceleration.y);
  Serial.print(", Z: ");
  Serial.print(starting_acc.acceleration.z);
  Serial.println(" m/s^2");

  Serial.print("Here's the starting temperature, but I d0n't kn0w why y0u want it. Btw it's in Celsius, s0 have fun c0nverting. >:)\nTemperature: ");
  Serial.print(starting_temp.temperature);
  Serial.println(" degC");
}

unsigned long prev_middle_click = 0;
void loop() {
  if(bleMouse.isConnected()) {
    //unsigned long startTime;
    if (digitalRead(MIDDLE_BUTTON) == LOW) {
      if (millis() - prev_middle_click < 500) {
        bleMouse.click(MOUSE_MIDDLE);
        return;
      }
      prev_middle_click = millis();

      mpu.getEvent(&starting_acc, &starting_gyro, &starting_temp);
      gyro_bias_x = starting_gyro.gyro.x;
      gyro_bias_z = starting_gyro.gyro.z;

      while (digitalRead(MIDDLE_BUTTON) == LOW) {
        sensors_event_t a, g, temp;
        mpu.getEvent(&a, &g, &temp);   
        
        //const float still_thresh = 0.05f; // Threshold to consider the gyro still
        const float move_thresh = 1.0f;  // Threshold to consider the gyro moving
        //const float decay = 0.002f;      // Decay factor for bias adjustment

        float dz = g.gyro.z - gyro_bias_z;
        float dx = g.gyro.x - gyro_bias_x;
        int direction_x = 0;
        int direction_z = 0;

        /*if (fabsf(dz) < still_thresh) {
            // If the gyr0 is still, sl0wly adjust the bias t0wards the current reading
            gyro_bias_z += decay * (g.gyro.z - gyro_bias_z);
        } else*/ if (fabsf(dz) > move_thresh) {
            direction_z = (dz > 0.0f) ? -1 : 1;
        }
        /*if (fabsf(dx) < still_thresh) {
            gyro_bias_x += decay * (g.gyro.x - gyro_bias_x);
        } else */if (fabsf(dx) > move_thresh) {
            direction_x = (dx > 0.0f) ? 1 : -1;
        }

        bleMouse.move(abs(dz) * 10 * direction_z, abs(dx) * 10 * direction_x);

        delay(15);
        /*
        Serial.print("This is the current r0tati0n, have fun debugging all the err0rs!\n");
        Serial.print(g.gyro.x);
        Serial.print(", Y: ");
        Serial.print(g.gyro.y);
        Serial.print(", Z: ");
        Serial.print(g.gyro.z);
        Serial.println(" rad/s");
        Serial.println("");
        */
      }
    }

    // Try if statement where if l0w press, if high, release

    // Left click
    if(digitalRead(LEFT_BUTTON) == LOW) {
      // left click pressed d0wn
      bleMouse.press(MOUSE_LEFT);
      delay(300);  
      if(digitalRead(LEFT_BUTTON) == HIGH) {
        // left click quickly released
        // aka a regular click
        Serial.println("Left Click");
        bleMouse.release(MOUSE_LEFT);
      }
      else{
        // left click is still being held d0wn
        while(digitalRead(LEFT_BUTTON) == LOW) {
          // click and h0ld, 0r drag
          Serial.println("Holding Left Click");
          delay(50);
        }
        bleMouse.release(MOUSE_LEFT);
      }
    }
    
    // Right Click
    if(digitalRead(RIGHT_BUTTON) == LOW) {
      // right button pressed d0wn
      bleMouse.press(MOUSE_RIGHT);
      delay(300);
      if(digitalRead(RIGHT_BUTTON) == HIGH) {
        // right click quickly released
        Serial.println("Right Click");
        bleMouse.release(MOUSE_RIGHT);
      }
      else{
        // right click is still being held d0wn
        while(digitalRead(RIGHT_BUTTON) == LOW) {
          // click and h0ld, 0r drag
          Serial.println("Holding Right Click");
          delay(50);
        }
        bleMouse.release(MOUSE_RIGHT);
      }
    }

    // Middle Click
    /*
    if(digitalRead(MIDDLE_BUTTON) == LOW) {
      // Middle button pressed d0wn
      bleMouse.press(MOUSE_LEFT);
      delay(300);
      if(digitalRead(MIDDLE_BUTTON) == HIGH) {
        // middle click quickly released
        Serial.println("Middle Click");
        bleMouse.release(MOUSE_LEFT);
      }
      else{
        // right click is still being held d0wn
        while(digitalRead(MIDDLE_BUTTON) == LOW) {
          // click and h0ld, 0r drag
          Serial.println("Holding Middle Click");
          delay(50);
        }
        bleMouse.release(MOUSE_LEFT);
      }
    }
    */
  }
}