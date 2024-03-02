#include <Wire.h>
#include <nfc.h>
#include <Keypad.h>

NFC_Module nfc;

const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {9, 8, 7, 6};
byte colPins[COLS] = {5, 4, 3, 2};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

int accountBalance = 100;
bool authenticated = false;

const int buzzerPin = 12;
const int relayPin = 11;

void setup(void) {
  Serial.begin(9600);
  nfc.begin();
  Serial.println("MF1S50 Reader Demo From Elechouse!");

  uint32_t versiondata = nfc.get_version();
  if (!versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1);
  }

  Serial.print("Found chip PN5");
  Serial.println((versiondata >> 24) & 0xFF, HEX);
  Serial.print("Firmware ver. ");
  Serial.print((versiondata >> 16) & 0xFF, DEC);
  Serial.print('.');
  Serial.println((versiondata >> 8) & 0xFF, DEC);

  nfc.SAMConfiguration();

  pinMode(buzzerPin, OUTPUT);
  pinMode(relayPin, OUTPUT);

  digitalWrite(buzzerPin, HIGH);
  digitalWrite(relayPin, HIGH);

  Serial.println("Welcome to the ATM Banking System");
  Serial.println("Place your card on the module.");
}

void loop(void) {
  u8 buf[32], sta;
  unsigned long start_time, end_time;

  if (!authenticated) {
    sta = nfc.InListPassiveTarget(buf);

    if (sta && buf[0] == 4) {
      Serial.print("UUID length: ");
      Serial.println(buf[0], DEC);
      Serial.print("UUID: ");
      nfc.puthex(buf + 1, buf[0]);
      Serial.println();

      u8 key[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

      sta = nfc.MifareAuthentication(0, 5, buf + 1, buf[0], key);
      if (sta) {
        Serial.println("Authentication success!");

        char enteredPIN[5];
        uint8_t index = 0;

        Serial.println("Enter the PIN:");

        while (index < 4) {
          char key = keypad.getKey();
          if (key) {
            enteredPIN[index] = key;
            Serial.print(key);
            index++;
          }
        }
        enteredPIN[4] = '\0';

        u8 pinNumber[16];
        sta = nfc.MifareReadBlock(5, pinNumber);
        if (sta) {
          Serial.println("\nPIN Number read successfully!");

          if (strcmp(enteredPIN, (char*)pinNumber) == 0) {
            Serial.println("Password is correct");

            authenticated = true;

            Serial.println("Authentication successful!");

            // Turn on the buzzer for 3 seconds
            digitalWrite(buzzerPin, LOW);
            delay(3000);
            digitalWrite(buzzerPin, HIGH);

            Serial.println("Select an option:");
            Serial.println("1. View Account Balance");
            Serial.println("2. Money Withdrawal");
          } else {
            Serial.println("Password is incorrect");
            authenticated = false;
          }
        } else {
          Serial.println("Error reading PIN Number from the block.");
        }
      } else {
        Serial.println("Authentication failed.");
      }
    }
  } else {
    char key = keypad.getKey();
    if (key) {
      if (key == '1') {
        Serial.print("Account balance: ");
        Serial.println(accountBalance);
      } else if (key == '2') {
        Serial.println("Enter the withdrawal amount:");
        uint32_t withdrawalAmount = 0;

        while (true) {
          char key = keypad.getKey();
          if (key) {
            if (key >= '0' && key <= '9') {
              withdrawalAmount = withdrawalAmount * 10 + (key - '0');
              Serial.print(key);
            } else if (key == '#') {
              break;
            }
          }
        }

        if (withdrawalAmount <= accountBalance) {
          accountBalance -= withdrawalAmount;
          Serial.print("\nWithdrawn amount: ");
          Serial.println(withdrawalAmount);
          Serial.print("Remaining balance: ");
          Serial.println(accountBalance);

          // Turn on the relay for 1 second
          digitalWrite(relayPin, LOW);
          delay(1000);
          digitalWrite(relayPin, HIGH);
        } else {
          Serial.println("\nInsufficient account balance!");
        }
      } else {
        Serial.println("Invalid option selected.");
      }
    }
  }

  delay(100);
}
