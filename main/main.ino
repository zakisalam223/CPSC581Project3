
#include <LiquidCrystal.h>

// ==========================================
// PIN DEFINITIONS
// ==========================================

// RGB LED 1 (Side 1)
const byte RED_PIN_1 = 11;
const byte GREEN_PIN_1 = 10;
const byte BLUE_PIN_1 = 9;

// RGB LED 2 (Side 2)
const byte RED_PIN_2 = 6;
const byte GREEN_PIN_2 = 5;
const byte BLUE_PIN_2 = 3;

// Buttons
const byte BEEF_BUTTON = 13;
const byte CHICKEN_BUTTON = 12;
const byte START_BUTTON = 8;

// Joystick
const byte JOY_X = A0;
const byte JOY_Y = A1;

// LCD
const byte RS_PIN = 2;
const byte ENABLE_PIN = 4;
const byte DATA_PIN_1 = A2;
const byte DATA_PIN_2 = A3;
const byte DATA_PIN_3 = A4;
const byte DATA_PIN_4 = A5;


// ==========================================
// LCD INITIALIZATION
// ==========================================

LiquidCrystal lcd(
  RS_PIN,
  ENABLE_PIN,
  DATA_PIN_1,
  DATA_PIN_2,
  DATA_PIN_3,
  DATA_PIN_4
);


// ==========================================
// TIMER DURATIONS
// ==========================================

const unsigned long BEEF_DURATION = 30000UL;
const unsigned long CHICKEN_DURATION = 60000UL;

const unsigned long SLOW_BLINK = 1000;
const unsigned long FAST_BLINK = 300;


// ==========================================
// JOYSTICK THRESHOLDS
// ==========================================

const int JOY_LEFT_THRESHOLD = 350;
const int JOY_RIGHT_THRESHOLD = 673;


// ==========================================
// GRILL SIDE SELECTION
// ==========================================

bool grill_side_1 = true;
bool grill_side_2 = false;


// ==========================================
// MEAT SELECTION
// ==========================================

enum MeatType {
  NONE,
  BEEF,
  CHICKEN
};


// ==========================================
// INDEPENDENT SIDE TIMERS
// ==========================================

struct SideTimer {

  MeatType selected_meat;

  unsigned long timerDuration;
  unsigned long timerStartTime;
  unsigned long remainingTime;

  unsigned long previousBlinkTime;

  bool timerRunning;
  bool timerFinished;
  bool ledOn;
};

// Create independent timers for both sides
SideTimer side1 = {
  NONE, 0, 0, 0, 0, false, false, true
};

SideTimer side2 = {
  NONE, 0, 0, 0, 0, false, false, true
};


// ==========================================
// BUTTON DEBOUNCE
// ==========================================

const unsigned long DEBOUNCE_DELAY = 50;

struct Button {
  byte pin;
  bool lastReading;
  bool stableState;
  unsigned long lastChangeTime;
};

// External pull-down resistors:
// LOW = not pressed
// HIGH = pressed

Button beefBtn = {BEEF_BUTTON, LOW, LOW, 0};
Button chickenBtn = {CHICKEN_BUTTON, LOW, LOW, 0};
Button startBtn = {START_BUTTON, LOW, LOW, 0};


// Returns true once when a button is pressed
bool buttonPressed(Button &button) {

  bool reading = digitalRead(button.pin);

  if (reading != button.lastReading) {
    button.lastChangeTime = millis();
    button.lastReading = reading;
  }

  if (millis() - button.lastChangeTime >= DEBOUNCE_DELAY) {

    if (reading != button.stableState) {

      button.stableState = reading;

      // HIGH means the button was pressed
      if (button.stableState == HIGH) {
        return true;
      }
    }
  }

  return false;
}


// ==========================================
// RGB LED FUNCTION
// Common Cathode
// ==========================================

void setColor(bool red, bool green, bool blue,
              byte red_pin, byte green_pin, byte blue_pin) {

  digitalWrite(red_pin, red ? HIGH : LOW);
  digitalWrite(green_pin, green ? HIGH : LOW);
  digitalWrite(blue_pin, blue ? HIGH : LOW);
}


// ==========================================
// UPDATE INDIVIDUAL SIDE LED
// ==========================================

void updateSideLED(SideTimer &side, bool isSide1) {

  byte redPin;
  byte greenPin;
  byte bluePin;

  if (isSide1) {
    redPin = RED_PIN_1;
    greenPin = GREEN_PIN_1;
    bluePin = BLUE_PIN_1;
  }
  else {
    redPin = RED_PIN_2;
    greenPin = GREEN_PIN_2;
    bluePin = BLUE_PIN_2;
  }

  // Finished: solid green
  if (side.timerFinished) {

    setColor(false, true, false,
             redPin, greenPin, bluePin);
  }

  // Currently cooking: Leave its blinking LED unchanged
  else if (side.timerRunning) {
    return;
  }

  // Joystick selected side: solid white
  else if ((isSide1 && grill_side_1) || (!isSide1 && grill_side_2)) {
    setColor(true, true, true, redPin, greenPin, bluePin);
  }

  // Not selected: LED off
  else {
    setColor(false, false, false,
             redPin, greenPin, bluePin);
  }
}


// ==========================================
// START INDIVIDUAL SIDE TIMER
// ==========================================

void startTimer(SideTimer &side, bool isSide1) {

  if (side.selected_meat == NONE) {

    Serial.println("Please select meat first!");
    return;
  }

  if (side.timerRunning) {

    Serial.println("This side is already cooking!");
    return;
  }

  side.timerRunning = true;
  side.timerFinished = false;

  side.timerStartTime = millis();

  side.previousBlinkTime = millis();

  side.remainingTime = side.timerDuration;

  side.ledOn = true;

  // Start with red LED
  byte redPin = isSide1 ? RED_PIN_1 : RED_PIN_2;
  byte greenPin = isSide1 ? GREEN_PIN_1 : GREEN_PIN_2;
  byte bluePin = isSide1 ? BLUE_PIN_1 : BLUE_PIN_2;

  setColor(true, false, false,
           redPin, greenPin, bluePin);

  Serial.print("Side ");
  Serial.print(isSide1 ? "1" : "2");
  Serial.println(" timer started!");
}


// ==========================================
// UPDATE INDIVIDUAL COUNTDOWN
// ==========================================

void updateSideTimer(SideTimer &side, bool isSide1) {

  // Timer is not running
  if (!side.timerRunning) {

    updateSideLED(side, isSide1);
    return;
  }

  // Calculate elapsed time
  unsigned long elapsedTime = millis() - side.timerStartTime;

  // Check if timer has finished
  if (elapsedTime >= side.timerDuration) {

    side.timerRunning = false;
    side.timerFinished = true;

    side.remainingTime = 0;

    updateSideLED(side, isSide1);

    Serial.print("Side ");
    Serial.print(isSide1 ? "1" : "2");
    Serial.println(" cooking finished!");

    return;
  }

  // Calculate remaining time
  side.remainingTime = side.timerDuration - elapsedTime;

  // Determine halfway and final 10 seconds
  bool isHalfway = elapsedTime >= side.timerDuration / 2;

  bool isFinalTenSeconds = side.remainingTime <= 10000UL;

  // Determine LED colour
  bool red = true;
  bool green = false;

  if (isHalfway) {
    red = true;
    green = true;
  }

  // Determine blink speed
  unsigned long blinkInterval = SLOW_BLINK;

  if (isFinalTenSeconds) {
    blinkInterval = FAST_BLINK;
  }

  // Blink LED without delay()
  if (millis() - side.previousBlinkTime >= blinkInterval) {

    side.previousBlinkTime = millis();

    side.ledOn = !side.ledOn;
  }

  // Set LED colour
  byte redPin = isSide1 ? RED_PIN_1 : RED_PIN_2;
  byte greenPin = isSide1 ? GREEN_PIN_1 : GREEN_PIN_2;
  byte bluePin = isSide1 ? BLUE_PIN_1 : BLUE_PIN_2;

  if (side.ledOn) {

    setColor(red, green, false,
             redPin, greenPin, bluePin);
  }
  else {

    setColor(false, false, false,
             redPin, greenPin, bluePin);
  }
}


// ==========================================
// UPDATE BOTH TIMERS
// ==========================================

void updateAllTimers() {

  updateSideTimer(side1, true);
  updateSideTimer(side2, false);
}


// ==========================================
// DISPLAY ONE SIDE ON LCD
// ==========================================

void displaySideRow(SideTimer &side, byte sideNumber, byte row) {

  lcd.setCursor(0, row);

  int characters = 0;

  // Show which side is currently selected
  if ((sideNumber == 1 && grill_side_1) ||
      (sideNumber == 2 && grill_side_2)) {

    lcd.print(">");
  }
  else {
    lcd.print(" ");
  }

  characters++;

  lcd.print(sideNumber);
  characters++;

  lcd.print(" ");
  characters++;

  // Display selected meat
  if (side.selected_meat == NONE) {

    lcd.print("Empty");
    characters += 5;
  }

  else if (side.selected_meat == BEEF) {

    lcd.print("Beef");
    characters += 4;
  }

  else if (side.selected_meat == CHICKEN) {

    lcd.print("Chicken");
    characters += 7;
  }

  lcd.print(" ");
  characters++;

  // Display timer status
  if (side.timerFinished) {

    lcd.print("DONE");
    characters += 4;
  }

  else if (side.selected_meat != NONE) {

    unsigned long seconds;

    if (side.timerRunning) {
      seconds = (side.remainingTime + 999) / 1000;
    }
    else {
      seconds = side.timerDuration / 1000;
    }

    lcd.print(seconds);
    lcd.print("s");

    // Count digits plus the 's'
    if (seconds >= 100) {
      characters += 4;
    }
    else if (seconds >= 10) {
      characters += 3;
    }
    else {
      characters += 2;
    }
  }

  else {

    lcd.print("---");
    characters += 3;
  }

  // Clear remaining LCD spaces
  while (characters < 16) {

    lcd.print(" ");
    characters++;
  }
}


// ==========================================
// UPDATE LCD
// ==========================================

void updateLCD() {

  displaySideRow(side1, 1, 0);
  displaySideRow(side2, 2, 1);
}


// ==========================================
// SETUP
// ==========================================

void setup() {

  // LED pins
  pinMode(RED_PIN_1, OUTPUT);
  pinMode(GREEN_PIN_1, OUTPUT);
  pinMode(BLUE_PIN_1, OUTPUT);

  pinMode(RED_PIN_2, OUTPUT);
  pinMode(GREEN_PIN_2, OUTPUT);
  pinMode(BLUE_PIN_2, OUTPUT);

  // Button pins
  pinMode(BEEF_BUTTON, INPUT);
  pinMode(CHICKEN_BUTTON, INPUT);
  pinMode(START_BUTTON, INPUT);

  // Joystick pins
  pinMode(JOY_X, INPUT);
  pinMode(JOY_Y, INPUT);

  // LCD
  lcd.begin(16, 2);

  Serial.begin(9600);
  Serial.println("KBBQ Timer Ready!");

  updateAllTimers();
  updateLCD();
}


// ==========================================
// MAIN LOOP
// ==========================================

void loop() {

  bool selectionChanged = false;


  // ----------------------------------------
  // 1. JOYSTICK SELECTION
  // ----------------------------------------

  int joystickX = analogRead(JOY_X);

  // Side selection is allowed even while
  // the other side is cooking

  if (joystickX < JOY_LEFT_THRESHOLD) {

    if (!grill_side_1) {

      grill_side_1 = true;
      grill_side_2 = false;

      selectionChanged = true;

      Serial.println("Side 1 selected");
    }
  }

  else if (joystickX > JOY_RIGHT_THRESHOLD) {

    if (!grill_side_2) {

      grill_side_1 = false;
      grill_side_2 = true;

      selectionChanged = true;

      Serial.println("Side 2 selected");
    }
  }


  // ----------------------------------------
  // 2. DETERMINE CURRENT SIDE
  // ----------------------------------------

  SideTimer &selectedSide = grill_side_1 ? side1 : side2;


  // ----------------------------------------
  // 3. BEEF BUTTON
  // ----------------------------------------

  if (buttonPressed(beefBtn)) {

    if (!selectedSide.timerRunning) {

      selectedSide.selected_meat = BEEF;
      selectedSide.timerDuration = BEEF_DURATION;
      selectedSide.remainingTime = BEEF_DURATION;

      selectedSide.timerFinished = false;

      updateSideLED(selectedSide, grill_side_1);

      selectionChanged = true;

      Serial.print("Beef selected for Side ");
      Serial.println(grill_side_1 ? "1" : "2");
    }

    else {
      Serial.println("This side is already cooking!");
    }
  }


  // ----------------------------------------
  // 4. CHICKEN BUTTON
  // ----------------------------------------

  if (buttonPressed(chickenBtn)) {

    if (!selectedSide.timerRunning) {

      selectedSide.selected_meat = CHICKEN;
      selectedSide.timerDuration = CHICKEN_DURATION;
      selectedSide.remainingTime = CHICKEN_DURATION;

      selectedSide.timerFinished = false;

      updateSideLED(selectedSide, grill_side_1);

      selectionChanged = true;

      Serial.print("Chicken selected for Side ");
      Serial.println(grill_side_1 ? "1" : "2");
    }

    else {
      Serial.println("This side is already cooking!");
    }
  }


  // ----------------------------------------
  // 5. START BUTTON
  // ----------------------------------------

  if (buttonPressed(startBtn)) {

    startTimer(selectedSide, grill_side_1);

    selectionChanged = true;
  }


  // ----------------------------------------
  // 6. UPDATE BOTH TIMERS
  // ----------------------------------------

  updateAllTimers();


  // ----------------------------------------
  // 7. UPDATE LCD
  // ----------------------------------------

  static unsigned long previousDisplayTime = 0;

  if (selectionChanged ||
      millis() - previousDisplayTime >= 250) {

    previousDisplayTime = millis();

    updateLCD();
  }

}
