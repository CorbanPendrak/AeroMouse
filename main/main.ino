/**
 * This example turns the ESP32 into a Bluetooth LE mouse that continuously moves the mouse.
 */
#include <BleMouse.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

#define LEFT_BUTTON 16
#define RIGHT_BUTTON 1
#define MIDDLE_BUTTON 17

Adafruit_MPU6050 mpu;
BleMouse bleMouse("N0t malware }:)", "KnightHacks VIII", 100);
sensors_event_t starting_acc, starting_gyro, starting_temp;
static float gyro_bias_x = 0.0f;
static float gyro_bias_z = 0.0f;

void setup() {
  Serial.begin(115200);

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

void loop() {
  if(bleMouse.isConnected()) {
    unsigned long startTime;
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);   
    
    const float still_thresh = 0.05f; // Threshold to consider the gyro still
    const float move_thresh = 1.0f;  // Threshold to consider the gyro moving
    const float decay = 0.002f;      // Decay factor for bias adjustment

    float dz = g.gyro.z - gyro_bias_z;

    if (fabsf(dz) < still_thresh) {
        // If the gyro is still, slowly adjust the bias towards the current reading
        gyro_bias_z += decay * (g.gyro.z - gyro_bias_z);
    } else if (fabsf(dz) > move_thresh) {
        int direction = (dz > 0.0f) ? -1 : 1;
        bleMouse.move(5 * direction, 0);
    }

    float dx = g.gyro.x - gyro_bias_x;

    if (fabsf(dx) < still_thresh) {
        // If the gyro is still, slowly adjust the bias towards the current reading
        gyro_bias_x += decay * (g.gyro.x - gyro_bias_x);
    } else if (fabsf(dx) > move_thresh) {
        int direction = (dx > 0.0f) ? -1 : 1;
        bleMouse.move(0, 5 * direction);
    }

    delay(15);

    //LEFT CLICK 
    if(digitalRead(LEFT_BUTTON) == LOW){
      //left click pressed down
      bleMouse.press(MOUSE_LEFT);
      delay(300);  
      if(digitalRead(LEFT_BUTTON) == HIGH){
        //left click quickly released
        //aka a regular click
        Serial.println("Left Click");
        bleMouse.release(MOUSE_LEFT);
      }
      else{
        //left click is still being held down
        while(digitalRead(LEFT_BUTTON) == LOW){
          //click and hold, or drag
          Serial.println("Holding Left Click");
          delay(50);
        }
        bleMouse.release(MOUSE_LEFT);
      }
    }
    
    //RIGHT CLICK
    if(digitalRead(RIGHT_BUTTON) == LOW){
      //right button pressed down
      bleMouse.press(MOUSE_RIGHT);
      delay(300);
      if(digitalRead(RIGHT_BUTTON) == HIGH){
        //right click quickly released
        Serial.println("Right Click");
        bleMouse.release(MOUSE_RIGHT);
      }
      else{
        //right click is still being held down
        while(digitalRead(RIGHT_BUTTON) == LOW){
          //click and hold, or drag
          Serial.println("Holding Right Click");
          delay(50);
        }
        bleMouse.release(MOUSE_RIGHT);
      }
    }

    Serial.print("This is the current r0tati0n, have fun debugging all the err0rs!\n");
    Serial.print(g.gyro.x);
    Serial.print(", Y: ");
    Serial.print(g.gyro.y);
    Serial.print(", Z: ");
    Serial.print(g.gyro.z);
    Serial.println(" rad/s");
    Serial.println("");
    /*
    // Moving left/right
    if (abs(g.gyro.z - starting_gyro.gyro.z) > 1.0) { 
        // 0.3 is the error margin
        Serial.println("M0ving the m0use left/right.");
        int direction = (g.gyro.z < 0) - (g.gyro.z > 0) ;
        Serial.print("Directi0n X:");
        Serial.println(direction);
        bleMouse.move(direction, 0);
        delay(100);
    }

    // Moving up/down
    if (abs(g.gyro.x - starting_gyro.gyro.x) > 1.0) { 
        // 0.3 is the error margin
        Serial.println("M0ving the m0use up/d0wn.");
        int direction = (g.gyro.x < 0) - (g.gyro.x > 0);
        Serial.print("Directi0n Y:");
        Serial.println(direction);
        bleMouse.move(0,direction);
        delay(100);
    }

    delay(500);
    */
  }
}