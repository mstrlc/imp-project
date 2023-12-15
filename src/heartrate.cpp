/**
 * @file heartrate.cpp
 *
 * IMP project 2023/24
 *
 * ESP32-based heart rate monitor, built with
 * MAX30102 sensor and SSD1306 OLED display
 *
 * @author <xstrel03> Matyáš Strelec
 */

#include <Adafruit_GFX.h>             // Core graphics library
#include <Adafruit_SSD1306.h>         // SSD1306 OLED display library
#include <Fonts/FreeSansBold9pt7b.h>  // Font for display
#include <MAX30105.h>                 // MAX30102 sensor library
#include <SPI.h>                      // SPI communication library
#include <Wire.h>                     // I2C communication library
#include <heartRate.h>                // Heart rate calculation library

#include "images.h"  // Images for display

#define SCREEN_WIDTH 128  // Display width px
#define SCREEN_HEIGHT 64  // Display height px
#define OLED_DC 27        // Display DC pin
#define OLED_CS 5         // Display CS pin
#define OLED_RESET 17     // Display reset pin

const byte RATE_SIZE = 8;
byte rates[RATE_SIZE];
byte rateIndex = 0;

int animationDisplayed = 0;
int selectedImage = 0;
int fingerDisplayed = 0;

byte rateSpot = 0;
long lastBeat = 0;  // Time at which the last beat occurred

float bpm;
int avg;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OLED_DC, OLED_RESET, OLED_CS);
MAX30105 sensor;

// Setup, runs once on startup
void setup() {
    Serial.begin(115200);
    Serial.println("Initializing...");

    // Initializing display and sensor
    if (!display.begin(SSD1306_SWITCHCAPVCC)) {
        Serial.println("Failed to initialize SSD1306 display");
        while (true) {
        }
    }
    if (!sensor.begin(Wire, I2C_SPEED_FAST)) {
        Serial.println("Failed to initialize MAX30102 sensor");
        while (true) {
        }
    }

    // Set up heart rate sensor with default values other than powerLevel,
    // which I found to work best
    byte powerLevel = 32;
    byte sampleAverage = 4;
    byte ledMode = 2;  // Red and IR
    byte sampleRate = 3200;
    int pulseWidth = 69;
    int adcRange = 4096;
    sensor.setup(powerLevel, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange);

    sensor.setPulseAmplitudeRed(0x0A);  // Turn Red LED to low to indicate sensor is running

    // Set up display and print info
    display.setFont(&FreeSansBold9pt7b);
    display.clearDisplay();
    display.display();
    display.setCursor(10, 25);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.println("Heart Rate");
    display.setCursor(10, 45);
    display.println("Monitor");
    display.display();
    delay(2000);
    display.clearDisplay();
    display.drawBitmap(49, 17, heart1, 30, 29, WHITE);
    display.display();
    delay(500);
    display.clearDisplay();
    display.drawBitmap(49, 17, heart2, 30, 29, WHITE);
    display.display();
    delay(500);
    display.clearDisplay();
    display.drawBitmap(49, 17, heart1, 30, 29, WHITE);
    display.display();
    delay(500);
    display.clearDisplay();
    display.drawBitmap(49, 17, heart2, 30, 29, WHITE);
    display.display();
    delay(500);
}

// Write "Place finger..." message to display
void printPlaceFinger() {
    display.clearDisplay();
    if (fingerDisplayed < 20) {
        display.setCursor(10, 25);
        display.println("Place finger");
        display.setCursor(10, 45);
        display.println("on sensor");
    } else {
        display.drawBitmap(49, 17, finger, 30, 29, WHITE);
    }
    display.display();
    delay(10);
    fingerDisplayed++;
    fingerDisplayed %= 40;
}

// Write HR values to display
void printHeartRate(int bpm, int avg) {
    if (animationDisplayed == 15) {
        animationDisplayed = 0;
        selectedImage = !selectedImage;
    }

    display.clearDisplay();
    display.setCursor(10, 25);
    display.print("BPM ");
    display.print(bpm);
    display.setCursor(10, 45);
    display.print("AVG ");
    display.print(avg);

    if (selectedImage == 0) {
        display.drawBitmap(88, 17, heart1, 30, 29, WHITE);
    } else {
        display.drawBitmap(88, 17, heart2, 30, 29, WHITE);
    }

    display.display();
    animationDisplayed++;
}

// Main loop, runs repeatedly
void loop() {
    long irReading = sensor.getIR();

    if (checkForBeat(irReading) == true) {
        // Heart beat detected
        long delta = millis() - lastBeat;
        lastBeat = millis();

        bpm = 60 / (delta / 1000.0);

        // Only store valid values
        if (bpm < 200 && bpm > 40) {
            rates[rateSpot++] = (byte)bpm;  // Store this reading in the array
            rateSpot %= RATE_SIZE;          // Wrap variable

            // Take average of readings
            avg = 0;
            for (byte x = 0; x < RATE_SIZE; x++)
                avg += rates[x];
            avg /= RATE_SIZE;
        }
    }

    if (irReading < 65000) {
        printPlaceFinger();
    } else {
        fingerDisplayed = 0;
        printHeartRate(bpm, avg);
        Serial.print("IR = ");
        Serial.print(irReading);
        Serial.print(", BPM = ");
        Serial.print(bpm);
        Serial.print(", AVG = ");
        Serial.print(avg);
        Serial.println();
    }
    delay(20);
}