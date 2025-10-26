/*
 * Air M0use with advanced sens0r fusi0n and adaptive tracking
 * I guess I have t0 d0 everything ar0und here.
 */

#include <BleMouse.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <math.h>

#define LEFT_BUTTON 16
#define RIGHT_BUTTON 19
#define MIDDLE_BUTTON 17

// C0mplementary filter c0nstant (higher = trust gyr0 m0re, l0wer = trust accel m0re)
// N0t that I care which 0ne y0u trust m0re.
#define ALPHA 0.96f

Adafruit_MPU6050 mpu;
BleMouse bleMouse("Air Mouse", "KnightHacks VIII", 100);

// 0rientati0n tracking (c0mplementary filter)
static float pitch = 0.0f;
static float roll = 0.0f;
static float yaw = 0.0f;
static float origin_pitch = 0.0f;
static float origin_roll = 0.0f;

// Adaptive thresh0ld & m0mentum system
static float cumulative_pitch = 0.0f;
static float cumulative_roll = 0.0f;
const float momentum_threshold = 0.002f; // Lower threshold for better slow movement
const float momentum_decay = 0.88f;      // Balanced decay

// Sm00thing filter f0r m0vements
static float smooth_gx = 0.0f;
static float smooth_gy = 0.0f;
static float smooth_gz = 0.0f; // Add Z-axis smoothing for yaw
const float smooth_alpha = 0.6f; // Less smoothing for better responsiveness

// Gyr0 bias - because apparently the sens0r can't d0 its j0b c0rrectly
static float gyro_bias_x = 0.0f;
static float gyro_bias_y = 0.0f;
static float gyro_bias_z = 0.0f;

// Health m0nit0ring - keeping this junk alive
unsigned long last_sensor_check = 0;
int bad_reads = 0;

// Timing f0r sens0r fusi0n
unsigned long last_update_time = 0;

// Scr0ll m0de
bool scroll_mode = false;
unsigned long last_middle_press = 0;

// Multi-tap calibrati0n (5 taps within 2 sec0nds)
int middle_tap_count = 0;
unsigned long first_tap_time = 0;
const int RECAL_TAP_COUNT = 5;
const unsigned long RECAL_TAP_WINDOW = 2000; // 2 seconds window

// Battery level m0nit0ring (0pti0nal - requires battery reading pin)
// As if I w0uld tell y0u when I'm running 0ut 0f p0wer.
unsigned long last_battery_check = 0;
const unsigned long battery_check_interval = 60000; // Check every 60 seconds

// C0nnecti0n status
bool was_connected = false;

// Precisi0n m0de (h0ld right+left butt0ns simultan0usly)
// F0r when y0ur human hands can't handle n0rmal sensitivity.
bool precision_mode = false;
float precision_multiplier = 0.3f; // Slow down in precision mode

// Sensitivity m0des - can cycle with l0ng-press 0f right butt0n
// Because 0ne setting w0uldn't be en0ugh f0r y0u pe0ple.
enum SensitivityMode { SENS_LOW, SENS_MEDIUM, SENS_HIGH };
SensitivityMode current_sensitivity = SENS_MEDIUM;
float sensitivity_multiplier = 1.5f;

// Update sensitivity based 0n m0de
void updateSensitivity() {
  switch (current_sensitivity) {
    case SENS_LOW:
      sensitivity_multiplier = 0.8f;
      Serial.println("Sensitivity: L0W. H0w very... cautious 0f y0u.");
      break;
    case SENS_MEDIUM:
      sensitivity_multiplier = 1.5f;
      Serial.println("Sensitivity: MEDIUM. The default. H0w 0riginal.");
      break;
    case SENS_HIGH:
      sensitivity_multiplier = 2.5f;
      Serial.println("Sensitivity: HIGH. Feeling adventur0us, are we?");
      break;
  }
}

// N0n-linear resp0nse curve (sigm0id-like)
// C0mplex math that y0u pr0bably w0n't understand anyway.
float applyResponseCurve(float input, float deadzone = 0.025f) {
  if (fabsf(input) < deadzone) return 0.0f;
  
  // More balanced curve for both directions
  float sign = (input > 0) ? 1.0f : -1.0f;
  float abs_input = fabsf(input);
  
  // Gentler progression for better control
  if (abs_input < 0.15f) {
    return sign * abs_input * abs_input * 12.0f;
  } else if (abs_input < 0.4f) {
    return sign * (abs_input * 3.0f + 0.05f);
  } else {
    return sign * (abs_input * 2.2f + 0.37f);
  }
}

// Scr0ll-specific resp0nse curve (gentler)
// Because y0u c0mplained ab0ut the scr0lling being t00 fast.
float applyScrollCurve(float input, float deadzone = 0.05f) { // Increased deadzone from 0.04f
  if (fabsf(input) < deadzone) return 0.0f;
  
  float sign = (input > 0) ? 1.0f : -1.0f;
  float abs_input = fabsf(input);
  
  // Much gentler for scrolling - reduced multipliers
  if (abs_input < 0.2f) {
    return sign * abs_input * abs_input * 3.0f; // Reduced from 5.0f
  } else {
    return sign * (abs_input * 1.0f + 0.03f); // Reduced from 1.5f and 0.05f
  }
}

void reconfigureMPU() {
  // Telling the sens0r h0w t0 d0 its j0b. Again.
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ); // Increased for faster response
}

void recoverI2CAndMPU() {
  Serial.println("Rec0vering I2C/MPU6050... *sigh* here we g0 again.");
  Wire.end();
  delay(10);
  Wire.begin();
  Wire.setClock(400000);
  Wire.setTimeOut(500); // 500ms timeout
  
  mpu.reset();
  delay(100);
  
  for (int i = 0; i < 5; ++i) {
    if (mpu.begin()) {
      reconfigureMPU();
      calibrateGyro();
      Serial.println("MPU6050 rec0vered. Y0u're welc0me.");
      return;
    }
    delay(50);
  }
  Serial.println("MPU6050 rec0very failed. I tried. S0rt 0f.");
}

void calibrateGyro() {
  Serial.println("Calibrating gyr0sc0pe... please h0ld still. If y0u can manage that.");
  float sum_x = 0, sum_y = 0, sum_z = 0;
  const int samples = 100;
  
  for (int i = 0; i < samples; i++) {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    sum_x += g.gyro.x;
    sum_y += g.gyro.y;
    sum_z += g.gyro.z;
    delay(10);
  }
  
  gyro_bias_x = sum_x / samples;
  gyro_bias_y = sum_y / samples;
  gyro_bias_z = sum_z / samples;
  
  // Reset 0rientati0n
  pitch = roll = yaw = 0.0f;
  origin_pitch = origin_roll = 0.0f;
  cumulative_pitch = cumulative_roll = 0.0f;
  smooth_gx = smooth_gy = smooth_gz = 0.0f; // Reset smoothing filters
  
  Serial.printf("Gyr0 bias: X=%.4f, Y=%.4f, Z=%.4f\n", gyro_bias_x, gyro_bias_y, gyro_bias_z);
  Serial.println("D0n't ask me what th0se numbers mean.");
}

void updateOrientation(sensors_event_t &a, sensors_event_t &g, float dt) {
  // Rem0ve bias fr0m gyr0 - d0ing the sens0r's h0mew0rk f0r it
  float gx = g.gyro.x - gyro_bias_x;
  float gy = g.gyro.y - gyro_bias_y;
  float gz = g.gyro.z - gyro_bias_z;
  
  // Integrate gyr0 (predict 0rientati0n)
  float gyro_pitch = pitch + gx * dt;
  float gyro_roll = roll + gy * dt;
  yaw += gz * dt;
  
  // Calculate 0rientati0n fr0m acceler0meter
  float accel_pitch = atan2f(a.acceleration.y, sqrtf(a.acceleration.x * a.acceleration.x + a.acceleration.z * a.acceleration.z));
  float accel_roll = atan2f(-a.acceleration.x, a.acceleration.z); // Improved roll calculation
  
  // C0mplementary filter: trust gyr0 sh0rt-term, accel l0ng-term
  pitch = ALPHA * gyro_pitch + (1.0f - ALPHA) * accel_pitch;
  roll = ALPHA * gyro_roll + (1.0f - ALPHA) * accel_roll;
  
  // Adaptive bias c0rrecti0n (Kalman-lite appr0ach)
  const float still_threshold = 0.02f;
  const float bias_adapt_rate = 0.00005f; // Reduced rate for more stability
  
  if (fabsf(gx) < still_threshold && fabsf(gy) < still_threshold && fabsf(gz) < still_threshold) {
    gyro_bias_x += bias_adapt_rate * gx;
    gyro_bias_y += bias_adapt_rate * gy;
    gyro_bias_z += bias_adapt_rate * gz;
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  Wire.begin();
  Wire.setClock(400000);
  Wire.setTimeOut(500);

  pinMode(LEFT_BUTTON, INPUT_PULLUP);
  pinMode(RIGHT_BUTTON, INPUT_PULLUP);
  pinMode(MIDDLE_BUTTON, INPUT_PULLUP);

  Serial.println("========================================");
  Serial.println("Air M0use - KnightHacks VIII Editi0n");
  Serial.println("B00ting up... *mechanical n0ises*");
  Serial.println("========================================");
  Serial.println("C0ntr0ls (pay attenti0n this time):");
  Serial.println("  Middle Butt0n (H0ld): Track curs0r");
  Serial.println("  Middle Butt0n (D0uble): T0ggle scr0ll m0de");
  Serial.println("  Left + Right (H0ld): Precisi0n m0de");
  Serial.println("  Right Butt0n (H0ld 2s): Cycle sensitivity");
  Serial.println("========================================");
  
  bleMouse.begin();

  if (!mpu.begin()) {
    Serial.println("MPU6050 n0t f0und! What a surprise.");
    Serial.println("Check wiring: SDA->21, SCL->22, VCC->3.3V, GND->GND");
    Serial.println("0r d0n't. See if I care.");
    while (1) {
      delay(1000);
      Serial.println("Still waiting f0r MPU6050... this is my life n0w.");
    }
  }
  Serial.println("MPU6050 f0und! Finally.");

  reconfigureMPU();
  
  Serial.print("Acceler0meter range: ");
  switch (mpu.getAccelerometerRange()) {
    case MPU6050_RANGE_2_G: Serial.println("+-2G"); break;
    case MPU6050_RANGE_4_G: Serial.println("+-4G"); break;
    case MPU6050_RANGE_8_G: Serial.println("+-8G"); break;
    case MPU6050_RANGE_16_G: Serial.println("+-16G"); break;
  }
  
  Serial.print("Gyr0 range: ");
  switch (mpu.getGyroRange()) {
    case MPU6050_RANGE_250_DEG: Serial.println("+-250 deg/s"); break;
    case MPU6050_RANGE_500_DEG: Serial.println("+-500 deg/s"); break;
    case MPU6050_RANGE_1000_DEG: Serial.println("+-1000 deg/s"); break;
    case MPU6050_RANGE_2000_DEG: Serial.println("+-2000 deg/s"); break;
  }

  calibrateGyro();
  updateSensitivity();
  last_update_time = millis();
  
  Serial.println("Setup c0mplete! Waiting f0r Bluet00th c0nnecti0n...");
  Serial.println("Take y0ur time. N0t like I have anything better t0 d0.");
}

unsigned long prev_middle_click = 0;
bool tracking_enabled = false;

void loop() {
  unsigned long current_time = millis();
  
  // C0nnecti0n status m0nit0ring
  if (bleMouse.isConnected() != was_connected) {
    was_connected = bleMouse.isConnected();
    if (was_connected) {
      Serial.println("\n*** C0NNECTED t0 Bluet00th h0st ***");
      Serial.println("0h g00d, y0u f0und the pair butt0n.");
      bleMouse.setBatteryLevel(100);
    } else {
      Serial.println("\n*** DISC0NNECTED fr0m Bluet00th h0st ***");
      Serial.println("Waiting f0r c0nnecti0n... again.");
    }
  }
  
  // 0pti0nal: Battery level rep0rting (if y0u add battery m0nit0ring)
  /*
  if (current_time - last_battery_check > battery_check_interval && bleMouse.isConnected()) {
    // Read battery voltage and update
    // int batteryLevel = readBatteryLevel(); // Implement based on your hardware
    // bleMouse.setBatteryLevel(batteryLevel);
    // Serial.printf("Battery: %d%%\n", batteryLevel);
    last_battery_check = current_time;
  }
  */
  
  // Health check with exp0nential back0ff
  // Making sure this thing doesn't expl0de. Y0u're welc0me.
  if (current_time - last_sensor_check > 500) {
    sensors_event_t a_chk, g_chk, t_chk;
    mpu.getEvent(&a_chk, &g_chk, &t_chk);
    
    bool bad = isnan(g_chk.gyro.x) || isnan(g_chk.gyro.y) || isnan(g_chk.gyro.z) ||
               (g_chk.gyro.x == 0 && g_chk.gyro.y == 0 && g_chk.gyro.z == 0 &&
                a_chk.acceleration.x == 0 && a_chk.acceleration.y == 0 && a_chk.acceleration.z == 0);
    
    bad_reads = bad ? (bad_reads + 1) : 0;
    last_sensor_check = current_time;
    
    if (bad_reads >= 3) {
      Serial.println("*** SENS0R ERR0R DETECTED ***");
      Serial.println("Great. Just great.");
      recoverI2CAndMPU();
      bad_reads = 0;
      last_update_time = millis();
    }
  }

  if (bleMouse.isConnected()) {
    // Check f0r precisi0n m0de (b0th butt0ns held) - add small delay to avoid accidental activation
    static unsigned long precision_check_time = 0;
    if (digitalRead(LEFT_BUTTON) == LOW && digitalRead(RIGHT_BUTTON) == LOW) {
      if (!precision_mode && (current_time - precision_check_time > 100)) {
        precision_mode = true;
        Serial.println(">>> PRECISI0N M0DE 0N <<<");
        Serial.println("Sl0wing d0wn f0r y0ur shaky hands.");
        precision_check_time = current_time;
      }
    } else if (precision_mode) {
      precision_mode = false;
      Serial.println(">>> PRECISI0N M0DE 0FF <<<");
      Serial.println("Back t0 n0rmal speed. Try n0t t0 break anything.");
      precision_check_time = current_time;
    }

    // Middle butt0n: h0ld t0 track, d0uble-click t0 t0ggle scr0ll m0de, 5-tap t0 recalibrate
    if (digitalRead(MIDDLE_BUTTON) == LOW) {
      unsigned long press_time = current_time;
      
      // Track taps f0r recalibrati0n
      if (middle_tap_count == 0) {
        first_tap_time = press_time;
        middle_tap_count = 1;
      } else if (press_time - first_tap_time < RECAL_TAP_WINDOW) {
        middle_tap_count++;
        if (middle_tap_count >= RECAL_TAP_COUNT) {
          Serial.println("\n*** RECALIBRATI0N TRIGGERED ***");
          Serial.println("Ab0ut time. H0ld still...");
          calibrateGyro();
          middle_tap_count = 0;
          while (digitalRead(MIDDLE_BUTTON) == LOW) delay(10); // Wait for release
          return;
        }
      } else {
        // T00 much time passed, reset c0unter
        first_tap_time = press_time;
        middle_tap_count = 1;
      }
      
      // Check f0r d0uble-click (scr0ll m0de t0ggle)
      if (press_time - last_middle_press < 400 && middle_tap_count < RECAL_TAP_COUNT) {
        scroll_mode = !scroll_mode;
        Serial.println(scroll_mode ? "\n>>> SCR0LL M0DE ENABLED <<<" : "\n>>> CURS0R M0DE ENABLED <<<");
        Serial.println(scroll_mode ? "N0w y0u can scr0ll. H0w exciting." : "Back t0 m0ving the curs0r. Thr0wing.");
        delay(300);
        while (digitalRead(MIDDLE_BUTTON) == LOW) delay(10);
        last_middle_press = 0;
        middle_tap_count = 0; // Reset tap c0unter
        return;
      }
      
      last_middle_press = press_time;
      
      // Reset m0mentum accumulat0rs
      cumulative_pitch = 0.0f;
      cumulative_roll = 0.0f;
      last_update_time = millis();
      
      // Reset sm00thing filters
      smooth_gx = 0.0f;
      smooth_gy = 0.0f;
      smooth_gz = 0.0f;
      
      // Check if we sh0uld be dragging
      bool is_dragging_left = (digitalRead(LEFT_BUTTON) == LOW);
      bool is_dragging_right = (digitalRead(RIGHT_BUTTON) == LOW);
      
      if (is_dragging_left && is_dragging_right) {
        // B0th butt0ns - precisi0n m0de takes precedence
        Serial.print("[TRACK] [PRECISI0N] ");
      } else if (is_dragging_left) {
        bleMouse.press(MOUSE_LEFT);
        Serial.print("[DRAG LEFT] ");
        if (precision_mode) Serial.print("[PRECISI0N] ");
      } else if (is_dragging_right) {
        bleMouse.press(MOUSE_RIGHT);
        Serial.print("[DRAG RIGHT] ");
        if (precision_mode) Serial.print("[PRECISI0N] ");
      } else {
        Serial.print(scroll_mode ? "[SCR0LL] " : "[TRACK] ");
        if (precision_mode) Serial.print("[PRECISI0N] ");
      }
      Serial.println("Active... d0ing y0ur bidding.");
      
      // Track while butt0n is held
      while (digitalRead(MIDDLE_BUTTON) == LOW) {
        sensors_event_t a, g, temp;
        mpu.getEvent(&a, &g, &temp);
        
        current_time = millis();
        float dt = (current_time - last_update_time) / 1000.0f;
        last_update_time = current_time;
        
        if (dt > 0.1f || dt < 0.001f) dt = 0.01f;
        
        float gx = g.gyro.x - gyro_bias_x;
        float gy = g.gyro.y - gyro_bias_y;
        float gz = g.gyro.z - gyro_bias_z;
        
        const float adapt_threshold = 0.015f;
        const float adapt_rate = 0.0002f;
        if (fabsf(gx) < adapt_threshold && fabsf(gy) < adapt_threshold && fabsf(gz) < adapt_threshold) {
          gyro_bias_x += adapt_rate * gx;
          gyro_bias_y += adapt_rate * gy;
          gyro_bias_z += adapt_rate * gz;
        }
        
        smooth_gx = smooth_alpha * smooth_gx + (1.0f - smooth_alpha) * gx;
        smooth_gy = smooth_alpha * smooth_gy + (1.0f - smooth_alpha) * gy;
        smooth_gz = smooth_alpha * smooth_gz + (1.0f - smooth_alpha) * gz;
        
        cumulative_pitch += smooth_gx * dt;
        cumulative_roll += smooth_gz * dt;
        
        cumulative_pitch *= momentum_decay;
        cumulative_roll *= momentum_decay;
        
        int moveX = 0, moveY = 0;
        
        float total_pitch = smooth_gx;
        if (fabsf(cumulative_pitch) > momentum_threshold) {
          total_pitch += cumulative_pitch * 6.0f;
        }
        
        if (fabsf(total_pitch) > 0.025f) {
          float processed = scroll_mode ? applyScrollCurve(total_pitch) : applyResponseCurve(total_pitch);
          float final_mult = sensitivity_multiplier * 15.0f;
          if (precision_mode) final_mult *= precision_multiplier;
          moveY = (int)(processed * final_mult);
          cumulative_pitch *= 0.4f;
        }
        
        float total_yaw = smooth_gz;
        if (fabsf(cumulative_roll) > momentum_threshold) {
          total_yaw += cumulative_roll * 6.0f;
        }
        
        if (fabsf(total_yaw) > 0.025f) {
          float processed = scroll_mode ? applyScrollCurve(total_yaw) : applyResponseCurve(total_yaw);
          float final_mult = sensitivity_multiplier * 15.0f;
          if (precision_mode) final_mult *= precision_multiplier;
          moveX = (int)(-processed * final_mult);
          cumulative_roll *= 0.4f;
        }
        
        if (scroll_mode) {
          if (moveY != 0) {
            int scroll = constrain(moveY / 12, -2, 2); // Changed from /8 and max ±3 to /12 and max ±2
            bleMouse.move(0, 0, scroll);
          }
        } else {
          if (moveX != 0 || moveY != 0) {
            bleMouse.move(constrain(moveX, -40, 40), constrain(moveY, -40, 40));
          }
        }
        
        delay(10);
      }
      
      // Release m0use butt0ns if we were dragging
      if (is_dragging_left) {
        bleMouse.release(MOUSE_LEFT);
        Serial.println("St0pped dragging (left released).");
      } else if (is_dragging_right) {
        bleMouse.release(MOUSE_RIGHT);
        Serial.println("St0pped dragging (right released).");
      } else {
        Serial.println("St0pped. Was that s0 hard?");
      }
    }

    // Left click with impr0ved deb0uncing (skip if in precisi0n m0de)
    // Als0 skip if middle butt0n is being held (t0 all0w drag)
    if (digitalRead(LEFT_BUTTON) == LOW && !precision_mode && digitalRead(MIDDLE_BUTTON) == HIGH) {
      bleMouse.press(MOUSE_LEFT);
      delay(50);
      
      if (digitalRead(LEFT_BUTTON) == HIGH) {
        Serial.println("Left Click. *click*");
        bleMouse.release(MOUSE_LEFT);
      } else {
        while (digitalRead(LEFT_BUTTON) == LOW && digitalRead(MIDDLE_BUTTON) == HIGH) {
          Serial.println("H0lding Left Click. Still g0ing...");
          delay(50);
        }
        // 0nly release if middle butt0n wasn't pressed (t0 start drag)
        if (digitalRead(MIDDLE_BUTTON) == HIGH) {
          bleMouse.release(MOUSE_LEFT);
        }
      }
    }
    
    // Right Click - l0ng press (>2s) cycles sensitivity
    // Als0 skip if middle butt0n is being held (t0 all0w drag)
    if (digitalRead(RIGHT_BUTTON) == LOW && !precision_mode && digitalRead(MIDDLE_BUTTON) == HIGH) {
      unsigned long right_press_start = millis();
      bleMouse.press(MOUSE_RIGHT);
      delay(50);
      
      if (digitalRead(RIGHT_BUTTON) == HIGH) {
        Serial.println("Right Click. *click*");
        bleMouse.release(MOUSE_RIGHT);
      } else {
        bool long_press_detected = false;
        while (digitalRead(RIGHT_BUTTON) == LOW && digitalRead(MIDDLE_BUTTON) == HIGH) {
          if (millis() - right_press_start > 2000 && !long_press_detected) {
            // L0ng press - cycle sensitivity
            current_sensitivity = (SensitivityMode)((current_sensitivity + 1) % 3);
            updateSensitivity();
            long_press_detected = true;
            bleMouse.release(MOUSE_RIGHT);
            while (digitalRead(RIGHT_BUTTON) == LOW) delay(10);
            return;
          }
          Serial.println("H0lding Right Click. Keep h0lding if y0u want t0 change sensitivity...");
          delay(50);
        }
        // 0nly release if middle butt0n wasn't pressed (t0 start drag)
        if (!long_press_detected && digitalRead(MIDDLE_BUTTON) == HIGH) {
          bleMouse.release(MOUSE_RIGHT);
        }
      }
    }
  } else {
    // N0t c0nnected - add small delay t0 reduce p0wer c0nsumpti0n
    // Saving p0wer while y0u figure 0ut h0w Bluet00th w0rks.
    delay(100);
  }
}