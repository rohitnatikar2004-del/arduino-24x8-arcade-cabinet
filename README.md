# arduino-24x8-arcade-cabinet
This project is an Arduino-powered mini arcade cabinet that brings classic retro games to life on a $24 \times 8$ LED matrix grid. Controlled via an analog joystick, dedicated navigation buttons, and an I2C LCD display, the system delivers an authentic arcade experience with custom 8-bit sound effects, visual animations, and smooth game mechanics.
An interactive dual-game arcade console powered by an Arduino Microcontroller, an I2C 16x2 LCD display, a 24x8 MAX7219 LED matrix stack, an analog joystick, dedicated external navigation buttons, and a piezo buzzer.

Features dynamic LED transitions, food pulsing effects, screen shake on game-over, and custom retro sound effects!

---

## 🚀 Features

* **Dual Retro Games:**
  * 🐍 **Snake:** Progressive speed acceleration, blinking food target, and wrap-around collision detection.
  * 🧱 **Horizontal Tetris:** 7 classic piece shapes, rotation controls, line-clear flash animations, and drop mechanics.
* **Visual Polish & Juiciness:**
  * Power-on sweeping matrix boot sequence.
  * Screen-shake effect on game over.
  * High-intensity matrix flash upon clearing lines in Tetris.
* **Dual Display Integration:**
  * **24x8 Matrix:** Main gameplay and menu selection digits.
  * **16x2 I2C LCD:** Real-time score tracking, mode indicator, and menu interaction.
* **Sound Effects:** Dynamic 8-bit audio feedback for moves, rotations, line clears, eating food, and game overs via a piezo buzzer.

---

## 🛠️ Hardware Requirements

| Component | Quantity | Notes |
| :--- | :--- | :--- |
| **Arduino Board** | 1 | Uno, Nano, or Mega |
| **MAX7219 8x8 LED Matrix** | 3 Modules | Daisy-chained (24x8 grid) |
| **I2C LCD 16x2 Display** | 1 | Standard I2C address `0x27` |
| **Analog Joystick** | 1 | Dual-axis with integrated push button |
| **Push Buttons** | 2 | Instant-action menu & restart switches |
| **Piezo Buzzer** | 1 | Passive buzzer for tone generation |
| **Breadboard & Wires** | — | Jumper wires and standard breadboard |

---

## 🔌 Pin Mapping

### 1. MAX7219 LED Matrix (Daisy-Chained 3 Modules)
* **DIN** ➡️ Arduino Pin `12`
* **CLK** ➡️ Arduino Pin `11`
* **CS / LOAD** ➡️ Arduino Pin `10`
* **VCC / GND** ➡️ 5V / GND

### 2. I2C 16x2 LCD Display
* **SDA** ➡️ Arduino Pin `A4` (or SDA pin)
* **SCL** ➡️ Arduino Pin `A5` (or SCL pin)
* **VCC / GND** ➡️ 5V / GND

### 3. Analog Joystick & Buzzer
* **VRX** ➡️ Arduino Pin `A0`
* **VRY** ➡️ Arduino Pin `A1`
* **SW (Joystick Click)** ➡️ Arduino Pin `2` *(Uses internal pullup)*
* **Buzzer (+)** ➡️ Arduino Pin `3`

### 4. External Push Buttons
* **Restart Button** ➡️ Arduino Pin `4` *(Uses internal pullup)*
* **Menu Button** ➡️ Arduino Pin `5` *(Uses internal pullup)*

---

## 💻 Software Dependencies & Libraries

Ensure you have the following libraries installed in your **Arduino IDE** (`Sketch -> Include Library -> Manage Libraries`):

1. **`LiquidCrystal_I2C`** by Frank de Brabander
2. **`LedControl`** by Eberhard Fahle

---

## 🎮 How to Play

### Menu Navigation
* **Move Joystick Left / Right:** Switch between Game `1` (Snake) and Game `2` (Tetris).
* **Press Joystick Button (SW):** Select highlighted game.

### In-Game Controls
* **Snake:**
  * Move Joystick (Up / Down / Left / Right) to steer.
  * Eat blinking apples to gain length and increase score.
* **Tetris:**
  * Move Joystick (Left / Right) to shift position down the line.
  * Press **Joystick Button (SW)** to rotate piece.
* **System Control:**
  * Press **Restart Button (Pin 4):** Instantly restart current game.
  * Press **Menu Button (Pin 5):** Exit back to main selection menu.
