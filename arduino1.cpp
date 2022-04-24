// C++ code
//
#include <Keypad.h>
#include <LiquidCrystal.h>
#include <Servo.h>
#include <Wire.h>

/* ----- KEYPAD SETUP ----- */

// ### KEYMAP TURN ON POWER KEYPAD
// size of the keypad on the keypad tool
const byte numOnRows = 1;
const byte numOnCols = 1;

// mapping each key on the keypad tool
char keyOnmap[numOnRows][numOnCols] =
    {
        {'A'},
};

// keypad position pin
// information which cable affect the row and column
// can hover mouse into keypad pin
// example pin 9 activate row 1 and pin 2 activate col 4
byte rowOnPins[numOnRows] = {9};
byte colOnPins[numOnCols] = {2};

// create object keypad
Keypad powerKeypad = Keypad(
    makeKeymap(keyOnmap),
    rowOnPins,
    colOnPins,
    numOnRows,
    numOnCols);

// ### KEYMAP PROMPT
const byte numRows = 3;
const byte numCols = 3;

char keymap[numRows][numCols] =
    {
        {'1', '2', '3'},
        {'4', '5', '6'},
        {'7', '8', '9'},
};

byte rowPins[numRows] = {9, 8, 7};
byte colPins[numCols] = {5, 4, 3};

Keypad inputKeypad = Keypad(
    makeKeymap(keymap),
    rowPins,
    colPins,
    numRows,
    numCols);

/* ----- LCD SETUP ----- */

// set lcd position pin
LiquidCrystal lcd(A5, A4, A3, A2, A1, A0);

/* ----- SERVO SETUP ----- */
Servo servo_10;
const int closeDoor = -90;
const int openDoor = 90;

/* ----- MAIN PROGRAM ----- */
bool isClearScreen = false;
bool isDoorOpen = false;
bool isPower = false;
bool isPerSecond = false;
bool isTimeUp = false;
int entryCountdown = 15;
int entryCooldown = 10;
int enterTimeSensor = 10;
String keypadEntry = "";
String password = "5432";
String messageRequest = "";
unsigned long startMillis = 0;
unsigned long currentMillis;
unsigned long period = 1000;
unsigned long threshold;

void setup()
{
    // setup lcd resolution rows x columns (col, row)
    // this resolution is based on the lcd itself
    // in our case, it has 2 rows x 16 columns
    lcd.begin(16, 2);

    // setup the serial
    Serial.begin(9600);

    servo_10.attach(10);
    servo_10.write(0);

    // setup the I2C communication
    Wire.begin();
}

void loop()
{
    /* --- SETUP PER SECOND BOOLEAN --- */
    currentMillis = millis();
    threshold = startMillis + period;

    if (currentMillis >= threshold)
    {
        startMillis = currentMillis;
        isPerSecond = true;
    }
    else
    {
        isPerSecond = false;
    }

    if (!isPower && !isTimeUp)
    {
        // get the value char key from keypad
        char keypressed = powerKeypad.getKey();

        if (keypressed != NO_KEY &&
            keypressed == 'A')
        {
            isPower = true;
        }
    }
    else if (isPower && !isTimeUp)
    {
        // get the value char key from keypad
        char keypressed = inputKeypad.getKey();

        // reset the lcd screen
        if (!isClearScreen)
        {
            lcd.clear();
            isClearScreen = true;
        }

        /* --- SETUP PRINT PASSWORD --- */
        // set the start cursor position (col, row)
        lcd.setCursor(0, 0);

        // write string into lcd
        lcd.print("PASSWORD: ");
        lcd.print(keypadEntry);

        /* --- SETUP PRINT COUNTDOWN --- */
        if (isPerSecond)
        {
            entryCountdown -= 1;
        }

        lcd.setCursor(0, 1);

        String countdownText = (entryCountdown < 10) ? "COUNTDOWN: 0" : "COUNTDOWN: ";
        lcd.print(countdownText);

        lcd.print(entryCountdown);

        isTimeUp = (entryCountdown <= 0) ? true : false;

        /* --- ENTRY PASSWORD HANDLING --- */
        if (keypressed != NO_KEY &&
            keypadEntry.length() < 4)
        {
            keypadEntry += keypressed;
        }
        else if (keypadEntry.length() == 4)
        {
            // clear output screen and set default cursor position
            lcd.clear();

            // entry password is correct
            if (keypadEntry == password)
            {
                isDoorOpen = true;
                servo_10.write(openDoor);

                lcd.setCursor(0, 0);
                lcd.print("UNLOCKED");
                lcd.setCursor(0, 1);
                lcd.print("WELCOME MOBITA");

                // create transmission with address 8
                Wire.beginTransmission(8);
                Wire.write("enteropen");
                Wire.endTransmission();

                entryCountdown = 15;
                isClearScreen = false;
                delay(1000);
            }
            else
            {
                lcd.setCursor(0, 0);
                lcd.print("INCORRECT");
                lcd.setCursor(0, 1);
                lcd.print("PASSWORD");
                delay(1000);
            }
            keypadEntry = "";
        }

        /* --- RESET STATE --- */
        if (entryCountdown <= 0)
        {
            entryCountdown = 15;
            isClearScreen = false;
            isTimeUp = true;
            delay(500);
        }
    }
    else if (isTimeUp)
    {
        // reset the lcd screen
        if (!isClearScreen)
        {
            lcd.clear();
            isClearScreen = true;
        }

        if (isPerSecond)
        {
            entryCooldown -= 1;
        }

        lcd.setCursor(0, 0);
        lcd.print("TIME IS UP");
        lcd.setCursor(0, 1);
        String cooldownText = (entryCooldown < 10) ? "WAIT  " : "WAIT ";
        lcd.print(cooldownText);
        lcd.print(entryCooldown);
        lcd.print(" SECONDS");

        /* --- RESET STATE --- */
        if (entryCooldown <= 0)
        {
            entryCooldown = 10;
            isClearScreen = false;
            isTimeUp = false;
            keypadEntry = "";
            delay(500);
        }
    }

    /* --- DOOR LAB HANDLING --- */
    if (isDoorOpen)
    {
        if (Wire.requestFrom(8, 5))
        {
            while (Wire.available())
            {
                char c = Wire.read();
                messageRequest += c;
                if (messageRequest.length() > 4 &&
                    messageRequest != "enter")
                {
                    messageRequest = "";
                }
            }
        }

        if (isPerSecond)
        {
            enterTimeSensor -= 1;
        }

        if (messageRequest == "enter")
        {
            servo_10.write(closeDoor);

            // create transmission with address 8
            Wire.beginTransmission(8);
            Wire.write("closed");
            Wire.endTransmission();

            enterTimeSensor = 10;
            messageRequest == "";
            isDoorOpen = false;
        }
        else if (enterTimeSensor <= 0)
        {
            servo_10.write(closeDoor);

            // create transmission with address 8
            Wire.beginTransmission(8);
            Wire.write("closed");
            Wire.endTransmission();

            messageRequest == "";
            isDoorOpen = false;
        }
    }
    else
    {
        if (Wire.requestFrom(8, 5))
        {
            while (Wire.available())
            {
                char c = Wire.read();
                messageRequest = c == 'o' ? c : messageRequest + c;

                if (messageRequest.length() > 4 &&
                    messageRequest != "openn")
                {
                    messageRequest = "";
                }
            }
        }

        if (messageRequest == "openn")
        {
            servo_10.write(openDoor);

            // create transmission with address 8
            Wire.beginTransmission(8);
            Wire.write("exitopen");
            Wire.endTransmission();

            enterTimeSensor = 10;
            messageRequest == "";
            isDoorOpen = true;
        }
    }
}
