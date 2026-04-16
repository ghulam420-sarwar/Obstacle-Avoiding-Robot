# Obstacle-Avoiding Robot

An Arduino UNO robot car that scans its surroundings with an HC-SR04 ultrasonic sensor mounted on a servo, detects obstacles, and autonomously steers toward the clearest path. Includes a buzzer alert and serial debug output.

![Circuit diagram](https://github.com/ghulam420-sarwar/Obstacle-Avoiding-Robot/blob/main/circuit_diagram.png)

## How It Works

```
Drive forward
     │
     ▼
Obstacle < 35 cm?
   │         │
  No        Yes → Stop + beep
   │              │
   └──────────────▼
            Sweep servo 0° → 180°
            Record distance at each 15° step
                   │
                   ▼
            Turn toward angle with most clearance
                   │
                   ▼
            Resume driving forward
```

## Hardware

| Component          | Role                          | Pin(s)          |
|--------------------|-------------------------------|-----------------|
| Arduino UNO        | MCU                           | —               |
| HC-SR04            | Ultrasonic distance sensor    | TRIG=9, ECHO=8  |
| SG90 servo         | Rotate HC-SR04 for scanning   | PWM=10          |
| L298N motor driver | Drive 2× DC motors            | ENA=5, ENB=6, IN1-4 |
| 2× DC gear motors  | Left + right wheels           | Via L298N       |
| Passive buzzer     | Obstacle alert                | GPIO7           |
| 9V battery         | Robot power                   | —               |

## Build

```bash
pio run -t upload --upload-port COM3
pio device monitor
```

## Serial Debug Output

```
Robot ready
front=92 cm
front=87 cm
front=21 cm          ← obstacle detected
  scan   0 deg -> 84 cm
  scan  15 deg -> 91 cm
  scan  30 deg -> 78 cm
  scan  45 deg -> 22 cm
  scan  60 deg -> 18 cm
  scan  75 deg -> 15 cm   ← blocked
  scan  90 deg -> 17 cm
  scan 105 deg -> 80 cm
  scan 120 deg -> 95 cm   ← clearest
  scan 135 deg -> 88 cm
  ...
  best=120 deg (95 cm)
Turn RIGHT
front=90 cm
```

## Tuning

| Constant       | Default | What it controls |
|----------------|---------|-----------------|
| `BASE_SPEED`   | 160     | Forward drive speed (0–255) |
| `TURN_SPEED`   | 140     | Turn speed |
| `STOP_DIST`    | 20 cm   | Distance to trigger stop |
| `SCAN_DIST`    | 35 cm   | Distance to start slowing |
| `TURN_MS`      | 400 ms  | Duration of a ~90° turn |

Adjust `TURN_MS` on your actual robot — it varies by battery level and surface.

## What I Learned

- `pulseIn()` timeout handling to avoid hanging when no echo returns
- Servo settling time is critical — too short gives noisy distance readings
- L298N enable pins need PWM-capable Arduino pins (5, 6, 9, 10, 11, 3)
- Why hysteresis on the stop distance prevents oscillation near obstacles

## License

MIT © Ghulam Sarwar
