/*+----------------------------------------------------------------------------+
  | RZ-RC-v1.transmitter                                                       |
  +----------------------------------------------------------------------------+
  |       Usage: ---                                                           |
  | Description: https://github.com/WieWaldi/RZ-RC-v1                          |
  |    Requires: ---                                                           |
  |       Notes: ---                                                           |
  |      Author: Waldemar Schroeer                                             |
  |     Company: Rechenzentrum Amper                                           |
  |     Version: 0.1                                                           |
  |     Created: 10.01.2026                                                    |
  |    Revision: ---                                                           |
  |                                                                            |
  | Copyright © 2026 Waldemar Schroeer                                         |
  |                  waldemar.schroeer(at)rz-amper.de                          |
  +----------------------------------------------------------------------------+*/


#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --------------------- Settings ----------------------------------------------
        // Arduino Nano
        #define SCREEN_WIDTH                      128                           // OLED display width, in pixels
        #define SCREEN_HEIGHT                     64                            // OLED display height, in pixels
                                                                                // Define input pins of the rotary encoder
                                                                                // If you use a Rotary Encoder breakout board, leave pin V+ unconnected
        #define CLK_PIN                           3                             // pin D3 connected to rotary encoder CLK (OUT A)
        #define DT_PIN                            4                             // pin D4 connected to rotary encoder DT (OUT B)
        #define SW_PIN                            2                             // pin D2 connected to rotary encoder SW (SWITCH)

        // oLED Display settings
        #define OLED_RESET -1                                                   // Reset pin # (or -1 if sharing Arduino reset pin)
        #define SCREEN_ADDRESS                    0x3C                          // See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
        #define SCREEN_WIDTH                      128                           // OLED display width, in pixels
        #define SCREEN_HEIGHT                     64                            // OLED display height, in pixels
                                                                                // Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
                                                                                // The pins for I2C are defined by the Wire-library.
                                                                                // On an arduino UNO:       A4(SDA), A5(SCL)
                                                                                // On an arduino MEGA 2560: 20(SDA), 21(SCL)
                                                                                // On an arduino LEONARDO:   2(SDA),  3(SCL),
                                                                                // On an arduino Nano:      A4(SDA), A5(SCL),
        // Misc settings
        boolean debug                           = true;                        // Debugging on/off
        boolean serialDebug                     = false;                        // Serial debugging on/off
        const int iLED                          = 2;                            // onboard indicator led gpio pin
        const int menuTimeout                   = 10;                           // menu inactivity timeout (seconds)
        const int topLine                       = 12;                           // y position of lower area of the display (18 with two colour displays)
        #define IntervalScreenUpdate              250                           // Screen update interval in milliseconds
        #define IntervalSensorRead                10                            // Sensor read interval in milliseconds
        #define IntervalRotaryRead                10                            // Rotary encoder read interval in milliseconds
        #define IntervalDebug                     1000                          // Debug output interval in milliseconds
        // Macro for time intervals
        #define every(interval)\
          static uint32_t __every__##interval = millis();\
          if(millis() - __every__##interval >= interval && (__every__##interval = millis()))

        // define modes of movement
        #define DIRECTION_R                       0                             // right turn
        #define DIRECTION_L                       1                             // left turn
        
        // define state variables
        volatile int rotaryCLKstate             = LOW;                          // current state of CLK pin
        volatile int rotaryPrevCLKstate         = LOW;                          // previous state of CLK pin
        volatile int rotaryCounter              = 0;                            // counter for rotary encoder
        volatile int rotaryPosition             = 0;                            // current position of rotary encoder
        volatile int rotaryDirection            = DIRECTION_R;                  // current direction of rotary encoder start with right
        
        volatile int rotaryPrevButtonState      = 0;                            // previous state of BUTTON pin
        volatile int rotaryButtonState          = 0;                            // current state of BUTTON pin
        
        volatile uint32_t lastUpdateScreen      = 0;                            // last screen update time

        volatile int ValForChannel1             = 0;                            // Value for Channel 1
        volatile int ValForChannel2             = 0;                            // Value for Channel 2
        volatile int ValForChannel3             = 0;                            // Value for Channel 3
        volatile int ValForChannel4             = 0;                            // Value for Channel 4
        volatile int ValForAux1                 = 0;                            // Value for Aux 1
        volatile int ValForAux2                 = 0;                            // Value for Aux 2
// -----------------------------------------------------------------------------


Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);       // Create display object


// Modes that the program can be in
enum MenuMode {
  off,
  menu,
  channelInfo,
  auxInfo,
  settings
};
MenuMode menuMode = menu;                                                        // current menu mode




// Forward declarations of functions
void bootScreen();
void debugSerialOutput();
void drawCanvas();
void drawChannelInfo(int channelNumber, int y, int ValForChannel);
void drawFrame();
void menuReset();
void readAnalogValues();
void readRotaryEncoder();
void serviceScreen();
void updateScreen();


void debugSerialOutput() {
  if (debug) {
    every(IntervalDebug)  {                                                     // Output debug information at defined interval
      // If rotary encoder has moved or button state has changed, print debug info
      // if (rotaryCLKstate != rotaryPrevCLKstate || rotaryButtonState != rotaryPrevButtonState) {
        Serial.println("\n\n-- Debug Information -- ");
        Serial.print("Direction:");
        Serial.println(rotaryDirection);
        Serial.print("Position: ");
        Serial.println(rotaryPosition);
        Serial.print("Rotary CLK State:");
        Serial.println(rotaryCLKstate);
        Serial.print("Previous Rotary CLK State:");
        Serial.println(rotaryPrevCLKstate);
        Serial.print("Rotary Button State:");
        Serial.println(rotaryButtonState);
        Serial.print("Previous Rotary Button State:");
        Serial.println(rotaryPrevButtonState);
        Serial.print("Ch1:");
        Serial.println(ValForChannel1);
        Serial.print("Ch2:");
        Serial.println(ValForChannel2);
        Serial.println("-- Debug Information -- ");
      // }
    }
  }
}


void drawFrame() {                                                              // Draw frame around the display
  display.clearDisplay();                                                      // Clear the buffer
  display.drawRect(0, 0, 128, 64, SSD1306_WHITE);                            // Draw outer rectangle 
}

void drawCanvas() {                                                              // 
  display.drawRect(0, 0, 128, 64, SSD1306_WHITE);                            // Draw outer rectangle 
  display.setTextSize(1);                                                       // Text size 1
  display.setCursor(5, 2);                                                   // Cursorposition x, y  
  display.println("RZ-RC-v1 Transmitter");  
  display.drawFastHLine(0,10,128,SSD1306_WHITE);                           // Draw horizontal line
}

void drawChannelInfo(int channelNumber, int y, int ValForChannel) {             // Channel Number, y Position, Value for Channel
  display.setCursor(2, y);                                                      //Cursorposition x, y
  display.print("Ch ");
  display.print(channelNumber);
  display.print(":");
  display.setCursor(31, y);
  display.print(ValForChannel);
  display.drawRect(50, y, 74, 6, SSD1306_WHITE);                                // Draw rectangle for bar
  if (ValForChannel < 127) {                                                    // Draw bar for value
    int barLenght = map(ValForChannel, 0, 127, 35, 0);
    display.fillRect(86, y+2, barLenght, 2, SSD1306_WHITE);
  } else if (ValForChannel > 127) {
    int barLenght = map(ValForChannel, 128, 255, 0, 35);
    display.fillRect(86 - barLenght, y+2, barLenght, 2, SSD1306_WHITE);
  }
}

void readAnalogValues() {
  ValForChannel1 = map( analogRead(A0), 0, 1023, 0, 255 );
  ValForChannel2 = map( analogRead(A1), 0, 1023, 0, 255 );
  ValForChannel3 = map( analogRead(A2), 0, 1023, 0, 255 );
  ValForChannel4 = map( analogRead(A3), 0, 1023, 0, 255 );
  ValForAux1 = map( analogRead(A6), 0, 1023, 0, 255 );
  ValForAux2 = map( analogRead(A7), 0, 1023, 0, 255 );
}

// Function to read the rotary encoder
void readRotaryEncoder() {
  rotaryCLKstate = digitalRead(CLK_PIN);                                        // Read the current state of CLK
  if (rotaryCLKstate != rotaryPrevCLKstate && rotaryCLKstate == HIGH) {         // Has the rotary encoder moved?
    if (digitalRead(DT_PIN) == HIGH) {
      rotaryCounter++;                                                          // Counter-Clockwise
      rotaryPosition++;
      rotaryDirection = DIRECTION_L;
    } else {
      rotaryCounter--;                                                          // Clockwise
      rotaryPosition--;
      rotaryDirection = DIRECTION_R;
    }
    // Serial.print("Direction:");
    // if (rotaryDirection == DIRECTION_R) {
    //   Serial.println("R ");
    // } else {
    //   Serial.println("L ");
    // }
    // Serial.print("Position: ");
    // Serial.println(rotaryCounter);
  }
  rotaryPrevCLKstate = rotaryCLKstate;                                          // Store current state as previous state for next loop

  if (digitalRead(SW_PIN) == LOW) {                                     // Read the current state of BUTTON
    rotaryButtonState = 1;                                                      // Button is PRESSED
  } else {
    rotaryButtonState = 0;                                                      // Button is RELEASED
  }
  // if (rotaryButtonState != rotaryPrevButtonState) {                             // Has the button state changed?
  //   if (rotaryButtonState == 0) {
  //     Serial.println("The button is RELEASED.");
  //     Serial.println(rotaryButtonState);
  //   } else {
  //     Serial.println("The button is PRESSED.");
  //     Serial.println(rotaryButtonState);
  //   }
  // rotaryPrevButtonState = rotaryButtonState;                                    // Store current state as previous state for next loop
  // }
}

void updateScreen(){
  if (menuMode == off) return;                                         // If menu mode is off, exit the function
  if ( (unsigned long)(millis() - lastUpdateScreen) > (menuTimeout * 1000) ) {  // Check for menu timeout and reset if necessary
    menuMode = off;
    return;
  }
  switch (menuMode) {
    case off:
      break;
    case menu:
      display.clearDisplay();                                                      // Clear the buffer
      drawCanvas();

      if (rotaryCounter == 0) {
        display.setTextColor(BLACK,WHITE);                                          // Normal text color
      } else {
        display.setTextColor(WHITE);                                          // Normal text color
      }
      display.setCursor(20, topLine);                                                   // Cursorposition x, y
      display.println("Screen Off");

      if (rotaryCounter == 1) {
        display.setTextColor(BLACK,WHITE);                                          // Normal text color
      } else {
        display.setTextColor(WHITE);                                          // Normal text color
      }
      display.setCursor(20, topLine + 8);                                                   // Cursorposition x, y
      display.println("Channel Info");

      if (rotaryCounter == 2) {
        display.setTextColor(BLACK,WHITE);                                          // Normal text color
      } else {
        display.setTextColor(WHITE);                                          // Normal text color
      }
      display.setCursor(20, topLine + 16);                                                   // Cursorposition x, y
      display.println("Aux Info");

      if (rotaryCounter == 3) {
        display.setTextColor(BLACK,WHITE);                                          // Normal text color
      } else {
        display.setTextColor(WHITE);                                          // Normal text color
      }
      display.setCursor(20, topLine + 24);                                                   // Cursorposition x, y
      display.println("Settings");

      display.setTextColor(WHITE);                                          // Normal text color
      display.setCursor(3, 55);                                                   // Cursorposition x, y
      display.print("Rotary:");
      display.print(rotaryPosition);
      display.print("  B:");
      display.print(rotaryButtonState);
      break;
    case channelInfo:
      drawFrame();
      drawChannelInfo(1, 2, ValForChannel1);
      drawChannelInfo(2, 12, ValForChannel2);
      drawChannelInfo(3, 22, ValForChannel3);
      drawChannelInfo(4, 32, ValForChannel4);
      drawChannelInfo(5, 42, ValForAux1);
      drawChannelInfo(6, 52, ValForAux2);
      display.display();
      break;
    case auxInfo:
      break;
    case settings:
      break;
  }
  display.display();
  lastUpdateScreen = millis();                                                // Update last screen update time
}

void serviceScreen() {
  if (rotaryPosition == 0 && rotaryButtonState == 1) {
    menuMode = off;
  } else if (rotaryPosition == 1 && rotaryButtonState == 1) {
    menuMode = channelInfo;
  } else if (rotaryPosition == 2 && rotaryButtonState == 1) {
    menuMode = auxInfo;
  } else if (rotaryPosition == 3 && rotaryButtonState == 1) {
    menuMode = settings;
  }

  if (menuMode == off && rotaryButtonState == 1) {
    menuMode = menu;
  } else if (menuMode == channelInfo && rotaryButtonState == 1) {
    menuMode = menu;
  } else if (menuMode == auxInfo && rotaryButtonState == 1) {
    menuMode = menu;
  } else if (menuMode == settings && rotaryButtonState == 1) {
    menuMode = menu;
  }
}

void menuReset() {
  rotaryCounter = 0;
  rotaryPosition = 0;
  menuMode = menu;
}

void bootScreen(){
  display.clearDisplay();                                                       // Clear the buffer
  // display.setTextSize(2);                                                       // Text size 2
  display.setCursor(10, 20);                                                    // Cursorposition x, y
  display.println("RZ-RC-v1");
  display.drawRect(0, 0, 128, 64, SSD1306_WHITE);                               // Draw outer rectangle  
  display.drawRect(15, 30, 100, 20, SSD1306_WHITE);                             // Draw inner rectangle
  display.display();

  for (int i = 1; i < 97;)  {
    display.fillRect(17, 32, i, 16, SSD1306_WHITE);                     // Draw loading bar
    display.display();
    i = i + 4;
  }
  delay(2000);
}

// --------------------- void setup() ------------------------------------------
void setup() {
  Serial.begin(115200);
  Serial.println("RZ-RC-v1 Transmitter");
  Serial.println("Initializing Screen");
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("Keinen SSD1306 Verbindung gefunden"));
    while (1) {
      delay(100);
    }
  }
  pinMode(CLK_PIN, INPUT_PULLUP);                                               // pin D3 connected to rotary encoder CLK (OUT A)
  pinMode(DT_PIN, INPUT_PULLUP);                                                // pin D4 connected to rotary encoder DT (OUT B)
  pinMode(SW_PIN, INPUT_PULLUP);                                                // pin D2 connected to rotary encoder SW (SWITCH)

  rotaryPrevCLKstate = digitalRead(CLK_PIN);                                         // Read initial state of CLK
  rotaryPrevButtonState = digitalRead(SW_PIN);                                       // Read initial state of BUTTON

  display.clearDisplay();                                                       // Clear the buffer
  display.setTextSize(2);                                                       // Text size 2
  display.setTextColor(SSD1306_WHITE);                                          // Put white text
  display.cp437(true);                                                          // Use full 256 char 'Code Page 437' font
  display.setCursor(5, 0);                                                      // Cursorposition x, y
  display.print("RZ-RC-v1");
  display.setTextSize(1);                                                       // Text size 1
  display.setCursor(0, 30);                                                     // Cursorposition x, y
  display.println("-- Transmitter --");
  display.print("Debug:");
  display.println( (debug ? "ON" : "OFF") );
  Serial.println("-- Transmitter --");
  Serial.print("Debug:");
  Serial.println( (debug ? "ON" : "OFF") );
  display.println("Setup finished");
  Serial.println("Setup finished");
  display.display();
  bootScreen();
}

// --------------------- void loop() -------------------------------------------
void loop() {
  every(IntervalSensorRead) {                                                   // Read analog values at defined interval
    readAnalogValues();
  }
  every(IntervalRotaryRead)  {                                                  // Read rotary encoder at defined interval
    readRotaryEncoder();                                                        // Read rotary encoder
    serviceScreen();                                                            // Service screen based on rotary encoder input  
  }
  every(IntervalScreenUpdate) {                                                 // Update screen at defined interval
    updateScreen();
  }
  debugSerialOutput();                                                         // Output debug information
}
// --------------------- END ---------------------------------------------------

