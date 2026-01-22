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
        // Define input pins of the rotary encoder
        // If you use a Rotary Encoder breakout board, leave pin V+ unconnected
        #define SCREEN_WIDTH 128                                                // OLED display width, in pixels
        #define SCREEN_HEIGHT 64                                                // OLED display height, in pixels
        // Define input pins of the rotary encoder
        // If you use a Rotary Encoder breakout board, leave pin V+ unconnected
        #define CLK_PIN 3                                                       // pin D3 connected to rotary encoder CLK (OUT A)
        #define DT_PIN 4                                                        // pin D4 connected to rotary encoder DT (OUT B)
        #define SW_PIN 2                                                        // pin D2 connected to rotary encoder SW (SWITCH)

        // oLED Display settings
        #define OLED_RESET -1                                                   // Reset pin # (or -1 if sharing Arduino reset pin)
        #define SCREEN_ADDRESS 0x3C                                             // See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
        #define SCREEN_WIDTH 128                                                // OLED display width, in pixels
        #define SCREEN_HEIGHT 64                                                // OLED display height, in pixels
        // Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
        // The pins for I2C are defined by the Wire-library.
        // On an arduino UNO:       A4(SDA), A5(SCL)
        // On an arduino MEGA 2560: 20(SDA), 21(SCL)
        // On an arduino LEONARDO:   2(SDA),  3(SCL),
        // On an arduino Nano:      A4(SDA), A5(SCL),

        // Misc settings
        boolean debug = false;                                                  // Debugging on/off
        boolean serialDebug = false;                                            // Serial debugging on/off
        const int iLED= 2;                                                      // onboard indicator led gpio pin
        const int menuTimeout = 10;                                             // menu inactivity timeout (seconds)
        const bool menuLargeText = 0;                                           // show larger text when possible (if struggling to read the small text)
        const int maxmenuItems = 12;                                            // max number of items used in any of the menus (keep as low as possible to save memory)
        const int itemTrigger = 2;                                              // rotary encoder - counts per tick (varies between encoders usually 1 or 2)
        const int topLine = 18;                                                 // y position of lower area of the display (18 with two colour displays)
        const byte lineSpace1 = 9;                                              // line spacing for textsize 1 (small text)
        const byte lineSpace2 = 17;                                             // line spacing for textsize 2 (large text)
        const int displayMaxLines = 5;                                          // max lines that can be displayed in lower section of display in textsize1 (5 on larger oLeds)
        const int MaxmenuTitleLength = 10;                                      // max characters per line when using text size 2 (usually 10)
        #define IntervalScreenUpdate 250                                        // Screen update interval in milliseconds
        #define IntervalSensorRead 10                                           // Sensor read interval in milliseconds
        #define IntervalRotaryRead 10                                           // Rotary encoder read interval in milliseconds
        #define IntervalDebug 1000                                              // Debug output interval in milliseconds
// -----------------------------------------------------------------------------

// Macro for time intervals
#define every(interval)\
  static uint32_t __every__##interval = millis();\
  if(millis() - __every__##interval >= interval && (__every__##interval = millis()))

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);       // Create display object




// define modes of movement
#define DIRECTION_R 0                                                           // right turn
#define DIRECTION_L 1                                                           // left turn

// define state variables
int rotaryCounter = 0;                                                     // counter for rotary encoder
int rotarydirection = DIRECTION_R;                                       // current direction of rotary encoder start with right
int rotaryCLKstate;                                                      // current state of CLK pin
int rotaryButtonState;                                                     // current state of BUTTON pin
int rotaryPrevCLKstate;                                                     // previous state of CLK pin
int rotaryPrevButtonState;                                                  // previous state of BUTTON pin

int ValForChannel1;                                             
int ValForChannel2;       
int ValForChannel3;
int ValForChannel4;
int ValForAux1;
int ValForAux2;

struct Signal {   
  byte channel1;
  byte channel2;
  byte channel3;
  byte channel4;
  byte aux1;
  byte aux2;
};

Signal data;                
void ResetData() {                                                         // Reset all channels to default values 
  data.channel1 = 0;
  data.channel2 = 127;
  data.channel3 = 127;
  data.channel4 = 127;
  data.aux1 = 0;
  data.aux2 = 0;
}

void drawFrame() {                                                              // Draw frame around the display
  display.clearDisplay();                                                      // Clear the buffer
  display.drawRect(0, 0, 128, 64, SSD1306_WHITE);                            // Draw outer rectangle 
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
  // refactor as function now
  rotaryCLKstate = digitalRead(CLK_PIN);                                        // Read the current state of CLK
  if (rotaryCLKstate != rotaryPrevCLKstate && rotaryCLKstate == HIGH) {         // Has the rotary encoder moved?
    if (digitalRead(DT_PIN) == HIGH) {
      rotaryCounter++;                                                          // Counter-Clockwise
      rotarydirection = DIRECTION_L;
    } else {
      rotaryCounter--;                                                          // Clockwise
      rotarydirection = DIRECTION_R;
    }
    Serial.print("Direction:");
    if (rotarydirection == DIRECTION_R) {
      Serial.println("R ");
    } else {
      Serial.println("L ");
    }
  }
  rotaryPrevCLKstate = rotaryCLKstate;                                          // Store current state as previous state for next loop
  rotaryButtonState = digitalRead(SW_PIN);                                      // Read the current state of BUTTON

  if (rotaryButtonState != rotaryPrevButtonState) {                             // Has the button state changed?
    if (rotaryButtonState == LOW) {
      Serial.println("The button is PRESSED.");
    } else {
      Serial.println("The button is RELEASED.");
    }
  rotaryPrevButtonState = rotaryButtonState;                                    // Store current state as previous state for next loop
  }
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
  delay(2000);
}

// --------------------- void loop() -------------------------------------------
void loop() {

  every(IntervalSensorRead) {                                                   // Read analog values at defined interval
    readAnalogValues();
  }
  every(IntervalRotaryRead)  {                                                  // Read rotary encoder at defined interval
    readRotaryEncoder();
  }
  every(IntervalScreenUpdate) {                                                 // Update screen at defined interval
    drawFrame();
    drawChannelInfo(1, 2, ValForChannel1);
    drawChannelInfo(2, 12, ValForChannel2);
    drawChannelInfo(3, 22, ValForChannel3);
    drawChannelInfo(4, 32, ValForChannel4);
    drawChannelInfo(5, 42, ValForAux1);
    drawChannelInfo(6, 52, ValForAux2);
    display.display();
  }
  if (debug) {
    every(IntervalDebug)  {                                                     // Output debug information at defined interval
      Serial.print("Rotary:");
      Serial.println(rotaryCounter);
      Serial.print("Ch1:");
      Serial.println(ValForChannel1);
      Serial.print("Ch2:");
      Serial.println(ValForChannel2);
    }
  }
}
// --------------------- END ---------------------------------------------------

