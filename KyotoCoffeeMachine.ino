/*
   Tinker and Twist
   Kyoto Cold Drip Coffee Maker
   Copyright 2018 - GPL v3.0

   Author : Martin Bogomolni <martinbogo@gmail.com>

   This code provides control for our Kyoto-style cold drip
   coffee maker.  Hardware required is an Arduino Uno or
   similar board, an Adafruit RGBLCDShield, and an Adafruit
   stepper/motor shield board.

   This project was inspired by the awesome coffee machine that
   sits in the windowfront of the Ada's Technical Books and Cafe
   in Seattle WA.  ( http://www.seattletechnicalbooks.com/ )

   To build this machine, you will need the following:

   Yama Glass Kyoto Style Coffee Machine : http://a.co/4k3O1dN

   Arduino Uno : https://www.adafruit.com/product/50
   RGBLCDShield : https://www.adafruit.com/product/714
   Stepper/Motor Shield : https://www.adafruit.com/product/1438
   12V 1A Power Supply : https://www.adafruit.com/product/798

   A quiet solenoid can be sourced from Ada Techincal Bookstore
   to control the drip.  They are the quietest we have tested.

   http://www.seattletechnicalbooks.com/product/super-quiet-solenoid

   By default this code only supports one solenoid, and uses
   millis() timing to control when the drops occur.  It must be
   connected to the DC Motor1 connector.  Use a good 12V/1A power
   supply.

   Happy Coffee Making!

   Martin
*/

#define VERSION "1.0.5"

#include <Wire.h>
#include <Adafruit_MCP23017.h>
#include <Adafruit_RGBLCDShield.h>
#include <Adafruit_MotorShield.h>
#include <Servo.h>
#include <EEPROM.h>
#include "colors.h"

// Create an Adafruit LCD shield object
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

// Create an Adafruit Motor Shield object
Adafruit_MotorShield mshield = Adafruit_MotorShield();

Adafruit_DCMotor *motor1 = mshield.getMotor(1);

// Initial 'drop' valve time is 70ms
#define DROPTIME 70
// Initial time between drops is 1s
#define DROPDELAY 1000

unsigned long previousMillis;
unsigned long dropMillis;
unsigned long currentMillis;
int t_display = 250;
int t_drop = DROPTIME;
int t_delay = DROPDELAY;
int s_enable = 0;
int displaymode = 0;
uint8_t buttons = 0;
uint8_t buttons_last = 0;
colorState current_color = WHITE;
boolean bip;
boolean showonce;

struct ConfigStruct {
  colorState color;
  int t_drop_val;
  int t_delay_val;
} settings = {
  WHITE,
  70,
  1000
};

void setup() {

  // Initialize the serial port for debug output
  Serial.begin(115200);

  current_color = EEPROM.read(0);
  if ( current_color > 0x7 ) {
    current_color = 0x7;
    EEPROM.write(0, 0x7);
  }

  lcd.begin(16, 2); // initialize display
  lcd.setBacklight(current_color);
  lcd.noCursor();
  lcd.clear();

  mshield.begin();

  lcd.display();
  lcd.setCursor(1, 0);
  lcd.print("Tinker & Twist");
  lcd.setCursor(0, 1);
  lcd.print("Cold Coffee Brew");
  delay(2000);

  mshield.begin();

  // set motor 1 to max PWM, then idle
  motor1->setSpeed(255);
  motor1->run(RELEASE);

  displaymode = 2; // default to pause
}

void loop() {
  // put your main code here, to run repeatedly:
  currentMillis = millis();
  if ((unsigned long)(currentMillis - previousMillis) >= t_display) {
    updateDisplay();
    previousMillis = currentMillis;
  }
  readButtons();
  currentMillis = millis();
  if ((unsigned long)(currentMillis - dropMillis) >= t_delay) {
    doMotor();
    dropMillis = currentMillis;
  }
}

void readButtons() {
  switch (displaymode) {
    case 0:
      buttons = lcd.readButtons();
      if ( buttons & BUTTON_SELECT ) {
        displaymode = 1;
        delay(250);
        lcd.clear();
        break;
      }
      if ( buttons & BUTTON_LEFT ) {
        t_drop = t_drop - 10;
        if ( t_drop < 0 ) t_drop = 0;
        delay(250);
        lcd.clear();
        break;
      }
      if ( buttons & BUTTON_RIGHT ) {
        t_drop = t_drop + 10;
        delay(250);
        lcd.clear();
        break;
      }
      if ( buttons & BUTTON_UP ) {
        t_delay = t_delay + 200;
        delay(250);
        lcd.clear();
        break;
      }
      if ( buttons & BUTTON_DOWN ) {
        t_delay = t_delay - 200;
        if ( t_delay < 0 ) t_delay = 0;
        delay(250);
        lcd.clear();
        break;
      }
      break;
    case 2: // pause until any button pushed
      buttons = lcd.readButtons();
      while ( !buttons ) {
        buttons = lcd.readButtons();
      }
      buttons = 0;
      displaymode = 0;
      lcd.clear();
      break;
    default:
      break;
  }
  return;
}

void updateDisplay() {
  switch (displaymode) {
    case 0:
      lcd.setCursor(0, 0);
      lcd.print("Drop: ");
      lcd.print(t_drop);
      lcd.print(" msec");
      lcd.setCursor(0, 1);
      lcd.print("Rate: ");
      lcd.print(t_delay / 1000);
      lcd.print(".");
      lcd.print((t_delay / 100) % 10);
      lcd.print(" sec ");
      break;
    case 1:
      doMenu();
      break;
    case 2:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Press any button");
      lcd.setCursor(0, 1);
      lcd.print("to begin brewing");
      break;
    default:
      break;
  }
}

void doMotor() {
  /* We keep track of when the drop needs to happen by setting
     drop_alarm.  If the drop_alarm has been exceeded, run the
     motor for drop_time then reset drop_alarm into the future
     by t_delay
  */
  if ( t_delay == 0 ) {    // valve full open || special case
    motor1->run(FORWARD);
    lcd.setCursor(15,0);
    lcd.print((char)255);
  } else {
    motor1->run(FORWARD);
    lcd.setCursor(15, 0);
    lcd.print((char)243);
    delay(t_drop);
    motor1->run(RELEASE);
    // If the drop is less than 50ms, extend the display so we
    //  can see it on the display.
    if ( t_drop < 50 ) {
      delay(50);
    }
    lcd.setCursor(15, 0);
    lcd.print(" ");
  }
}

/*
   Right now, there is only one menu item.  In the future, I
   will add a drop calibration option, which will add a fixed
   amount of time to each drop to control the precise amount
   of mL per drop, and support for more than one solenoid for
   Kyoto brewers that have more than one dropper.
*/
void doMenu() {
  // buttonstate is initialized on entry to the menu
  buttons = 0;
  int menuitem = 0;
  int exitstate = 0;

  while ( !exitstate ) {
    switch (menuitem) {
      case 0: {
          if (showonce) {
            menuitem = 1;
            break;
          }
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Menu Item: UP/DN");
          lcd.setCursor(0, 1);
          lcd.print("Value: LT/RT/SELECT");
          while ( !buttons ) {
            buttons = lcd.readButtons();
          }
          showonce = 1;
          menuitem = 1;
          break;
        }
      case 1: {
          buttons = 0;
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Backlight Color");
          lcd.setCursor(0, 1);
          lcd.print(colorname[current_color]);
          while ( !buttons ) {
            buttons = lcd.readButtons();
            delay(40);
          }
          if ( buttons & BUTTON_LEFT ) {
            current_color = current_color - 1;
            if ( current_color < 0x0 ) current_color = 0x0;
            lcd.setBacklight(current_color);
            EEPROM.write(0, current_color);
          }
          if ( buttons & BUTTON_RIGHT ) {
            current_color = current_color + 1;
            if ( current_color > 0x7 ) current_color = 0x7;
            lcd.setBacklight(current_color);
            EEPROM.write(0, current_color);
          }
          if ( buttons & BUTTON_SELECT ) {
            delay(250);
            lcd.clear();
            displaymode = 0;
            exitstate = 1;
          }
        }
        break;
      default:
        exitstate = 1;
        break;
    }
  }
}

