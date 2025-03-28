// Blynk Setup
#define BLYNK_TEMPLATE_ID "TMPL3B-4xjkR3"
#define BLYNK_TEMPLATE_NAME "Smart Pot"
#define BLYNK_AUTH_TOKEN "-sN9iuqzcuX6jvf53o6G7UJ_hL9ofXZy"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// WiFi credentials
char ssid[] = "Squareseven";
char pass[] = "hellonerds";
char auth[] = BLYNK_AUTH_TOKEN;

// Pins
#define SENSOR_PIN 33
#define RELAY_PIN 4

// LCD setup
LiquidCrystal_I2C lcd(0x27, 16, 2);

// System Variables
bool autoMode = true;   // Default mode: Automatic 
bool motorOn = false;   // Motor state
unsigned long lastMoistureUpdate = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long motorStartTime = 0;
const int moistureCheckInterval = 2000;   // 2 seconds polling
const int displayUpdateInterval = 300000; // 5 minutes update

void setup() {
    Serial.begin(115200);
    lcd.init();
    lcd.backlight();

    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW); // Ensure motor is OFF at start

    // Display startup message
    lcd.setCursor(0, 0);
    lcd.print("Smart Plant Care");
    lcd.setCursor(0, 1);
    lcd.print("Initializing...");
    delay(2000);

    // Connect to WiFi and Blynk
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
    }
    Serial.println("\nWiFi Connected!");
    Blynk.begin(auth, ssid, pass);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Connected to WiFi");
    delay(1000);
}

void loop() {
    Blynk.run();

    unsigned long currentMillis = millis();
    static int lastMoisture = -1;

    // Read soil moisture every 2 seconds
    if (currentMillis - lastMoistureUpdate >= moistureCheckInterval) {
        lastMoistureUpdate = currentMillis;
        int moisture = analogRead(SENSOR_PIN) / 40.95; // Convert to 0-100%

        Serial.print("Moisture: ");
        Serial.print(moisture);
        Serial.println("%");

        // Blynk moisture update
        Blynk.virtualWrite(V0, moisture);

        // Handle auto mode motor control
        if (autoMode) {
            if (moisture < 30 && !motorOn) {
                motorOn = true;
                motorStartTime = currentMillis;
                digitalWrite(RELAY_PIN, HIGH);
                Serial.println("Motor Turned ON (Auto Mode)");
                updateDisplay(moisture);
            } 
            else if (moisture >= 80 && motorOn) {
                motorOn = false;
                digitalWrite(RELAY_PIN, LOW);
                Serial.println("Motor Turned OFF (Auto Mode)");
                updateDisplay(moisture);
            }
        }

        // Sudden drop detection
        if (lastMoisture != -1 && (moisture < lastMoisture - 10)) {
            Serial.println("ALERT: Sudden Moisture Drop!");
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("ALERT: Moisture");
            lcd.setCursor(0, 1);
            lcd.print("Drop Detected!");
            return; // Skip regular display updates
        }

        lastMoisture = moisture;
    }

    // Update the display every 5 minutes unless there is an alert
    if (currentMillis - lastDisplayUpdate >= displayUpdateInterval) {
        lastDisplayUpdate = currentMillis;
        updateDisplay(analogRead(SENSOR_PIN) / 40.95);
    }
}

// Function to update displays (LCD & Blynk)
void updateDisplay(int moisture) {
    String modeText = autoMode ? "Auto" : "Man";
    String motorText = motorOn ? "ON" : "OFF";

    // Update LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Mode: " + modeText);
    lcd.print(" (");
    lcd.print(motorText);
    lcd.print(")");
    lcd.setCursor(0, 1);
    lcd.print("Moisture: ");
    lcd.print(moisture);
    lcd.print("%");

    // Update Blynk LCDs
    Blynk.virtualWrite(V3, modeText + " (Motor: " + motorText + ")");
    Blynk.virtualWrite(V4, "Moisture: " + String(moisture) + "%");
}

// Blynk function to toggle Auto/Manual mode
BLYNK_WRITE(V2) {
    autoMode = param.asInt();
    Serial.println(autoMode ? "Switched to Auto Mode" : "Switched to Manual Mode");
    updateDisplay(analogRead(SENSOR_PIN) / 40.95);
}

// Blynk function for Manual Motor Control
BLYNK_WRITE(V1) {
    if (!autoMode) {  // Only allow control in Manual mode
        int value = param.asInt();
        motorOn = value;
        digitalWrite(RELAY_PIN, motorOn ? HIGH : LOW);
        Serial.println(motorOn ? "Motor Turned ON (Manual Mode)" : "Motor Turned OFF (Manual Mode)");
        updateDisplay(analogRead(SENSOR_PIN) / 40.95);
    }
}
