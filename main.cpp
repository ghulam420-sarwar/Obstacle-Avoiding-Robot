/**
 * Obstacle-Avoiding Robot
 * -----------------------
 * Target MCU : Arduino UNO (ATmega328P)
 * Sensors    : HC-SR04 ultrasonic distance sensor on SG90 servo (scan)
 * Actuators  : L298N motor driver, 2× DC gear motors, passive buzzer
 * Author     : Ghulam Sarwar
 *
 * Behaviour:
 *   1. Drive forward
 *   2. If obstacle closer than STOP_DIST → stop + beep
 *   3. Sweep servo 0°→180° and record distances every 15°
 *   4. Turn toward the direction with the most clearance
 *   5. Resume driving
 *
 * MIT License
 */

#include <Arduino.h>
#include <Servo.h>

// ── pins ──────────────────────────────────────────────────────────────────────
#define TRIG_PIN    9
#define ECHO_PIN    8
#define SERVO_PIN  10
#define BUZZER_PIN  7

// L298N motor A (left wheel)
#define ENA_PIN     5   // PWM speed
#define IN1_PIN     4
#define IN2_PIN     3

// L298N motor B (right wheel)
#define ENB_PIN     6   // PWM speed
#define IN3_PIN     A0
#define IN4_PIN     A1

// ── tuning ────────────────────────────────────────────────────────────────────
constexpr uint8_t  BASE_SPEED   = 160;  // 0-255
constexpr uint8_t  TURN_SPEED   = 140;
constexpr uint16_t STOP_DIST    = 20;   // cm
constexpr uint16_t SCAN_DIST    = 35;   // cm — start scanning when closer
constexpr uint16_t TURN_MS      = 400;  // ms for a ~90° turn (tune per robot)
constexpr uint8_t  SCAN_STEP    = 15;   // servo degrees per step

Servo sonar;

// ── motor helpers ─────────────────────────────────────────────────────────────
void motorLeft(int spd) {        // positive = fwd, negative = rev
    if (spd >= 0) { digitalWrite(IN1_PIN, HIGH); digitalWrite(IN2_PIN, LOW);  }
    else          { digitalWrite(IN1_PIN, LOW);  digitalWrite(IN2_PIN, HIGH); }
    analogWrite(ENA_PIN, abs(spd));
}
void motorRight(int spd) {
    if (spd >= 0) { digitalWrite(IN3_PIN, HIGH); digitalWrite(IN4_PIN, LOW);  }
    else          { digitalWrite(IN3_PIN, LOW);  digitalWrite(IN4_PIN, HIGH); }
    analogWrite(ENB_PIN, abs(spd));
}
void stopMotors()              { analogWrite(ENA_PIN, 0); analogWrite(ENB_PIN, 0); }
void driveForward(uint8_t spd) { motorLeft(spd);  motorRight(spd); }
void turnLeft(uint8_t spd)     { motorLeft(-spd); motorRight(spd); }
void turnRight(uint8_t spd)    { motorLeft(spd);  motorRight(-spd); }

// ── ultrasonic ────────────────────────────────────────────────────────────────
uint16_t measureCm() {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    uint32_t dur = pulseIn(ECHO_PIN, HIGH, 30000UL);  // 30 ms timeout = ~5 m
    if (dur == 0) return 400;                          // no echo → far away
    return (uint16_t)(dur / 58U);
}

// ── beep ──────────────────────────────────────────────────────────────────────
void beep(uint16_t freq, uint16_t dur) {
    tone(BUZZER_PIN, freq, dur);
}

// ── obstacle scan ─────────────────────────────────────────────────────────────
uint8_t bestAngle() {
    constexpr uint8_t STEPS = 180 / SCAN_STEP + 1;
    uint16_t dist[STEPS];
    uint8_t  angles[STEPS];

    for (uint8_t i = 0; i < STEPS; ++i) {
        uint8_t angle = i * SCAN_STEP;
        angles[i] = angle;
        sonar.write(angle);
        delay(120);  // let servo settle
        dist[i] = measureCm();
        Serial.printf("  scan %3u deg -> %u cm\n", angle, dist[i]);
    }

    // Return servo to forward
    sonar.write(90);
    delay(200);

    // Find angle with max clearance
    uint8_t best = 0;
    uint16_t maxD = 0;
    for (uint8_t i = 0; i < STEPS; ++i) {
        if (dist[i] > maxD) { maxD = dist[i]; best = i; }
    }
    Serial.printf("  best=%u deg (%u cm)\n", angles[best], maxD);
    return angles[best];
}

// ── setup ────────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    for (uint8_t p : {IN1_PIN, IN2_PIN, IN3_PIN, IN4_PIN})
        pinMode(p, OUTPUT);

    sonar.attach(SERVO_PIN);
    sonar.write(90);   // face forward
    delay(500);
    Serial.println("Robot ready");
}

// ── main loop ─────────────────────────────────────────────────────────────────
void loop() {
    uint16_t d = measureCm();
    Serial.printf("front=%u cm\n", d);

    if (d > SCAN_DIST) {
        driveForward(BASE_SPEED);
        return;
    }

    stopMotors();
    beep(1000, 200);
    delay(300);

    if (d <= STOP_DIST) {
        uint8_t best = bestAngle();

        if (best < 75) {
            Serial.println("Turn LEFT");
            turnLeft(TURN_SPEED);
            delay(TURN_MS * (75 - best) / 75);
        } else if (best > 105) {
            Serial.println("Turn RIGHT");
            turnRight(TURN_SPEED);
            delay(TURN_MS * (best - 105) / 75);
        } else {
            // Obstacle directly ahead even after scan — reverse a bit
            Serial.println("Reverse");
            motorLeft(-TURN_SPEED);
            motorRight(-TURN_SPEED);
            delay(400);
        }
        stopMotors();
        delay(200);
    }
}
