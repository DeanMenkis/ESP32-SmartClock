#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ezTime.h> // Handles time, timezone, and DST automatically
#include <ArduinoJson.h>
#include <pgmspace.h>

// WiFi Credentials
const char* ssid = "";
const char* password = "";

// API Keys
const char* owmApiKey = ""; // OpenWeatherMap API Key
const char* fmpApiKey = ""; // Financial Modeling Prep API Key

// Coordinates for OpenWeatherMap
const float lat = ;
const float lon = ;

// Stock Symbol
const char* stockSymbol = "SPY";

// Pin Definitions
const int buttonPin = 27; // Capacitive touch button input pin
const int sdaPin = 25;    // I2C Data Pin
const int sclPin = 33;     // I2C Clock Pin

// Display Settings
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1    // Reset pin # (-1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C // Common I2C Address (can also be 0x3D)
#define ICON_WIDTH 32    // Width of weather icons
#define ICON_HEIGHT 32   // Height of weather icons

// Update Intervals
const unsigned long timeUpdateInterval = 1000;        // 1 second
const unsigned long weatherUpdateInterval = 15 * 60 * 1000; // 15 minutes
const unsigned long stockUpdateInterval = 10 * 60 * 1000;   // 10 minutes
const unsigned long ntpSyncTimeout = 30000; // 30 seconds

// --- Weather Icon Bitmaps (32x32 pixels, PROGMEM) ---
// Clear (Sun)
const unsigned char icon_sun_bmp[] PROGMEM = {
  0x00, 0x01, 0x80, 0x00, 0x00, 0x01, 0x80, 0x00, 0x01, 0x81, 0x81, 0x80, 0x00, 0xc1, 0x83, 0x00, 
	0x00, 0xc0, 0x03, 0x00, 0x00, 0x60, 0x06, 0x00, 0x00, 0x0f, 0xf0, 0x00, 0x20, 0x3f, 0xfc, 0x04, 
	0x38, 0x70, 0x0e, 0x1c, 0x1c, 0xe0, 0x07, 0x38, 0x05, 0xc0, 0x03, 0xa0, 0x01, 0x80, 0x01, 0x80, 
	0x03, 0x00, 0x00, 0xc0, 0x03, 0x00, 0x00, 0xc0, 0x03, 0x00, 0x00, 0xc0, 0xf3, 0x00, 0x00, 0xcf, 
	0xf3, 0x00, 0x00, 0xcf, 0x03, 0x00, 0x00, 0xc0, 0x03, 0x00, 0x00, 0xc0, 0x03, 0x00, 0x00, 0xc0, 
	0x01, 0x80, 0x01, 0x80, 0x05, 0xc0, 0x03, 0xa0, 0x1c, 0xe0, 0x07, 0x38, 0x38, 0x70, 0x0e, 0x1c, 
	0x20, 0x3f, 0xfc, 0x04, 0x00, 0x0f, 0xf0, 0x00, 0x00, 0x60, 0x06, 0x00, 0x00, 0xc0, 0x03, 0x00, 
	0x00, 0xc1, 0x83, 0x00, 0x01, 0x81, 0x81, 0x80, 0x00, 0x01, 0x80, 0x00, 0x00, 0x01, 0x80, 0x00
};

// Few/Scattered Clouds (Sun behind cloud)
const unsigned char icon_partly_cloudy_bmp[] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x88, 0x00, 0x00, 0x00, 0x88, 0x80, 0x00, 0x0c, 0xc9, 0x80, 0x00, 
	0x04, 0x79, 0x00, 0x00, 0x02, 0xff, 0x30, 0x00, 0x23, 0x01, 0xa0, 0x00, 0x3a, 0x00, 0xc0, 0x00, 
	0x0c, 0x00, 0x6c, 0x00, 0x0c, 0x00, 0x38, 0x00, 0x68, 0x00, 0x37, 0x00, 0x38, 0x00, 0x7f, 0xc0, 
	0x08, 0x00, 0xc0, 0x30, 0x08, 0x03, 0x00, 0x18, 0x7c, 0x12, 0x3c, 0x08, 0x46, 0xfe, 0x18, 0x08, 
	0x03, 0x82, 0x00, 0x0c, 0x0f, 0x02, 0x00, 0x0c, 0x1a, 0x02, 0x00, 0x08, 0x02, 0x06, 0x00, 0x38, 
	0x0e, 0x00, 0x00, 0xfc, 0x33, 0x00, 0x01, 0x82, 0x60, 0x00, 0x00, 0x07, 0x40, 0x00, 0x00, 0x07, 
	0x40, 0x00, 0x00, 0x0f, 0x40, 0x00, 0x00, 0x1b, 0x40, 0x00, 0x02, 0xd6, 0x20, 0x00, 0xaf, 0x6c, 
	0x1d, 0xef, 0xfe, 0xb8, 0x0f, 0xdd, 0x2b, 0xf0, 0x00, 0xff, 0xff, 0x80, 0x00, 0x01, 0xe0, 0x00
};

// Broken/Overcast Clouds (Cloud only)
const unsigned char icon_cloud_bmp[] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x01, 0xfc, 0x00, 0x00, 0x03, 0x06, 0x00, 0x00, 
	0x06, 0x89, 0x78, 0x00, 0x04, 0x01, 0xe6, 0x00, 0x04, 0x01, 0x83, 0x00, 0x0c, 0x01, 0x83, 0x00, 
	0x7d, 0x00, 0x3f, 0xc0, 0x46, 0x00, 0x70, 0x60, 0x80, 0x07, 0xc0, 0x30, 0x80, 0x1c, 0xa0, 0x10, 
	0xc0, 0x30, 0x80, 0x18, 0x70, 0x30, 0x40, 0x08, 0x1f, 0xe0, 0x00, 0x1e, 0x03, 0xe0, 0x00, 0x33, 
	0x06, 0x18, 0x00, 0x07, 0x06, 0x00, 0x00, 0x07, 0x06, 0x00, 0x29, 0x7f, 0x03, 0xff, 0xfe, 0xbe, 
	0x01, 0xff, 0xef, 0x5c, 0x00, 0x3f, 0xff, 0xe0, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// Drizzle or Rain (Cloud with rain)
const unsigned char icon_rain_bmp[] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x01, 0xfc, 0x00, 0x00, 0x03, 0x06, 0x00, 0x00, 
	0x06, 0x89, 0x78, 0x00, 0x04, 0x01, 0xe6, 0x00, 0x04, 0x01, 0x83, 0x00, 0x0c, 0x01, 0x83, 0x00, 
	0x7d, 0x00, 0x3f, 0xc0, 0x46, 0x00, 0x70, 0x60, 0x80, 0x07, 0xc0, 0x30, 0x80, 0x1c, 0xa0, 0x10, 
	0xc0, 0x30, 0x80, 0x18, 0x70, 0x30, 0x40, 0x08, 0x1f, 0xe0, 0x00, 0x1e, 0x03, 0xe0, 0x00, 0x33, 
	0x06, 0x18, 0x00, 0x07, 0x06, 0x00, 0x00, 0x07, 0x06, 0x00, 0x29, 0x7f, 0x03, 0xff, 0xfe, 0xbe, 
	0x01, 0xff, 0xef, 0x5c, 0x00, 0x3f, 0xff, 0xe0, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// Thunderstorm (Cloud with lightning)
const unsigned char icon_storm_bmp[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x38, 0x7c, 0x00, 
	0x00, 0xf4, 0xee, 0x00, 0x01, 0x83, 0xd3, 0x00, 0x01, 0x01, 0x91, 0x00, 0x03, 0x03, 0x20, 0x00, 
	0x02, 0x02, 0x60, 0xf0, 0x07, 0x04, 0x41, 0x38, 0x19, 0x08, 0x7c, 0x04, 0x30, 0x88, 0x04, 0x04, 
	0x60, 0x08, 0x0c, 0x06, 0x40, 0x07, 0x18, 0x0a, 0xc0, 0x02, 0x30, 0x01, 0x80, 0x06, 0x60, 0x07, 
	0x40, 0x04, 0xc0, 0x0e, 0x30, 0x09, 0x85, 0x7c, 0x0f, 0xdb, 0xff, 0xf0, 0x00, 0x37, 0x80, 0x00, 
	0x00, 0x28, 0x00, 0x00, 0x00, 0x70, 0x00, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// Snow (Cloud with snowflakes)
const unsigned char icon_snow_bmp[] PROGMEM = {
	0x00, 0x07, 0xc0, 0x00, 0x00, 0x1c, 0xe0, 0x00, 0x00, 0x31, 0x30, 0x00, 0x00, 0x63, 0x1b, 0x00, 
	0x00, 0x46, 0x8f, 0xc0, 0x00, 0x44, 0x08, 0x40, 0x1f, 0xc0, 0x78, 0x70, 0x30, 0xc0, 0x30, 0x7c, 
	0x60, 0x40, 0x01, 0xc6, 0x40, 0x40, 0x00, 0x03, 0xd9, 0x81, 0xe0, 0x03, 0x98, 0x03, 0xb0, 0x03, 
	0x90, 0x06, 0x40, 0x06, 0xc0, 0x00, 0x00, 0x7e, 0x40, 0x00, 0x00, 0x38, 0x60, 0xc6, 0x02, 0x18, 
	0x3f, 0x8c, 0x06, 0x1c, 0x0f, 0x98, 0x04, 0x1c, 0x00, 0xfc, 0x3f, 0xf8, 0x00, 0x7d, 0xff, 0xf0, 
	0x00, 0x03, 0xf8, 0x00, 0x00, 0x01, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xce, 0x73, 0x80, 
	0x01, 0xee, 0x77, 0x80, 0x01, 0xce, 0x73, 0x80, 0x00, 0x84, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x39, 0xdc, 0x00, 0x00, 0x7b, 0xde, 0x00, 0x00, 0x3b, 0x9c, 0x00, 0x00, 0x21, 0x08, 0x00
};

// Atmospheric Conditions (Fog/Mist lines)
const unsigned char icon_fog_bmp[] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xff, 0x1f, 0xff, 0x1f, 0xff, 0x1f, 0xff, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xc7, 0xff, 0x00, 
	0xff, 0xc7, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xfe, 0x78, 
	0x01, 0xff, 0xfe, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xfc, 0x3f, 0xe0, 
	0x0f, 0xfe, 0x7f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x01, 0xff, 0xf8, 0x00, 0x01, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// Unknown (Question mark)
const unsigned char icon_unknown_bmp[] PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0xF8, 0x00,
    0x00, 0xFF, 0xFE, 0x00, 0x03, 0xFF, 0xFF, 0x80, 0x07, 0xC0, 0x03, 0xC0, 0x0F, 0x80, 0x01, 0xE0,
    0x0F, 0x00, 0x00, 0xE0, 0x1E, 0x00, 0x00, 0xE0, 0x1C, 0x00, 0x01, 0xC0, 0x38, 0x00, 0x03, 0x80,
    0x30, 0x00, 0x07, 0x00, 0x30, 0x00, 0x0E, 0x00, 0x60, 0x00, 0x1C, 0x00, 0x60, 0x00, 0x18, 0x00,
    0x40, 0x00, 0x38, 0x00, 0x40, 0x00, 0x30, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x30, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x30, 0x38, 0x38, 0x00, 0x30, 0x38, 0x38, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};


// --- Global Variables ---
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Timezone myTZ;

// Display State Machine
enum DisplayState { TIME, WEATHER, STOCK };
DisplayState currentDisplayState = TIME;
volatile bool buttonPressed = false; // Flag to indicate button press detected in ISR
int lastButtonState = LOW;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50; // Debounce time in ms

// Data Storage
String currentTimeStr = "Loading...";
String weatherTempStr = "-- C";
String weatherCondStr = "Unknown";
// Pointer to the current weather icon bitmap in PROGMEM
const unsigned char* currentIconBitmap = icon_unknown_bmp; // Initialize with unknown
String stockPriceStr = "----.--";
String stockChangeStr = "-.--%";
String stockStatus = "Loading"; // Loading, OK, Error

// Timing for Updates
unsigned long lastTimeUpdate = 0;
unsigned long lastWeatherUpdate = 0;
unsigned long lastStockUpdate = 0;

// --- Function Prototypes ---
void connectWiFi();
void updateTime();
void updateWeather();
void updateStock();
void displayTime();
void displayWeather();
void displayStock();
void IRAM_ATTR handleButtonInterrupt();
const unsigned char* getWeatherIcon(String mainCondition, String description);
void drawCenteredString(const String &text, int y);
void drawString(int x, int y, const String &text, int size = 1);


// --- Interrupt Service Routine ---
void IRAM_ATTR handleButtonInterrupt() {
  // Basic debounce within ISR
  if ((long)(millis() - lastDebounceTime) >= debounceDelay) {
     buttonPressed = true; // Set flag for main loop to handle
     lastDebounceTime = millis();
  }
}

// --- Setup Function ---
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\nESP32 Live Display Starting...");

  // Initialize I2C
  Serial.println("Initializing I2C...");
  Wire.begin(sdaPin, sclPin);
  delay(100);
  Serial.println("I2C Initialized.");

  // Initialize OLED display
  Serial.println("Initializing OLED...");
  Serial.print("Attempting to use I2C address: 0x"); Serial.println(SCREEN_ADDRESS, HEX);
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed OR I2C connection issue!"));
    Serial.println(F("Check wiring (SDA, SCL, GND, VCC) & I2C Address (0x3C or 0x3D)"));
    while(1);
  }
  Serial.println("OLED Initialized Successfully.");
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("OLED OK");
  display.display();
  delay(1000);

  // Setup Button Pin
  Serial.println("Initializing Button...");
  pinMode(buttonPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(buttonPin), handleButtonInterrupt, RISING);
  Serial.println("Button Initialized.");

  // Connect to WiFi
  connectWiFi();

  if (WiFi.status() == WL_CONNECTED) {
    // Setup Timezone
    Serial.println("Setting Timezone...");
    myTZ.setLocation(F("America/Vancouver")); // or whatever you want
    Serial.print("Timezone set to: ");
    Serial.println(myTZ.getOlsen());

    display.clearDisplay();
    display.setCursor(0, 10);
    display.println("Fetching data...");
    display.display();
    Serial.println("Fetching initial data...");

    Serial.println("Waiting for NTP time sync...");
    if (waitForSync(ntpSyncTimeout)) {
        Serial.println("NTP Sync Successful.");
        updateTime();
    } else {
        Serial.println("NTP Sync Failed/Timed Out!");
        currentTimeStr = "No Time Sync";
    }

    updateWeather(); // Sets currentIconBitmap
    updateStock();

    lastTimeUpdate = millis();
    lastWeatherUpdate = millis();
    lastStockUpdate = millis();

    Serial.println("Initial data fetch complete.");
    displayTime(); // Show initial screen

  } else {
    Serial.println("Setup halted: No WiFi connection.");
    display.clearDisplay();
    display.setCursor(0, 10);
    display.println("Setup Failed:");
    display.setCursor(0, 20);
    display.println("No WiFi!");
    display.display();
  }

  Serial.println("Setup Complete. Entering main loop...");
}

// --- Main Loop ---
void loop() {
  unsigned long currentMillis = millis();

  if (buttonPressed) {
    Serial.println("Button Pressed - Changing State");
    currentDisplayState = (DisplayState)(((int)currentDisplayState + 1) % 3);
    Serial.print("New State: "); Serial.println((int)currentDisplayState);

    // Update display
    switch (currentDisplayState) {
      case TIME:
        displayTime();
        break;
      case WEATHER:
        displayWeather();
        break;
      case STOCK:
        displayStock();
        break;
    }
    buttonPressed = false; // Reset flag
  }

  // Update Time
  if (timeStatus() != timeNotSet && currentMillis - lastTimeUpdate >= timeUpdateInterval) {
    updateTime();
    if (currentDisplayState == TIME) {
      displayTime();
    }
    lastTimeUpdate = currentMillis;
  }

  // Update Weather
  if (currentMillis - lastWeatherUpdate >= weatherUpdateInterval) {
    updateWeather();
    if (currentDisplayState == WEATHER) {
       Serial.println("Updating Weather Display");
      displayWeather();
    }
    lastWeatherUpdate = currentMillis;
  }

  // Update Stock
  if (currentMillis - lastStockUpdate >= stockUpdateInterval) {
    updateStock();
    if (currentDisplayState == STOCK) {
       Serial.println("Updating Stock Display");
      displayStock();
    }
    lastStockUpdate = currentMillis;
  }

  // ezTime background tasks
  events();

  delay(10); // Small delay for stability
}

void connectWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  display.clearDisplay();
  display.setCursor(0, 10);
  display.print("Connecting WiFi...");
  display.display();

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Waiting for connection...");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    display.print(".");
    display.display();
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    display.clearDisplay();
    display.setCursor(0, 10);
    display.print("WiFi Connected!");
    display.setCursor(0, 20);
    display.print(WiFi.localIP());
    display.display();
    delay(1500);
  } else {
    Serial.println("Failed to connect to WiFi!");
    display.clearDisplay();
    display.setCursor(0, 10);
    display.print("WiFi Failed!");
    display.display();
  }
}

void updateTime() {
  if (timeStatus() != timeNotSet) {
      currentTimeStr = myTZ.dateTime("H:i:s"); // 24-hour format
  } else {
      currentTimeStr = "Sync Error";
      Serial.println("Time Update Skipped: Time not synchronized.");
  }
}


void updateWeather() {
  Serial.println("Attempting Weather Update...");
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Weather Update Failed: WiFi disconnected.");
    weatherTempStr = "N/A";
    weatherCondStr = "No WiFi";
    currentIconBitmap = icon_unknown_bmp;
    return;
  }

  Serial.println("Updating Weather...");
  HTTPClient http;
  String url = "http://api.openweathermap.org/data/2.5/weather?lat=" + String(lat) + "&lon=" + String(lon) + "&appid=" + String(owmApiKey) + "&units=metric";
  Serial.print("Weather URL: "); Serial.println(url);

  http.begin(url);
  http.setTimeout(10000); // 10 second timeout
  int httpCode = http.GET();
  Serial.print("Weather HTTP Code: "); Serial.println(httpCode);

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    DynamicJsonDocument doc(1200);
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
      Serial.print(F("Weather deserializeJson() failed: ")); Serial.println(error.f_str());
      weatherTempStr = "ERR";
      weatherCondStr = "JSON Error";
      currentIconBitmap = icon_unknown_bmp;
    } else {
      // Check JSON structure before accessing
      if (doc.containsKey("main") && doc["main"].is<JsonObject>() &&
          doc.containsKey("weather") && doc["weather"].is<JsonArray>() && doc["weather"].size() > 0 &&
          doc["weather"][0].is<JsonObject>())
      {
          JsonObject main = doc["main"];
          if (main.containsKey("temp")) {
            float temp = main["temp"];
            weatherTempStr = String((int)round(temp)) + " C";
          } else {
             weatherTempStr = "N/A";
             Serial.println("Weather JSON missing 'temp'.");
          }

          JsonObject weather_0 = doc["weather"][0];
          String mainCondition = weather_0["main"] | "Unknown";
          String description = weather_0["description"] | "no description";
          weatherCondStr = description;
          if (weatherCondStr.length() > 0) {
               weatherCondStr[0] = toupper(weatherCondStr[0]); 
          }
          currentIconBitmap = getWeatherIcon(mainCondition, description);

          Serial.print("Weather Updated: "); Serial.print(weatherTempStr); Serial.print(", "); Serial.println(weatherCondStr);
      } else {
          Serial.println("Weather JSON structure invalid or missing keys.");
          weatherTempStr = "ERR";
          weatherCondStr = "Data Error";
          currentIconBitmap = icon_unknown_bmp;
      }
    }
  } else {
    Serial.print("Weather HTTP Request Failed, Error: "); Serial.println(http.errorToString(httpCode).c_str());
    weatherTempStr = "ERR";
    weatherCondStr = "HTTP Err";
    currentIconBitmap = icon_unknown_bmp;
  }
  http.end();
}

void updateStock() {
    Serial.println("Attempting Stock Update...");
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Stock Update Failed: WiFi disconnected.");
        stockPriceStr = "N/A";
        stockChangeStr = "No WiFi";
        stockStatus = "Error";
        return;
    }

    Serial.println("Updating Stock Data...");
    HTTPClient http;
    String url = "https://financialmodelingprep.com/api/v3/quote/" + String(stockSymbol) + "?apikey=" + String(fmpApiKey);
    Serial.print("Stock URL: "); Serial.println(url);

    http.begin(url);
    http.setTimeout(10000); // 10 second timeout
    int httpCode = http.GET();
    Serial.print("Stock HTTP Code: "); Serial.println(httpCode);

    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, payload);

        if (error) {
            Serial.print(F("Stock deserializeJson() failed: ")); Serial.println(error.f_str());
            stockPriceStr = "ERR";
            stockChangeStr = "JSON Err";
            stockStatus = "Error";
        } else {
            // Check JSON structure
            if (doc.is<JsonArray>() && doc.size() > 0) {
                JsonObject quote = doc[0];
                // Check required fields exist and are not null
                if (quote.containsKey("price") && !quote["price"].isNull() &&
                    quote.containsKey("changesPercentage") && !quote["changesPercentage"].isNull())
                {
                    double price = quote["price"];
                    double changePercent = quote["changesPercentage"];

                    stockPriceStr = String(price, 2);
                    stockChangeStr = String(changePercent, 2) + "%";
                    if (changePercent > 0) {
                        stockChangeStr = "+" + stockChangeStr;
                    }
                    stockStatus = "OK";

                    Serial.print("Stock Updated: "); Serial.print(stockPriceStr); Serial.print(" ("); Serial.print(stockChangeStr); Serial.println(")");
                } else {
                     Serial.println("Stock JSON missing required fields or fields are null.");
                     stockPriceStr = "ERR";
                     stockChangeStr = "Data Err";
                     stockStatus = "Error";
                }
            } else {
                 Serial.println("Stock JSON is not a valid array or is empty.");
                 stockPriceStr = "ERR";
                 stockChangeStr = "API Err";
                 stockStatus = "Error";
            }
        }
    } else {
        String errorPayload = http.errorToString(httpCode);
        Serial.print("Stock HTTP Request Failed, Code: "); Serial.print(httpCode); Serial.print(", Error: "); Serial.println(errorPayload);
        stockPriceStr = "ERR";
        stockChangeStr = "HTTP " + String(httpCode); // Show HTTP code on display
        stockStatus = "Error";
    }
    http.end();
}


// --- Display Functions ---

void displayTime() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);

  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(currentTimeStr, 0, 0, &x1, &y1, &w, &h);
  int cursorX = (SCREEN_WIDTH - w) / 2;
  int cursorY = (SCREEN_HEIGHT - h) / 2 + 2; // Slightly adjust vertical centering

  display.setCursor(cursorX, cursorY);
  display.print(currentTimeStr);

  display.display();
}

void displayWeather() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // Center icon
  int iconX = (SCREEN_WIDTH - ICON_WIDTH) / 2;
  int iconY = 2; // Padding from top

  // Draw icon bitmap
  if (currentIconBitmap != nullptr) {
      display.drawBitmap(iconX, iconY, currentIconBitmap, ICON_WIDTH, ICON_HEIGHT, SSD1306_WHITE);
  } else {
      // Fallback (shouldn't happen if initialized correctly)
      display.drawBitmap(iconX, iconY, icon_unknown_bmp, ICON_WIDTH, ICON_HEIGHT, SSD1306_WHITE);
      Serial.println("Error: currentIconBitmap was null!");
  }

  // Draw Temperature
  display.setTextSize(2);
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(weatherTempStr, 0, 0, &x1, &y1, &w, &h);
  int tempX = (SCREEN_WIDTH - w) / 2;
  int tempY = iconY + ICON_HEIGHT + 3; // Position below icon with padding
  display.setCursor(tempX, tempY);
  display.print(weatherTempStr);

  // Draw Condition Text
  display.setTextSize(1);
  display.getTextBounds(weatherCondStr, 0, 0, &x1, &y1, &w, &h);
  int condX = (SCREEN_WIDTH - w) / 2;
  int condY = SCREEN_HEIGHT - h - 1; // Position near bottom with padding
  // Basic overlap check (adjust if needed)
  if (condY < tempY + 16) {
      condY = tempY + 16 + 1;
      if (condY > SCREEN_HEIGHT - h) condY = SCREEN_HEIGHT - h; // Ensure it fits
  }
  display.setCursor(condX, condY);
  display.print(weatherCondStr);

  display.display();
}

void displayStock() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // Title (Stock Symbol)
  display.setTextSize(1);
  drawCenteredString(stockSymbol, 5); // Position near top

  // Price
  display.setTextSize(2);
  drawCenteredString(stockPriceStr, 20); // Position below title

  // Change Percentage
  display.setTextSize(1);
  drawCenteredString(stockChangeStr, 45); // Position below price

  display.display();
}

// --- Utility Functions ---

// Helper to draw centered text
void drawCenteredString(const String &text, int y) {
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  int cursorX = (SCREEN_WIDTH - w) / 2;
  if (cursorX < 0) cursorX = 0; // Prevent negative X coordinate
  display.setCursor(cursorX, y);
  display.print(text);
}

// Helper to draw text at specific coords
void drawString(int x, int y, const String &text, int size) {
    display.setTextSize(size);
    display.setCursor(x,y);
    display.print(text);
}


// Returns a pointer to the appropriate weather icon bitmap in PROGMEM
const unsigned char* getWeatherIcon(String mainCondition, String description) {
    mainCondition.toLowerCase();
    description.toLowerCase();

    if (mainCondition == "clear") return icon_sun_bmp;
    if (mainCondition == "clouds") {
        if (description.indexOf("few") != -1 || description.indexOf("scattered") != -1) return icon_partly_cloudy_bmp;
        else return icon_cloud_bmp; // Broken, overcast etc.
    }
    if (mainCondition == "drizzle" || mainCondition == "rain") return icon_rain_bmp;
    if (mainCondition == "thunderstorm") return icon_storm_bmp;
    if (mainCondition == "snow") return icon_snow_bmp;
    // Group all atmospheric conditions under fog/mist icon
    if (mainCondition == "mist" || mainCondition == "smoke" || mainCondition == "haze" ||
        mainCondition == "dust" || mainCondition == "fog" || mainCondition == "sand" ||
        mainCondition == "ash" || mainCondition == "squall" || mainCondition == "tornado") {
        return icon_fog_bmp;
    }

    // Fallback for any other unknown conditions
    Serial.print("Unknown weather condition, using '?': "); Serial.print(mainCondition); Serial.print(" / "); Serial.println(description);
    return icon_unknown_bmp;
}