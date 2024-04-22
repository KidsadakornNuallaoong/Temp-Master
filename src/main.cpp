#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "./config.h"


#include <SPI.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include "./Adafruit_GC9A01A-main/Adafruit_GC9A01A.h" // Hardware-specific library
#include <CST816S.h>

#define TFT_CS     32 // for select
#define TFT_RST    25 // for reset
#define TFT_DC     33 // for data/command
#define TFT_MOSI   23
#define TFT_MISO   19
#define TFT_SCLK   18
#define TFT_BL     26

#define TS_SDA     21    // Connect to tp_sda
#define TS_SCL     22    // Connect to tp_scl
#define TS_INT     16    // Connect to tp_int
#define TS_RST     4       // Connect to tp_rst

#define clk 15
#define dt 17
#define sw 35

#define UiYMove 30

Adafruit_GC9A01A tft(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST, TFT_MISO);

// sda, scl, rst, irq
CST816S touch(TS_SDA, TS_SCL, TS_RST, TS_INT);

float temperature = 0;
float humidity = 0;

void touchTask(void *pvParameters);
void screenTask(void *pvParameters);

void sendWeather(float temp, float hum);

int screen[3] = {0, 1, 2};
int stateScreen = 0;

int counter = 0;
int currentStateCLK;
int lastStateCLK;
String currentDir = "";
unsigned long lastButtonPress = 0;

void setup() {
  Serial.begin(115200);
  stateScreen = 1;

  tft.begin();
  tft.setRotation(2);
  // tft.fillScreen(GC9A01A_PINK);

  // SPI.begin(); // Initialize SPI
  // SPI.setClockDivider(SPI_CLOCK_DIV2); // Set SPI speed to 8 MHz (half the default speed)

  // Connect to Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Connecting to WiFi...");
    stateScreen = 1;
    delay(1000);
  }

  // Print the local IP address
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Connected to WiFi!");
  stateScreen = 0;

  // rotary encoder
  pinMode(clk, INPUT);
  pinMode(dt, INPUT);
  pinMode(sw, INPUT_PULLUP);

  // Initialize the touch screen and the screen
  touch.begin();
  Serial.print(touch.data.version);
  Serial.print("\t");
  Serial.print(touch.data.versionInfo[0]);
  Serial.print("-");
  Serial.print(touch.data.versionInfo[1]);
  Serial.print("-");
  Serial.println(touch.data.versionInfo[2]);

  tft.setRotation(0); // Adjust rotation if needed
  tft.fillScreen(GC9A01A_BLACK);
  pinMode(TFT_BL, OUTPUT);
  pinMode(TFT_RST, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  // Create tasks for touch screen and screen processing on different cores
  xTaskCreatePinnedToCore(
    touchTask,          // Function to run on the touch screen task
    "TouchTask",        // Name of the task
    10000,              // Stack size (bytes)
    NULL,               // Task input parameter
    1,                  // Priority of the task (1 is default)
    NULL,               // Task handle
    0                   // Core to run the task on (Core 0)
  );

  xTaskCreatePinnedToCore(
    screenTask,         // Function to run on the screen task
    "ScreenTask",       // Name of the task
    10000,              // Stack size (bytes)
    NULL,               // Task input parameter
    1,                  // Priority of the task (1 is default)
    NULL,               // Task handle
    1                   // Core to run the task on (Core 1)
  );
}

void loop(void) {

  temperature = random(15, 100);
  humidity = random(15, 100);
  delay(1000);
}

void touchTask(void *pvParameters) {
  Serial.println(stateScreen);

  while (1) {
    // Read the current state of CLK
  currentStateCLK = digitalRead(clk);

  // If last and current state of CLK are different, then pulse occurred
  // React to only 1 state change to avoid double count
  if (currentStateCLK != lastStateCLK && currentStateCLK == 1)
  {

    // If the DT state is different than the CLK state then
    // the encoder is rotating CCW so decrement
    if (digitalRead(dt) != currentStateCLK)
    {
      counter--;
      currentDir = "CCW";
    }
    else
    {
      // Encoder is rotating CW so increment
      counter++;
      currentDir = "CW";
    }

    Serial.print("Direction: ");
    Serial.print(currentDir);
    Serial.print(" | Counter: ");
    Serial.println(counter);
  }

  // Remember last CLK state
  lastStateCLK = currentStateCLK;

  // Read the button state
  int btnState = digitalRead(sw);

  // If we detect LOW signal, button is pressed
  if (btnState == LOW)
  {
    // if 50ms have passed since last LOW pulse, it means that the
    // button has been pressed, released and pressed again
    if (millis() - lastButtonPress > 50)
    {
      Serial.println("Button pressed!");
    }

    // Remember last button press event
    lastButtonPress = millis();
  }
  
    if (touch.available()) {
      // Read touch screen data and process it
      // Example: Print touch data for debugging
      String touchData = touch.gesture();
      if(touchData == "SINGLE CLICK"){
        stateScreen = 1;
        sendWeather(temperature, humidity);
      }
    }

    // Adjust the delay as needed to control the task execution rate
    delay(100);
  }
}

void screenTask(void *pvParameters) {
  while (1) {
    /// Update screen or perform other screen-related tasks here
    // For example, drawing graphics on the screen
    if(screen[stateScreen] == 0){
      // tft.setRotation(2);
      // tft.invertDisplay(true); // Invert color if needed
      // tft.setTextColor(GC9A01A_RED);
      // tft.setCursor(100 , 120);
      // tft.print(WiFi.SSID(0));
      // yield();
      // tft.fillScreen(GC9A01A_BLACK);
      // yield();
      // Reset the screen by filling it with black color
      char H[5];
      char T[5];

      sprintf(H, "%02d", int(humidity));
      sprintf(T, "%02d", int(temperature));
      
      // * draw rectangle the screen
      tft.setSPISpeed(40000000);

      // * draw circle the screen
      tft.fillRect(60, 120, 50, 50, GC9A01A_BLACK);
      tft.fillRect(100, 90, 40, 30, GC9A01A_BLACK);
      tft.drawCircle(120, 120, 90, GC9A01A_WHITE);
      tft.drawCircle(120, 120, 100, GC9A01A_WHITE);

      // // * draw the screen
      tft.setRotation(2);
      tft.invertDisplay(true); // Invert color if needed
      tft.setTextColor(GC9A01A_WHITE);
      tft.setCursor(110 , 30 + UiYMove);
      tft.setTextSize(4);
      tft.print("H");
      tft.setCursor(140 , 56 + UiYMove);
      tft.setTextSize(2);
      tft.print("%");
      tft.setCursor(110 , 65 + UiYMove);
      tft.setTextSize(2);
      tft.print(String(H));

      tft.setCursor(65 , 100 + UiYMove);
      tft.setTextSize(4);
      tft.print(String(T) + "°C");
      delay(500);
      
    } else if (screen[stateScreen] == 1) {
      // * loading screen
      tft.setRotation(2);
      tft.invertDisplay(true); // Invert color if needed
      // * draw circle the screen
      tft.fillCircle(120, 115, 120, GC9A01A_BLACK);
      tft.drawCircle(120, 120, 90, GC9A01A_GREEN);
      tft.drawCircle(120, 120, 100, GC9A01A_GREEN);

      tft.setTextColor(GC9A01A_WHITE);
      tft.setCursor(60 , 110);
      tft.setTextSize(2);
      tft.print("Loading...");
      delay(100);
      tft.fillCircle(120, 115, 120, GC9A01A_BLACK);
    } else {
      // Clear the screen if needed
      tft.fillScreen(GC9A01A_BLACK);
    }
  }
}

void sendWeather(float temp, float hum) {

  // Send the data to the server
  HTTPClient http;
  http.begin("https://euphyllia-discord-bot.onrender.com/weather");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("authorization", AUTH_KEY);

  // Convert float variables to strings
  String tempStr = String(temp);
  String humStr = String(hum);

  // Send the data to the server
  int httpResponseCode = http.POST("{\"temp\": " + tempStr + ", \"hum\": " + humStr + "}");

  // Check the response code
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println(httpResponseCode);
    Serial.println(response);
  } else {
    Serial.println("Error on sending POST: " + http.errorToString(httpResponseCode));
  }
  stateScreen = 0;
  http.end();
}