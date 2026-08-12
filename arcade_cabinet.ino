#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <LedControl.h>

// -------------------------------------------------------------
// HARDWARE SETUP
// -------------------------------------------------------------
LiquidCrystal_I2C lcd(0x27, 16, 2);

const int NUM_DEVICES = 3;
LedControl lc = LedControl(12, 11, 10, NUM_DEVICES); // DIN=12, CLK=11, CS=10

const int VRX_PIN = A0;
const int VRY_PIN = A1;
const int SW_PIN  = 2;         
const int BUZZER_PIN = 3;     
const int RESTART_BTN_PIN = 4; 
const int MENU_BTN_PIN    = 5; 

const bool REVERSE_DEVICE_ORDER = true; 
const int DISPLAY_ROTATION = 90; 

enum State { STATE_MENU, STATE_SNAKE, STATE_TETRIS };
State currentState = STATE_MENU;
int selectedGame = 1; 
int score = 0;

unsigned long lastMoveTime = 0;
unsigned long lastTetrisInputTime = 0;
const unsigned long TETRIS_INPUT_DELAY = 120;

unsigned long swPressStartTime = 0;
bool swIsPressed = false;
bool swActionHandled = false;

unsigned long lastRestartDebounce = 0;
unsigned long lastMenuDebounce    = 0;
const unsigned long DEBOUNCE_DELAY  = 150;

// Food pulse animation timing
unsigned long lastPulseTime = 0;
bool foodState = true;

// Forward Declarations
void drawMenu();
void updateLCDMenu();
void updateLCDScore();
void resetSnake();
void resetTetris();

// -------------------------------------------------------------
// SOUND SYSTEM
// -------------------------------------------------------------
void playTone(int frequency, int duration) {
  tone(BUZZER_PIN, frequency, duration);
}

void playSelectSound()     { playTone(800, 50); }
void playRotateSound()     { playTone(950, 25); }
void playEatSound()        { playTone(1046, 60); }
void playStartSound()      { playTone(523, 80); delay(90); playTone(659, 80); delay(90); playTone(784, 120); }
void playLineClearSound()  { playTone(880, 70); delay(80); playTone(1175, 120); }
void playGameOverSound()   { playTone(300, 150); delay(160); playTone(200, 150); delay(160); playTone(130, 300); }
void playReturnMenuSound() { playTone(600, 80); delay(90); playTone(400, 80); }

// -------------------------------------------------------------
// MATRIX DRAWING & VISUAL EFFECTS
// -------------------------------------------------------------
void drawPixel(int x, int y, bool state) {
  if (x < 0 || x >= 24 || y < 0 || y >= 8) return;

  int deviceIndex = x / 8;
  int localCol = x % 8;
  int localRow = y;

  if (REVERSE_DEVICE_ORDER) {
    deviceIndex = (NUM_DEVICES - 1) - deviceIndex;
  }

  int finalRow = localRow;
  int finalCol = localCol;

  if (DISPLAY_ROTATION == 90) {
    finalRow = localCol;
    finalCol = 7 - localRow;
  } else if (DISPLAY_ROTATION == 180) {
    finalRow = 7 - localRow;
    finalCol = 7 - localCol;
  } else if (DISPLAY_ROTATION == 270) {
    finalRow = 7 - localCol;
    finalCol = localRow;
  }

  lc.setLed(deviceIndex, finalRow, finalCol, state);
}

void clearAllDisplays() {
  for (int dev = 0; dev < NUM_DEVICES; dev++) {
    lc.clearDisplay(dev);
  }
}

// Startup Sweep Effect
void playStartupAnimation() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("  ARCADE SYSTEM ");
  lcd.setCursor(0, 1);
  lcd.print("  INITIALIZING ");

  // Fill matrix left to right
  for (int x = 0; x < 24; x++) {
    for (int y = 0; y < 8; y++) {
      drawPixel(x, y, true);
    }
    if (x % 6 == 0) lcd.print(".");
    delay(20);
  }

  playStartSound();

  // Clear matrix right to left
  for (int x = 23; x >= 0; x--) {
    for (int y = 0; y < 8; y++) {
      drawPixel(x, y, false);
    }
    delay(20);
  }
  delay(200);
}

// Game Over Screen Shake Effect
void playScreenShakeEffect() {
  for (int shake = 0; shake < 4; shake++) {
    // Offset left
    for (int dev = 0; dev < NUM_DEVICES; dev++) {
      lc.setIntensity(dev, 12);
    }
    delay(40);
    // Offset back
    for (int dev = 0; dev < NUM_DEVICES; dev++) {
      lc.setIntensity(dev, 4);
    }
    delay(40);
  }
}

// Line Clear Flash Effect
void playFlashEffect() {
  for (int dev = 0; dev < NUM_DEVICES; dev++) {
    lc.setIntensity(dev, 15);
  }
  delay(60);
  for (int dev = 0; dev < NUM_DEVICES; dev++) {
    lc.setIntensity(dev, 4);
  }
}

// -------------------------------------------------------------
// LCD DISPLAY FUNCTIONS
// -------------------------------------------------------------
void updateLCDMenu() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("== SELECT GAME ==");
  lcd.setCursor(0, 1);
  if (selectedGame == 1) {
    lcd.print("> 1. SNAKE      ");
  } else {
    lcd.print("> 2. TETRIS     ");
  }
}

void updateLCDScore() {
  lcd.setCursor(0, 1);
  lcd.print("Score: ");
  lcd.print(score);
  lcd.print("       ");
}

void displayLCDGameOver() {
  playScreenShakeEffect();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("   GAME OVER!   ");
  lcd.setCursor(0, 1);
  lcd.print("Final Score: ");
  lcd.print(score);
}

// -------------------------------------------------------------
// NAVIGATION & BUTTON HANDLERS
// -------------------------------------------------------------
void backToMenu() {
  playReturnMenuSound();
  currentState = STATE_MENU;
  drawMenu();
  updateLCDMenu();
}

void restartGame() {
  playStartSound();
  score = 0;
  delay(200);
  
  lcd.clear();
  lcd.setCursor(0, 0);
  
  if (currentState == STATE_SNAKE || (currentState == STATE_MENU && selectedGame == 1)) {
    currentState = STATE_SNAKE;
    lcd.print("Playing: SNAKE  ");
    updateLCDScore();
    resetSnake();
  } else if (currentState == STATE_TETRIS || (currentState == STATE_MENU && selectedGame == 2)) {
    currentState = STATE_TETRIS;
    lcd.print("Playing: TETRIS ");
    updateLCDScore();
    resetTetris();
  }
}

void checkExternalButtons() {
  unsigned long now = millis();

  if (digitalRead(RESTART_BTN_PIN) == LOW && (now - lastRestartDebounce > DEBOUNCE_DELAY)) {
    lastRestartDebounce = now;
    restartGame();
  }

  if (digitalRead(MENU_BTN_PIN) == LOW && (now - lastMenuDebounce > DEBOUNCE_DELAY)) {
    lastMenuDebounce = now;
    backToMenu();
  }
}

bool checkCollision(int nextX, int nextY, int nextRot);
extern int pieceX, pieceY, pieceType, pieceRotation;

void handleJoystickButton() {
  int swState = digitalRead(SW_PIN);

  if (swState == LOW && !swIsPressed) {
    swIsPressed = true;
    swPressStartTime = millis();
    swActionHandled = false;
  }

  if (swState == LOW && swIsPressed && !swActionHandled) {
    if (millis() - swPressStartTime >= 1500) {
      swActionHandled = true;
      backToMenu();
      return;
    }
  }

  if (swState == HIGH && swIsPressed) {
    swIsPressed = false;

    if (!swActionHandled && (millis() - swPressStartTime < 400)) {
      if (currentState == STATE_TETRIS) {
        int nextRot = (pieceRotation + 1) % 4;
        if (!checkCollision(pieceX, pieceY, nextRot)) {
          pieceRotation = nextRot;
          playRotateSound();
        }
      }
    }
  }
}

// -------------------------------------------------------------
// GAME 1: SNAKE
// -------------------------------------------------------------
struct Point { int x; int y; };
Point snake[192];
int snakeLength = 4;
enum Direction { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT };
Direction snakeDir = DIR_RIGHT;
Point food;
bool snakeGameOver = false;

int baseSnakeSpeed = 180;
int currentSnakeSpeed = 180;

void generateFood() {
  bool inSnake;
  do {
    inSnake = false;
    food.x = random(0, 24);
    food.y = random(0, 8);
    for (int i = 0; i < snakeLength; i++) {
      if (snake[i].x == food.x && snake[i].y == food.y) {
        inSnake = true;
        break;
      }
    }
  } while (inSnake);
}

void resetSnake() {
  snakeLength = 4;
  snake[0] = {3, 3};
  snake[1] = {2, 3};
  snake[2] = {1, 3};
  snake[3] = {0, 3};
  snakeDir = DIR_RIGHT;
  snakeGameOver = false;
  currentSnakeSpeed = baseSnakeSpeed;
  generateFood();
}

void updateSnake() {
  handleJoystickButton();
  if (currentState != STATE_SNAKE) return;

  int xVal = analogRead(VRX_PIN);
  int yVal = analogRead(VRY_PIN);

  if (xVal < 300 && snakeDir != DIR_RIGHT) snakeDir = DIR_LEFT;
  else if (xVal > 700 && snakeDir != DIR_LEFT) snakeDir = DIR_RIGHT;
  else if (yVal < 300 && snakeDir != DIR_DOWN) snakeDir = DIR_UP;
  else if (yVal > 700 && snakeDir != DIR_UP) snakeDir = DIR_DOWN;

  // Pulse effect for food
  if (millis() - lastPulseTime > 150) {
    foodState = !foodState;
    lastPulseTime = millis();
    drawPixel(food.x, food.y, foodState);
  }

  if (millis() - lastMoveTime >= currentSnakeSpeed) {
    Point newHead = snake[0];
    switch (snakeDir) {
      case DIR_UP:    newHead.y--; break;
      case DIR_DOWN:  newHead.y++; break;
      case DIR_LEFT:  newHead.x--; break;
      case DIR_RIGHT: newHead.x++; break;
    }

    if (newHead.x < 0 || newHead.x >= 24 || newHead.y < 0 || newHead.y >= 8) {
      snakeGameOver = true;
    }

    for (int i = 0; i < snakeLength; i++) {
      if (snake[i].x == newHead.x && snake[i].y == newHead.y) {
        snakeGameOver = true;
      }
    }

    if (snakeGameOver) {
      playGameOverSound();
      displayLCDGameOver();
      delay(1500);
      backToMenu();
      return;
    }

    if (newHead.x == food.x && newHead.y == food.y) {
      snakeLength++;
      score += 10;
      updateLCDScore();
      playEatSound();
      
      if (currentSnakeSpeed > 60) {
        currentSnakeSpeed -= 4;
      }
      
      generateFood();
    }

    for (int i = snakeLength - 1; i > 0; i--) {
      snake[i] = snake[i - 1];
    }
    snake[0] = newHead;

    clearAllDisplays();
    for (int i = 0; i < snakeLength; i++) drawPixel(snake[i].x, snake[i].y, true);
    drawPixel(food.x, food.y, foodState);

    lastMoveTime = millis();
  }
}

// -------------------------------------------------------------
// GAME 2: HORIZONTAL TETRIS
// -------------------------------------------------------------
bool board[24][8]; 
int pieceX = 0, pieceY = 3;
int pieceType = 0; 
int pieceRotation = 0; 
int fallSpeed = 300;

void getPieceOffsets(int type, int rot, int offsets[4][2], int &pixelCount) {
  for (int i = 0; i < 4; i++) { offsets[i][0] = 0; offsets[i][1] = 0; }

  switch (type) {
    case 0: // Square
      pixelCount = 4;
      offsets[0][0] = 0; offsets[0][1] = 0; offsets[1][0] = 1; offsets[1][1] = 0;
      offsets[2][0] = 0; offsets[2][1] = 1; offsets[3][0] = 1; offsets[3][1] = 1;
      break;

    case 1: // 2-Pixel Line
      pixelCount = 2;
      if (rot % 2 == 0) {
        offsets[0][0] = 0; offsets[0][1] = 0; offsets[1][0] = 1; offsets[1][1] = 0;
      } else {
        offsets[0][0] = 0; offsets[0][1] = 0; offsets[1][0] = 0; offsets[1][1] = 1;
      }
      break;

    case 2: // L-Shape
      pixelCount = 3;
      if (rot == 0) {
        offsets[0][0] = 0; offsets[0][1] = 0; offsets[1][0] = 1; offsets[1][1] = 0; offsets[2][0] = 0; offsets[2][1] = 1;
      } else if (rot == 1) {
        offsets[0][0] = 0; offsets[0][1] = 0; offsets[1][0] = 0; offsets[1][1] = 1; offsets[2][0] = 1; offsets[2][1] = 1;
      } else if (rot == 2) {
        offsets[0][0] = 1; offsets[0][1] = 0; offsets[1][0] = 0; offsets[1][1] = 1; offsets[2][0] = 1; offsets[2][1] = 1;
      } else {
        offsets[0][0] = 0; offsets[0][1] = 0; offsets[1][0] = 1; offsets[1][0] = 1; offsets[2][0] = 1; offsets[2][1] = 1;
      }
      break;

    case 3: // Inverted L-Shape
      pixelCount = 3;
      if (rot == 0) {
        offsets[0][0] = 0; offsets[0][1] = 0; offsets[1][0] = 1; offsets[1][1] = 0; offsets[2][0] = 1; offsets[2][1] = 1;
      } else if (rot == 1) {
        offsets[0][0] = 1; offsets[0][1] = 0; offsets[1][0] = 0; offsets[1][1] = 1; offsets[2][0] = 1; offsets[2][1] = 1;
      } else if (rot == 2) {
        offsets[0][0] = 0; offsets[0][1] = 0; offsets[1][0] = 0; offsets[1][1] = 1; offsets[2][0] = 1; offsets[2][1] = 0;
      } else {
        offsets[0][0] = 0; offsets[0][1] = 0; offsets[1][0] = 1; offsets[1][1] = 0; offsets[2][0] = 0; offsets[2][1] = 1;
      }
      break;

    case 4: // T-Shape
      pixelCount = 4;
      if (rot == 0) {
        offsets[0][0] = 0; offsets[0][1] = 0; offsets[1][0] = 1; offsets[1][1] = 0; offsets[2][0] = 2; offsets[2][1] = 0; offsets[3][0] = 1; offsets[3][1] = 1;
      } else if (rot == 1) {
        offsets[0][0] = 1; offsets[0][1] = 0; offsets[1][0] = 0; offsets[1][1] = 1; offsets[2][0] = 1; offsets[2][1] = 1; offsets[3][0] = 1; offsets[3][1] = 2;
      } else if (rot == 2) {
        offsets[0][0] = 1; offsets[0][1] = 0; offsets[1][0] = 0; offsets[1][1] = 1; offsets[2][0] = 1; offsets[2][1] = 1; offsets[3][0] = 2; offsets[3][1] = 1;
      } else {
        offsets[0][0] = 0; offsets[0][1] = 0; offsets[1][0] = 0; offsets[1][1] = 1; offsets[2][0] = 1; offsets[2][1] = 1; offsets[3][0] = 0; offsets[3][1] = 2;
      }
      break;

    case 5: // Long Line
      pixelCount = 3;
      if (rot % 2 == 0) {
        offsets[0][0] = 0; offsets[0][1] = 0; offsets[1][0] = 1; offsets[1][1] = 0; offsets[2][0] = 2; offsets[2][1] = 0;
      } else {
        offsets[0][0] = 0; offsets[0][1] = 0; offsets[1][0] = 0; offsets[1][1] = 1; offsets[2][0] = 0; offsets[2][1] = 2;
      }
      break;

    case 6: // Single Dot
      pixelCount = 1;
      offsets[0][0] = 0; offsets[0][1] = 0;
      break;
  }
}

void spawnTetrisPiece() {
  pieceX = 0;
  pieceY = 3;
  pieceType = random(0, 7);
  pieceRotation = 0;

  int offsets[4][2];
  int count;
  getPieceOffsets(pieceType, pieceRotation, offsets, count);

  for (int i = 0; i < count; i++) {
    if (board[pieceX + offsets[i][0]][pieceY + offsets[i][1]]) {
      playGameOverSound();
      displayLCDGameOver();
      delay(1500);
      backToMenu();
      return;
    }
  }
}

void resetTetris() {
  for (int x = 0; x < 24; x++) {
    for (int y = 0; y < 8; y++) {
      board[x][y] = false;
    }
  }
  spawnTetrisPiece();
}

bool checkCollision(int nextX, int nextY, int nextRot) {
  int offsets[4][2];
  int count;
  getPieceOffsets(pieceType, nextRot, offsets, count);

  for (int i = 0; i < count; i++) {
    int px = nextX + offsets[i][0];
    int py = nextY + offsets[i][1];

    if (px < 0 || px >= 24 || py < 0 || py >= 8) return true;
    if (board[px][py]) return true;
  }
  return false;
}

void lockPiece() {
  int offsets[4][2];
  int count;
  getPieceOffsets(pieceType, pieceRotation, offsets, count);

  for (int i = 0; i < count; i++) {
    board[pieceX + offsets[i][0]][pieceY + offsets[i][1]] = true;
  }

  playTone(400, 30);

  for (int x = 23; x >= 0; x--) {
    bool full = true;
    for (int y = 0; y < 8; y++) {
      if (!board[x][y]) { full = false; break; }
    }
    if (full) {
      playFlashEffect(); // Flash animation on line clear
      playLineClearSound();
      score += 50;
      updateLCDScore();
      
      for (int shiftX = x; shiftX > 0; shiftX--) {
        for (int y = 0; y < 8; y++) {
          board[shiftX][y] = board[shiftX - 1][y];
        }
      }
      for (int y = 0; y < 8; y++) board[0][y] = false;
      x++;
    }
  }

  spawnTetrisPiece();
}

void updateTetris() {
  handleJoystickButton();
  if (currentState != STATE_TETRIS) return;

  if (millis() - lastTetrisInputTime >= TETRIS_INPUT_DELAY) {
    int xVal = analogRead(VRX_PIN);
    int yVal = analogRead(VRY_PIN);

    bool moved = false;
    if (yVal < 300 && !checkCollision(pieceX, pieceY - 1, pieceRotation)) { pieceY--; moved = true; }
    else if (yVal > 700 && !checkCollision(pieceX, pieceY + 1, pieceRotation)) { pieceY++; moved = true; }
    if (xVal > 700 && !checkCollision(pieceX + 1, pieceY, pieceRotation)) { pieceX++; moved = true; }

    if (moved) {
      lastTetrisInputTime = millis();
    }
  }

  if (millis() - lastMoveTime >= fallSpeed) {
    if (!checkCollision(pieceX + 1, pieceY, pieceRotation)) {
      pieceX++;
    } else {
      lockPiece();
    }

    clearAllDisplays();

    for (int x = 0; x < 24; x++) {
      for (int y = 0; y < 8; y++) {
        if (board[x][y]) drawPixel(x, y, true);
      }
    }

    int offsets[4][2];
    int count;
    getPieceOffsets(pieceType, pieceRotation, offsets, count);
    for (int i = 0; i < count; i++) {
      drawPixel(pieceX + offsets[i][0], pieceY + offsets[i][1], true);
    }

    lastMoveTime = millis();
  }
}

// -------------------------------------------------------------
// MENU SYSTEM
// -------------------------------------------------------------
void drawMenu() {
  clearAllDisplays();
  
  if (selectedGame == 1) {
    // Upright Vertical Digit '1'
    drawPixel(9, 4, true);  drawPixel(9, 3, true);
    drawPixel(10, 3, true); drawPixel(11, 3, true);
    drawPixel(12, 3, true); drawPixel(13, 3, true); drawPixel(14, 3, true);
    drawPixel(14, 2, true); drawPixel(14, 4, true);
  } else if (selectedGame == 2) {
    // Upright Vertical Digit '2'
    drawPixel(9, 2, true);  drawPixel(9, 3, true);  drawPixel(9, 4, true);
    drawPixel(10, 2, true);
    drawPixel(11, 2, true); drawPixel(11, 3, true); drawPixel(11, 4, true);
    drawPixel(12, 4, true); drawPixel(13, 4, true);
    drawPixel(14, 2, true); drawPixel(14, 3, true); drawPixel(14, 4, true);
  }
}

void handleMenu() {
  int xVal = analogRead(VRX_PIN);
  
  if (xVal < 300 && selectedGame > 1) { 
    selectedGame--; 
    playSelectSound(); 
    drawMenu(); 
    updateLCDMenu();
    delay(250); 
  }
  if (xVal > 700 && selectedGame < 2) { 
    selectedGame++; 
    playSelectSound(); 
    drawMenu(); 
    updateLCDMenu();
    delay(250); 
  }

  if (digitalRead(SW_PIN) == LOW) {
    restartGame();
  }
}

// -------------------------------------------------------------
// SETUP & LOOP
// -------------------------------------------------------------
void setup() {
  Wire.begin();
  
  lcd.init();
  lcd.clear();
  lcd.backlight();

  for (int dev = 0; dev < NUM_DEVICES; dev++) {
    lc.shutdown(dev, false);
    lc.setIntensity(dev, 4);
    lc.clearDisplay(dev);
  }

  pinMode(VRX_PIN, INPUT);
  pinMode(VRY_PIN, INPUT);
  pinMode(SW_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RESTART_BTN_PIN, INPUT_PULLUP);
  pinMode(MENU_BTN_PIN, INPUT_PULLUP);

  randomSeed(analogRead(A2));
  
  // Play startup wipe animation
  playStartupAnimation();

  drawMenu();
  updateLCDMenu();
}

void loop() {
  checkExternalButtons();

  switch (currentState) {
    case STATE_MENU:
      handleMenu();
      break;
    case STATE_SNAKE:
      updateSnake();
      break;
    case STATE_TETRIS:
      updateTetris();
      break;
  }
}
