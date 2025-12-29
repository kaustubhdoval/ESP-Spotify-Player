/*ESP Spotify Player V2.1
- By Kaustubh Doval

OLED: 128x64
  SCK -> D22
  SDA -> D21

Buttons
  previous button   -> D5
  play/pause button -> D18
  next button       -> D19

Rotary Encoder
  CLK -> D4
  DT  -> D2
  SW  -> D15

SETUP:
(1) Change SSID and Password (line 45)
(2) Change Client Secret and ID (49)
(3) Run the program, You can see your ESPs IP on Serial Monitor (9600 Baud Rate) and on the OLED.
    You need to add the IP address of your ESP to REDIRECT_URI definition (line 51):
          https://YOUR_ESP_IP/callback
(4) Add this callback to Spotify Web API portal as well (ensure that it is exactly the same as REDIRECT_URI)
(5) You are good to go!
*/

// Increase Header Sizes
#define HTTPS_REQUEST_MAX_REQUEST_LENGTH 8192
#define HTTPS_RESPONSE_MAX_RESPONSE_LENGTH 8192
#define HTTPS_REQUEST_MAX_HEADER_LENGTH 8192
#define HTTPS_HEADER_MAX_LENGTH 8192
#define HTTPS_CONNECTIONDATA_MAX_SIZE 16384
#define HTTPS_CONNECTION_DATA_CHUNK_SIZE 1024

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <ESP32Encoder.h>
#include <ArduinoJson.h>
#include <base64.h>
#include <ezButton.h>

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPSServer.hpp>
#include <SSLCert.hpp>
#include <HTTPRequest.hpp>
#include <HTTPResponse.hpp>

// Load Env Variables
#include "secrets.h"

// Load other Files
#include "index.h"

// Pin Definitions
#define PREV_BTN_PIN 5
#define PLAY_BTN_PIN 18
#define NEXT_BTN_PIN 19
#define ENC_CLK_PIN 4
#define ENC_DT_PIN 2
#define ENC_SW_PIN 15

// OLED Definitions
#define I2C_ADDRESS 0x3c
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

// Objects
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
ESP32Encoder encoder;

ezButton prevBtn(PREV_BTN_PIN);
ezButton playBtn(PLAY_BTN_PIN);
ezButton nextBtn(NEXT_BTN_PIN);
ezButton encSwBtn(ENC_SW_PIN);

// Timing variables
unsigned long lastApiRefresh = 0;

// Song details struct
struct SongDetails
{
  int durationMs;
  String album;
  String artist;
  String song;
  String Id;
  bool isLiked;
};

// Forward declarations
String truncateString(const String &input, int maxLength);
void handleButtons();
void handleVolumeControl();
void handleRoot();
void handleCallbackPage();
bool httpsRequest(
    const char *host,
    const char *path,
    const String &method,
    const String &headers,
    const String &body,
    String &responseBody);

using namespace httpsserver;

// Some Bitmap Definitions
static const unsigned char PROGMEM no_active_device[] = {0x07, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf8, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0xe0, 0x07, 0xce, 0x78, 0x03, 0x80, 0x00, 0x41, 0x02, 0x00, 0x01, 0x00, 0x7c, 0x01, 0xe0, 0x04, 0x11, 0x44, 0x04, 0x40, 0x00, 0x40, 0x05, 0x00, 0x01, 0x00, 0x44, 0x01, 0xe0, 0x04, 0x10, 0x44, 0x04, 0x16, 0x39, 0xf3, 0x04, 0x44, 0x01, 0x00, 0x7c, 0x01, 0xe0, 0x07, 0x8e, 0x78, 0x03, 0x99, 0x44, 0x41, 0x0e, 0x44, 0x01, 0x00, 0x7c, 0x01, 0xe0, 0x04, 0x01, 0x40, 0x00, 0x59, 0x44, 0x41, 0x04, 0x3c, 0x01, 0x00, 0x7c, 0x01, 0xe0, 0x04, 0x11, 0x40, 0x04, 0x56, 0x44, 0x51, 0x04, 0x04, 0x01, 0x00, 0x00, 0x01, 0xe0, 0x07, 0xce, 0x40, 0x03, 0x90, 0x38, 0x23, 0x84, 0x44, 0x01, 0x00, 0x00, 0x01, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x38, 0x01, 0x1f, 0xff, 0x9f, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x10, 0x00, 0x91, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xf0, 0x00, 0xf1, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0xe0, 0x00, 0x00, 0x0f, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x7c, 0x01, 0xe0, 0x00, 0x00, 0x08, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x7c, 0x01, 0xe0, 0x00, 0x00, 0x08, 0x88, 0x62, 0x27, 0x2c, 0x00, 0x00, 0x01, 0x00, 0x7c, 0x01, 0xe0, 0x00, 0x00, 0x0f, 0x08, 0x12, 0x28, 0xb2, 0x00, 0x00, 0x01, 0x00, 0x44, 0x01, 0xe0, 0x00, 0x00, 0x08, 0x08, 0x71, 0xef, 0xa0, 0x00, 0x00, 0x01, 0x00, 0x7c, 0x01, 0xe0, 0x00, 0x00, 0x08, 0x08, 0x90, 0x28, 0x20, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0xe0, 0x00, 0x00, 0x08, 0x1c, 0x7a, 0x27, 0x20, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x01, 0xc0, 0x00, 0x00, 0x00, 0x01, 0xff, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xff, 0xfe, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xff, 0xfc, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf8, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x20, 0x00, 0x20, 0x02, 0x08, 0x00, 0x00, 0x3c, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02, 0x20, 0x00, 0x50, 0x02, 0x00, 0x00, 0x00, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x27, 0x00, 0x89, 0xcf, 0x98, 0x89, 0xc0, 0x22, 0x72, 0x26, 0x1c, 0x70, 0x00, 0x02, 0xa8, 0x80, 0x8a, 0x22, 0x08, 0x8a, 0x20, 0x22, 0x8a, 0x22, 0x22, 0x88, 0x00, 0x02, 0x68, 0x80, 0xfa, 0x02, 0x08, 0x8b, 0xe0, 0x22, 0xfa, 0x22, 0x20, 0xf8, 0x00, 0x02, 0x28, 0x80, 0x8a, 0x22, 0x88, 0x52, 0x00, 0x22, 0x81, 0x42, 0x22, 0x80, 0x00, 0x02, 0x27, 0x00, 0x89, 0xc1, 0x1c, 0x21, 0xc0, 0x3c, 0x70, 0x87, 0x1c, 0x70, 0x00};
static const unsigned char PROGMEM splash_screen[] = {0x07, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf8, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0xe0, 0x07, 0xce, 0x78, 0x03, 0x80, 0x00, 0x41, 0x02, 0x00, 0x01, 0x00, 0x7c, 0x01, 0xe0, 0x04, 0x11, 0x44, 0x04, 0x40, 0x00, 0x40, 0x05, 0x00, 0x01, 0x00, 0x44, 0x01, 0xe0, 0x04, 0x10, 0x44, 0x04, 0x16, 0x39, 0xf3, 0x04, 0x44, 0x01, 0x00, 0x7c, 0x01, 0xe0, 0x07, 0x8e, 0x78, 0x03, 0x99, 0x44, 0x41, 0x0e, 0x44, 0x01, 0x00, 0x7c, 0x01, 0xe0, 0x04, 0x01, 0x40, 0x00, 0x59, 0x44, 0x41, 0x04, 0x3c, 0x01, 0x00, 0x7c, 0x01, 0xe0, 0x04, 0x11, 0x40, 0x04, 0x56, 0x44, 0x51, 0x04, 0x04, 0x01, 0x00, 0x00, 0x01, 0xe0, 0x07, 0xce, 0x40, 0x03, 0x90, 0x38, 0x23, 0x84, 0x44, 0x01, 0x00, 0x00, 0x01, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x38, 0x01, 0x1f, 0xff, 0x9f, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x10, 0x00, 0x91, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xf0, 0x00, 0xf1, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0xe0, 0x00, 0x00, 0x0f, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x7c, 0x01, 0xe0, 0x00, 0x00, 0x08, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x7c, 0x01, 0xe0, 0x00, 0x00, 0x08, 0x88, 0x62, 0x27, 0x2c, 0x00, 0x00, 0x01, 0x00, 0x7c, 0x01, 0xe0, 0x00, 0x00, 0x0f, 0x08, 0x12, 0x28, 0xb2, 0x00, 0x00, 0x01, 0x00, 0x44, 0x01, 0xe0, 0x00, 0x00, 0x08, 0x08, 0x71, 0xef, 0xa0, 0x00, 0x00, 0x01, 0x00, 0x7c, 0x01, 0xe0, 0x00, 0x00, 0x08, 0x08, 0x90, 0x28, 0x20, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0xe0, 0x00, 0x00, 0x08, 0x1c, 0x7a, 0x27, 0x20, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x01, 0xc0, 0x00, 0x00, 0x00, 0x01, 0xff, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xff, 0xfe, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xff, 0xfc, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf8, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x02, 0x20, 0x00, 0x00, 0x80, 0x20, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x80, 0x02, 0x40, 0x00, 0x00, 0x80, 0x20, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0xa2, 0x02, 0x86, 0x22, 0x7b, 0xe8, 0xac, 0xb0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x22, 0x03, 0x01, 0x22, 0x80, 0x88, 0xb2, 0xc8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x9e, 0x02, 0x87, 0x22, 0x70, 0x88, 0xa2, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x82, 0x02, 0x49, 0x26, 0x08, 0xa9, 0xb2, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x22, 0x02, 0x27, 0x9a, 0xf0, 0x46, 0xac, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const unsigned char PROGMEM configuring[] = {0x80, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc1, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x42, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x22, 0x8c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x13, 0x0c, 0x07, 0x00, 0x00, 0x42, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x0a, 0xb4, 0x08, 0x80, 0x00, 0xa0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x48, 0x08, 0x1c, 0xb0, 0x86, 0x1c, 0x8a, 0xc6, 0x2c, 0x70, 0x00, 0x00, 0x05, 0xf0, 0x08, 0x22, 0xc9, 0xc2, 0x26, 0x8b, 0x22, 0x32, 0x98, 0x00, 0x00, 0x0b, 0x00, 0x08, 0x22, 0x88, 0x82, 0x26, 0x8a, 0x02, 0x22, 0x98, 0x00, 0x00, 0x14, 0xe0, 0x08, 0xa2, 0x88, 0x82, 0x1a, 0x9a, 0x02, 0x22, 0x68, 0xc3, 0x0c, 0x29, 0xb0, 0x07, 0x1c, 0x88, 0x87, 0x02, 0x6a, 0x07, 0x22, 0x08, 0xc3, 0x0c, 0x50, 0xd8, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x70, 0x00, 0x00, 0xa0, 0x6c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

const char *spotify_root_ca PROGMEM =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIEyDCCA7CgAwIBAgIQDPW9BitWAvR6uFAsI8zwZjANBgkqhkiG9w0BAQsFADBh\n"
    "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n"
    "d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH\n"
    "MjAeFw0yMTAzMzAwMDAwMDBaFw0zMTAzMjkyMzU5NTlaMFkxCzAJBgNVBAYTAlVT\n"
    "MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxMzAxBgNVBAMTKkRpZ2lDZXJ0IEdsb2Jh\n"
    "bCBHMiBUTFMgUlNBIFNIQTI1NiAyMDIwIENBMTCCASIwDQYJKoZIhvcNAQEBBQAD\n"
    "ggEPADCCAQoCggEBAMz3EGJPprtjb+2QUlbFbSd7ehJWivH0+dbn4Y+9lavyYEEV\n"
    "cNsSAPonCrVXOFt9slGTcZUOakGUWzUb+nv6u8W+JDD+Vu/E832X4xT1FE3LpxDy\n"
    "FuqrIvAxIhFhaZAmunjZlx/jfWardUSVc8is/+9dCopZQ+GssjoP80j812s3wWPc\n"
    "3kbW20X+fSP9kOhRBx5Ro1/tSUZUfyyIxfQTnJcVPAPooTncaQwywa8WV0yUR0J8\n"
    "osicfebUTVSvQpmowQTCd5zWSOTOEeAqgJnwQ3DPP3Zr0UxJqyRewg2C/Uaoq2yT\n"
    "zGJSQnWS+Jr6Xl6ysGHlHx+5fwmY6D36g39HaaECAwEAAaOCAYIwggF+MBIGA1Ud\n"
    "EwEB/wQIMAYBAf8CAQAwHQYDVR0OBBYEFHSFgMBmx9833s+9KTeqAx2+7c0XMB8G\n"
    "A1UdIwQYMBaAFE4iVCAYlebjbuYP+vq5Eu0GF485MA4GA1UdDwEB/wQEAwIBhjAd\n"
    "BgNVHSUEFjAUBggrBgEFBQcDAQYIKwYBBQUHAwIwdgYIKwYBBQUHAQEEajBoMCQG\n"
    "CCsGAQUFBzABhhhodHRwOi8vb2NzcC5kaWdpY2VydC5jb20wQAYIKwYBBQUHMAKG\n"
    "NGh0dHA6Ly9jYWNlcnRzLmRpZ2ljZXJ0LmNvbS9EaWdpQ2VydEdsb2JhbFJvb3RH\n"
    "Mi5jcnQwQgYDVR0fBDswOTA3oDWgM4YxaHR0cDovL2NybDMuZGlnaWNlcnQuY29t\n"
    "L0RpZ2lDZXJ0R2xvYmFsUm9vdEcyLmNybDA9BgNVHSAENjA0MAsGCWCGSAGG/WwC\n"
    "ATAHBgVngQwBATAIBgZngQwBAgEwCAYGZ4EMAQICMAgGBmeBDAECAzANBgkqhkiG\n"
    "9w0BAQsFAAOCAQEAkPFwyyiXaZd8dP3A+iZ7U6utzWX9upwGnIrXWkOH7U1MVl+t\n"
    "wcW1BSAuWdH/SvWgKtiwla3JLko716f2b4gp/DA/JIS7w7d7kwcsr4drdjPtAFVS\n"
    "slme5LnQ89/nD/7d+MS5EHKBCQRfz5eeLjJ1js+aWNJXMX43AYGyZm0pGrFmCW3R\n"
    "bpD0ufovARTFXFZkAdl9h6g4U5+LXUZtXMYnhIHUfoyMo5tS58aI7Dd8KvvwVVo4\n"
    "chDYABPPTHPbqjc1qCmBaZx2vN4Ye5DUys/vZwP9BFohFrH/6j/f3IL16/RZkiMN\n"
    "JCqVJUzKoZHm1Lesh3Sz8W2jmdv51b2EQJ8HmA==\n"
    "-----END CERTIFICATE-----\n";

// Spotify Connection Class
class SpotConn
{
public:
  SpotConn()
  {
    // Constructor
  }

  bool getUserCode(const String &serverCode)
  {
    JsonDocument doc;
    String response;

    String auth = "Basic " + base64::encode(
                                 String(CLIENT_ID) + ":" + String(CLIENT_SECRET));

    String headers =
        "Authorization: " + auth + "\r\n"
                                   "Content-Type: application/x-www-form-urlencoded\r\n";

    String body =
        "grant_type=authorization_code"
        "&code=" +
        serverCode +
        "&redirect_uri=" + String(REDIRECT_URI);

    bool ok = httpsRequest(
        "accounts.spotify.com",
        "/api/token",
        "POST",
        headers,
        body,
        response);

    if (!ok)
    {
      Serial.println("HTTPS request failed");
      return false;
    }

    DeserializationError error = deserializeJson(doc, response);
    if (error)
    {
      Serial.print("JSON parse failed: ");
      Serial.println(error.c_str());
      return false;
    }

    accessToken = doc["access_token"].as<String>();
    refreshToken = doc["refresh_token"].as<String>();
    tokenExpireTime = doc["expires_in"].as<int>();
    tokenStartTime = millis();
    accessTokenSet = true;

    Serial.println("Access token: " + accessToken);
    Serial.println("Refresh token: " + refreshToken);

    return true;
  }

  bool refreshAuth()
  {
    JsonDocument doc;
    String response;

    accessTokenSet = false;

    String auth = "Basic " + base64::encode(
                                 String(CLIENT_ID) + ":" + String(CLIENT_SECRET));

    String headers =
        "Authorization: " + auth + "\r\n"
                                   "Content-Type: application/x-www-form-urlencoded\r\n";

    String body =
        "grant_type=refresh_token"
        "&refresh_token=" +
        refreshToken;

    bool ok = httpsRequest(
        "accounts.spotify.com",
        "/api/token",
        "POST",
        headers,
        body,
        response);

    if (!ok)
    {
      Serial.println("HTTPS refresh request failed");
      return false;
    }

    DeserializationError error = deserializeJson(doc, response);
    if (error)
    {
      Serial.print("JSON parse failed: ");
      Serial.println(error.c_str());
      return false;
    }

    accessToken = doc["access_token"].as<String>();

    // Optional refresh_token (Spotify sometimes omits it)
    if (!doc["refresh_token"].isNull())
    {
      refreshToken = doc["refresh_token"].as<String>();
    }

    tokenExpireTime = doc["expires_in"].as<int>();
    tokenStartTime = millis();
    accessTokenSet = true;

    Serial.println("Refreshed access token: " + accessToken);
    return true;
  }

  bool getTrackInfo()
  {
    JsonDocument doc;
    String response;
    bool success = false;

    String headers =
        "Authorization: Bearer " + accessToken + "\r\n";

    bool ok = httpsRequest(
        "api.spotify.com",
        "/v1/me/player",
        "GET",
        headers,
        "",
        response);

    if (!ok)
    {
      Serial.println("HTTPS player request failed");
      return false;
    }

    // Spotify returns 204 with an empty body
    if (response.length() == 0)
    {
      Serial.println("NOTE: No Active Device or No Song Playing.");
      isActive = false;
      volCtrl = false;
      success = true;
      drawScreen();
      return true;
    }

    DeserializationError error = deserializeJson(doc, response);
    if (error)
    {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return false;
    }

    // -------- DEVICE INFO --------
    if (!doc["device"].isNull())
    {
      JsonObject device = doc["device"];
      isActive = device["is_active"].as<bool>();
      volCtrl = device["supports_volume"].as<bool>();
      volume = device["volume_percent"].as<int>();

      encoder.setCount(volume / 5);
      Serial.print("Spotify Status: ");
      Serial.println(isActive);
    }
    else
    {
      isActive = false;
      volCtrl = false;
    }

    // -------- PROGRESS --------
    currentSongPositionMs =
        doc["progress_ms"].is<int>()
            ? doc["progress_ms"].as<int>()
            : 0;

    // -------- SONG ITEM --------
    if (!doc["item"].isNull())
    {
      JsonObject item = doc["item"];

      // Artist
      if (!item["artists"].isNull())
      {
        JsonArray artists = item["artists"].as<JsonArray>();
        currentSong.artist =
            (!artists.isNull() && artists.size() > 0)
                ? artists[0]["name"].as<String>()
                : "Unknown Artist";
      }
      else
      {
        currentSong.artist = "Unknown Artist";
      }

      // Song name
      currentSong.song = item["name"].as<String>();

      // Duration
      currentSong.durationMs = item["duration_ms"].as<int>();

      // Track ID
      String songId = item["uri"].as<String>();
      if (songId.startsWith("spotify:track:"))
      {
        songId = songId.substring(14);
      }
      currentSong.Id = songId;
    }
    else
    {
      currentSong.artist = "Unknown Artist";
      currentSong.song = "No Song Playing";
      currentSong.durationMs = 0;
      currentSong.Id = "";
    }

    // -------- PLAYBACK STATE --------
    isPlaying =
        doc["is_playing"].is<bool>()
            ? doc["is_playing"].as<bool>()
            : false;

    lastSongPositionMs = currentSongPositionMs;
    success = true;

    if (success)
    {
      drawScreen();
    }

    return success;
  }

  bool drawScreen()
  {
    display.clearDisplay();

    if (!isActive)
    {
      // Show the no active device screen
      display.drawBitmap(9, 8, no_active_device, 112, 49, 1);
      display.display();
      return true;
    }

    // Display content for active device
    display.setTextColor(SH110X_WHITE);
    display.setTextSize(1);

    // Truncate long strings
    String songName = truncateString(currentSong.song, 20);
    String artistName = truncateString(currentSong.artist, 20);

    // Display song info
    display.setCursor(0, 0);
    display.print(songName);

    display.setCursor(0, 15);
    display.print(artistName);

    // Draw progress bar
    int barY = 30;
    int barHeight = 8;
    int barWidth = map(currentSongPositionMs, 0, currentSong.durationMs, 0, SCREEN_WIDTH);

    display.drawRect(0, barY, SCREEN_WIDTH, barHeight, SH110X_WHITE);
    display.fillRect(0, barY, barWidth, barHeight, SH110X_WHITE);

    // Display play/pause state
    display.setCursor(45, 55);
    display.print(isPlaying ? "Playing" : "Paused");

    // Display volume
    display.setCursor(110, 55);
    display.print(volCtrl ? String(volume) : "N/A");

    display.display();
    return true;
  }

  bool togglePlay()
  {
    String path = "/v1/me/player/";
    path += isPlaying ? "pause" : "play";

    String headers =
        "Authorization: Bearer " + accessToken + "\r\n";

    String response;

    bool ok = httpsRequest(
        "api.spotify.com",
        path.c_str(),
        "PUT",
        headers,
        "",
        response);

    if (ok)
    {
      isPlaying = !isPlaying;
      Serial.println(isPlaying ? "Now playing" : "Now paused");
    }
    else
    {
      Serial.println("Error toggling playback");
    }

    getTrackInfo();
    return ok;
  }

  bool adjustVolume(int vol)
  {
    vol = constrain(vol, 0, 100);

    String path =
        "/v1/me/player/volume?volume_percent=" + String(vol);

    String headers =
        "Authorization: Bearer " + accessToken + "\r\n";

    String response;

    bool ok = httpsRequest(
        "api.spotify.com",
        path.c_str(),
        "PUT",
        headers,
        "",
        response);

    if (ok)
    {
      currVol = vol;
      Serial.println("Volume set to: " + String(vol));
    }
    else
    {
      Serial.println("Error setting volume");
    }

    getTrackInfo();
    return ok;
  }

  bool skipForward()
  {
    String headers =
        "Authorization: Bearer " + accessToken + "\r\n";

    String response;

    bool ok = httpsRequest(
        "api.spotify.com",
        "/v1/me/player/next",
        "POST",
        headers,
        "",
        response);

    if (ok)
    {
      Serial.println("Skipped to next track");
    }
    else
    {
      Serial.println("Error skipping forward");
    }

    getTrackInfo();
    return ok;
  }

  bool skipBack()
  {
    String headers =
        "Authorization: Bearer " + accessToken + "\r\n";

    String response;

    bool ok = httpsRequest(
        "api.spotify.com",
        "/v1/me/player/previous",
        "POST",
        headers,
        "",
        response);

    if (ok)
    {
      Serial.println("Skipped to previous track");
    }
    else
    {
      Serial.println("Error skipping backward");
    }

    getTrackInfo();
    return ok;
  }

  // Public variables
  bool accessTokenSet = false;
  unsigned long tokenStartTime;
  int tokenExpireTime;
  SongDetails currentSong;
  float currentSongPositionMs;
  float lastSongPositionMs;
  int currVol;
  bool isPlaying;
  bool isActive = false;
  bool volCtrl = false;
  int volume;

private:
  String accessToken;
  String refreshToken;
};

// Global Spotify Connection instance
SpotConn spotifyConnection;

SSLCert *cert;
HTTPSServer *secureServer;
WiFiClientSecure secureClient;

// HTTPS Connection Request Helper
bool httpsRequest(
    const char *host,
    const char *path,
    const String &method,
    const String &headers,
    const String &body,
    String &responseBody)
{
  secureClient.setCACert(spotify_root_ca);
  secureClient.setTimeout(10000); // 10 second timeout
  secureClient.setHandshakeTimeout(10000);

  if (!secureClient.connect(host, 443))
  {
    Serial.println("HTTPS connection failed");
    return false;
  }

  // ---- Request line ----
  secureClient.print(method + " " + path + " HTTP/1.1\r\n");
  secureClient.print("Host: " + String(host) + "\r\n");
  secureClient.print("User-Agent: ESP32\r\n");
  secureClient.print("Connection: close\r\n");

  // ---- Custom headers ----
  secureClient.print(headers);

  // ---- Content-Length if body exists ----
  if (body.length() > 0)
  {
    secureClient.print("Content-Length: ");
    secureClient.print(body.length());
    secureClient.print("\r\n");
  }

  secureClient.print("\r\n");

  // ---- Body ----
  if (body.length() > 0)
  {
    secureClient.print(body);
  }

  // ---- Read headers ----
  while (secureClient.connected())
  {
    String line = secureClient.readStringUntil('\n');
    if (line == "\r")
      break;
  }

  // ---- Read body ----
  responseBody = "";
  while (secureClient.available())
  {
    responseBody += secureClient.readString();
  }

  secureClient.stop();
  return true;
}

// Web server handlers
void handleRoot(HTTPRequest *req, HTTPResponse *res)
{
  Serial.println("Handling root request");

  // Static buffer (safer than stack allocation)
  static char page[8192];

  // Generate the page
  if (snprintf(page, sizeof(page), mainPage, CLIENT_ID, REDIRECT_URI) < 0)
  {
    res->setStatusCode(500);
    res->setStatusText("Internal Server Error");
    res->println("Failed to generate page");
    return;
  }

  // Set headers and send response
  res->setHeader("Content-Type", "text/html");
  res->println(page);
}

void handleCallbackPage(HTTPRequest *req, HTTPResponse *res)
{
  // Debug logging
  Serial.println("=== CALLBACK HEADERS ===");
  Serial.print("Request line: ");
  Serial.println(req->getRequestString().c_str());

  // Get the "code" parameter from query string
  std::string code;
  bool hasCode = req->getParams()->getQueryParameter("code", code);

  if (!spotifyConnection.accessTokenSet)
  {
    if (code.empty())
    {
      static char page[8192];
      if (snprintf(page, sizeof(page), errorPage, CLIENT_ID, REDIRECT_URI) < 0)
      {
        res->setStatusCode(500);
        res->setStatusText("Internal Server Error");
        res->println("Error page generation failed");
        return;
      }
      res->setHeader("Content-Type", "text/html");
      res->println(page);
    }
    else
    { // Convert std::string to String
      String codeStr = String(code.c_str());
      if (spotifyConnection.getUserCode(codeStr))
      {
        res->setHeader("Content-Type", "text/plain");
        res->print("Spotify setup complete. Refresh in: ");
        res->print(String(spotifyConnection.tokenExpireTime));
      }
      else
      {
        static char page[800];
        if (snprintf(page, sizeof(page), errorPage, CLIENT_ID, REDIRECT_URI) < 0)
        {
          res->setStatusCode(500);
          res->setStatusText("Internal Server Error");
          res->println("Error page generation failed");
          return;
        }
        res->setHeader("Content-Type", "text/html");
        res->println(page);
      }
    }
  }
  else
  {
    res->setHeader("Content-Type", "text/plain");
    res->println("Already authenticated");
  }
}

void handle404(HTTPRequest *req, HTTPResponse *res)
{
  // Discard request body if any
  req->discardRequestBody();

  // Set response status
  res->setStatusCode(404);
  res->setStatusText("Not Found");

  // Set content type
  res->setHeader("Content-Type", "text/html");

  // Write 404 page
  res->println("<!DOCTYPE html>");
  res->println("<html>");
  res->println("<head><title>Not Found</title></head>");
  res->println("<body><h1>404 Not Found</h1><p>The requested resource was not found on this server.</p></body>");
  res->println("</html>");
}

// Helper function to truncate strings
String truncateString(const String &input, int maxLength)
{
  if (input.length() <= maxLength)
    return input;
  return input.substring(0, maxLength - 3) + "...";
}

// Button handler function
void handleButtons()
{
  if (prevBtn.isPressed())
  {
    Serial.println("Previous button pressed");
    spotifyConnection.skipBack();
  }

  else if (playBtn.isPressed())
  {
    Serial.println("Play button pressed");
    spotifyConnection.togglePlay();
  }

  else if (nextBtn.isPressed())
  {
    Serial.println("Next button pressed");
    spotifyConnection.skipForward();
  }/*ESP Spotify Player V2.1
- By Kaustubh Doval

OLED: 128x64
  SCK -> D22
  SDA -> D21

Buttons
  previous button   -> D5
  play/pause button -> D18
  next button       -> D19

Rotary Encoder
  CLK -> D4
  DT  -> D2
  SW  -> D15

SETUP:
(1) Change SSID and Password (line 45)
(2) Change Client Secret and ID (49)
(3) Run the program, You can see your ESPs IP on Serial Monitor (9600 Baud Rate) and on the OLED.
    You need to add the IP address of your ESP to REDIRECT_URI definition (line 51):
          https://YOUR_ESP_IP/callback
(4) Add this callback to Spotify Web API portal as well (ensure that it is exactly the same as REDIRECT_URI)
(5) You are good to go!
*/

// Increase Header Sizes
#undef HTTPS_CONNECTION_DATA_CHUNK_SIZE
#define HTTPS_CONNECTION_DATA_CHUNK_SIZE 4096

#undef HTTPS_REQUEST_MAX_REQUEST_LENGTH
#define HTTPS_REQUEST_MAX_REQUEST_LENGTH 8192

#undef HTTPS_MAX_HEADER_LENGTH
#define HTTPS_MAX_HEADER_LENGTH 4096

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <ESP32Encoder.h>
#include <ArduinoJson.h>
#include <base64.h>
#include <ezButton.h>

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPSServer.hpp>
#include <SSLCert.hpp>
#include <HTTPRequest.hpp>
#include <HTTPResponse.hpp>

// Load Env Variables
#include "secrets.h"

// Load other Files
#include "index.h"

// Pin Definitions
#define PREV_BTN_PIN 5
#define PLAY_BTN_PIN 18
#define NEXT_BTN_PIN 19
#define ENC_CLK_PIN 4
#define ENC_DT_PIN 2
#define ENC_SW_PIN 15

// OLED Definitions
#define I2C_ADDRESS 0x3c
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

// Objects
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
ESP32Encoder encoder;

ezButton prevBtn(PREV_BTN_PIN);
ezButton playBtn(PLAY_BTN_PIN);
ezButton nextBtn(NEXT_BTN_PIN);
ezButton encSwBtn(ENC_SW_PIN);

// Timing variables
unsigned long lastApiRefresh = 0;

// Song details struct
struct SongDetails
{
  int durationMs;
  String album;
  String artist;
  String song;
  String Id;
  bool isLiked;
};

// Forward declarations
String truncateString(const String &input, int maxLength);
void handleButtons();
void handleVolumeControl();
void handleRoot();
void handleCallbackPage();
bool httpsRequest(
    const char *host,
    const char *path,
    const String &method,
    const String &headers,
    const String &body,
    String &responseBody);

using namespace httpsserver;

// Some Bitmap Definitions
static const unsigned char PROGMEM no_active_device[] = {0x07, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf8, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0xe0, 0x07, 0xce, 0x78, 0x03, 0x80, 0x00, 0x41, 0x02, 0x00, 0x01, 0x00, 0x7c, 0x01, 0xe0, 0x04, 0x11, 0x44, 0x04, 0x40, 0x00, 0x40, 0x05, 0x00, 0x01, 0x00, 0x44, 0x01, 0xe0, 0x04, 0x10, 0x44, 0x04, 0x16, 0x39, 0xf3, 0x04, 0x44, 0x01, 0x00, 0x7c, 0x01, 0xe0, 0x07, 0x8e, 0x78, 0x03, 0x99, 0x44, 0x41, 0x0e, 0x44, 0x01, 0x00, 0x7c, 0x01, 0xe0, 0x04, 0x01, 0x40, 0x00, 0x59, 0x44, 0x41, 0x04, 0x3c, 0x01, 0x00, 0x7c, 0x01, 0xe0, 0x04, 0x11, 0x40, 0x04, 0x56, 0x44, 0x51, 0x04, 0x04, 0x01, 0x00, 0x00, 0x01, 0xe0, 0x07, 0xce, 0x40, 0x03, 0x90, 0x38, 0x23, 0x84, 0x44, 0x01, 0x00, 0x00, 0x01, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x38, 0x01, 0x1f, 0xff, 0x9f, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x10, 0x00, 0x91, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xf0, 0x00, 0xf1, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0xe0, 0x00, 0x00, 0x0f, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x7c, 0x01, 0xe0, 0x00, 0x00, 0x08, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x7c, 0x01, 0xe0, 0x00, 0x00, 0x08, 0x88, 0x62, 0x27, 0x2c, 0x00, 0x00, 0x01, 0x00, 0x7c, 0x01, 0xe0, 0x00, 0x00, 0x0f, 0x08, 0x12, 0x28, 0xb2, 0x00, 0x00, 0x01, 0x00, 0x44, 0x01, 0xe0, 0x00, 0x00, 0x08, 0x08, 0x71, 0xef, 0xa0, 0x00, 0x00, 0x01, 0x00, 0x7c, 0x01, 0xe0, 0x00, 0x00, 0x08, 0x08, 0x90, 0x28, 0x20, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0xe0, 0x00, 0x00, 0x08, 0x1c, 0x7a, 0x27, 0x20, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x01, 0xc0, 0x00, 0x00, 0x00, 0x01, 0xff, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xff, 0xfe, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xff, 0xfc, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf8, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x20, 0x00, 0x20, 0x02, 0x08, 0x00, 0x00, 0x3c, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02, 0x20, 0x00, 0x50, 0x02, 0x00, 0x00, 0x00, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x27, 0x00, 0x89, 0xcf, 0x98, 0x89, 0xc0, 0x22, 0x72, 0x26, 0x1c, 0x70, 0x00, 0x02, 0xa8, 0x80, 0x8a, 0x22, 0x08, 0x8a, 0x20, 0x22, 0x8a, 0x22, 0x22, 0x88, 0x00, 0x02, 0x68, 0x80, 0xfa, 0x02, 0x08, 0x8b, 0xe0, 0x22, 0xfa, 0x22, 0x20, 0xf8, 0x00, 0x02, 0x28, 0x80, 0x8a, 0x22, 0x88, 0x52, 0x00, 0x22, 0x81, 0x42, 0x22, 0x80, 0x00, 0x02, 0x27, 0x00, 0x89, 0xc1, 0x1c, 0x21, 0xc0, 0x3c, 0x70, 0x87, 0x1c, 0x70, 0x00};
static const unsigned char PROGMEM splash_screen[] = {0x07, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf8, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0xe0, 0x07, 0xce, 0x78, 0x03, 0x80, 0x00, 0x41, 0x02, 0x00, 0x01, 0x00, 0x7c, 0x01, 0xe0, 0x04, 0x11, 0x44, 0x04, 0x40, 0x00, 0x40, 0x05, 0x00, 0x01, 0x00, 0x44, 0x01, 0xe0, 0x04, 0x10, 0x44, 0x04, 0x16, 0x39, 0xf3, 0x04, 0x44, 0x01, 0x00, 0x7c, 0x01, 0xe0, 0x07, 0x8e, 0x78, 0x03, 0x99, 0x44, 0x41, 0x0e, 0x44, 0x01, 0x00, 0x7c, 0x01, 0xe0, 0x04, 0x01, 0x40, 0x00, 0x59, 0x44, 0x41, 0x04, 0x3c, 0x01, 0x00, 0x7c, 0x01, 0xe0, 0x04, 0x11, 0x40, 0x04, 0x56, 0x44, 0x51, 0x04, 0x04, 0x01, 0x00, 0x00, 0x01, 0xe0, 0x07, 0xce, 0x40, 0x03, 0x90, 0x38, 0x23, 0x84, 0x44, 0x01, 0x00, 0x00, 0x01, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x38, 0x01, 0x1f, 0xff, 0x9f, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x10, 0x00, 0x91, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xf0, 0x00, 0xf1, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0xe0, 0x00, 0x00, 0x0f, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x7c, 0x01, 0xe0, 0x00, 0x00, 0x08, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x7c, 0x01, 0xe0, 0x00, 0x00, 0x08, 0x88, 0x62, 0x27, 0x2c, 0x00, 0x00, 0x01, 0x00, 0x7c, 0x01, 0xe0, 0x00, 0x00, 0x0f, 0x08, 0x12, 0x28, 0xb2, 0x00, 0x00, 0x01, 0x00, 0x44, 0x01, 0xe0, 0x00, 0x00, 0x08, 0x08, 0x71, 0xef, 0xa0, 0x00, 0x00, 0x01, 0x00, 0x7c, 0x01, 0xe0, 0x00, 0x00, 0x08, 0x08, 0x90, 0x28, 0x20, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0xe0, 0x00, 0x00, 0x08, 0x1c, 0x7a, 0x27, 0x20, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x01, 0xc0, 0x00, 0x00, 0x00, 0x01, 0xff, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xff, 0xfe, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xff, 0xfc, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf8, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x02, 0x20, 0x00, 0x00, 0x80, 0x20, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x80, 0x02, 0x40, 0x00, 0x00, 0x80, 0x20, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0xa2, 0x02, 0x86, 0x22, 0x7b, 0xe8, 0xac, 0xb0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x22, 0x03, 0x01, 0x22, 0x80, 0x88, 0xb2, 0xc8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x9e, 0x02, 0x87, 0x22, 0x70, 0x88, 0xa2, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x82, 0x02, 0x49, 0x26, 0x08, 0xa9, 0xb2, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x22, 0x02, 0x27, 0x9a, 0xf0, 0x46, 0xac, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const unsigned char PROGMEM configuring[] = {0x80, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc1, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x42, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x22, 0x8c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x13, 0x0c, 0x07, 0x00, 0x00, 0x42, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x0a, 0xb4, 0x08, 0x80, 0x00, 0xa0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x48, 0x08, 0x1c, 0xb0, 0x86, 0x1c, 0x8a, 0xc6, 0x2c, 0x70, 0x00, 0x00, 0x05, 0xf0, 0x08, 0x22, 0xc9, 0xc2, 0x26, 0x8b, 0x22, 0x32, 0x98, 0x00, 0x00, 0x0b, 0x00, 0x08, 0x22, 0x88, 0x82, 0x26, 0x8a, 0x02, 0x22, 0x98, 0x00, 0x00, 0x14, 0xe0, 0x08, 0xa2, 0x88, 0x82, 0x1a, 0x9a, 0x02, 0x22, 0x68, 0xc3, 0x0c, 0x29, 0xb0, 0x07, 0x1c, 0x88, 0x87, 0x02, 0x6a, 0x07, 0x22, 0x08, 0xc3, 0x0c, 0x50, 0xd8, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x70, 0x00, 0x00, 0xa0, 0x6c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

const char *spotify_root_ca PROGMEM =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIEyDCCA7CgAwIBAgIQDPW9BitWAvR6uFAsI8zwZjANBgkqhkiG9w0BAQsFADBh\n"
    "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n"
    "d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH\n"
    "MjAeFw0yMTAzMzAwMDAwMDBaFw0zMTAzMjkyMzU5NTlaMFkxCzAJBgNVBAYTAlVT\n"
    "MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxMzAxBgNVBAMTKkRpZ2lDZXJ0IEdsb2Jh\n"
    "bCBHMiBUTFMgUlNBIFNIQTI1NiAyMDIwIENBMTCCASIwDQYJKoZIhvcNAQEBBQAD\n"
    "ggEPADCCAQoCggEBAMz3EGJPprtjb+2QUlbFbSd7ehJWivH0+dbn4Y+9lavyYEEV\n"
    "cNsSAPonCrVXOFt9slGTcZUOakGUWzUb+nv6u8W+JDD+Vu/E832X4xT1FE3LpxDy\n"
    "FuqrIvAxIhFhaZAmunjZlx/jfWardUSVc8is/+9dCopZQ+GssjoP80j812s3wWPc\n"
    "3kbW20X+fSP9kOhRBx5Ro1/tSUZUfyyIxfQTnJcVPAPooTncaQwywa8WV0yUR0J8\n"
    "osicfebUTVSvQpmowQTCd5zWSOTOEeAqgJnwQ3DPP3Zr0UxJqyRewg2C/Uaoq2yT\n"
    "zGJSQnWS+Jr6Xl6ysGHlHx+5fwmY6D36g39HaaECAwEAAaOCAYIwggF+MBIGA1Ud\n"
    "EwEB/wQIMAYBAf8CAQAwHQYDVR0OBBYEFHSFgMBmx9833s+9KTeqAx2+7c0XMB8G\n"
    "A1UdIwQYMBaAFE4iVCAYlebjbuYP+vq5Eu0GF485MA4GA1UdDwEB/wQEAwIBhjAd\n"
    "BgNVHSUEFjAUBggrBgEFBQcDAQYIKwYBBQUHAwIwdgYIKwYBBQUHAQEEajBoMCQG\n"
    "CCsGAQUFBzABhhhodHRwOi8vb2NzcC5kaWdpY2VydC5jb20wQAYIKwYBBQUHMAKG\n"
    "NGh0dHA6Ly9jYWNlcnRzLmRpZ2ljZXJ0LmNvbS9EaWdpQ2VydEdsb2JhbFJvb3RH\n"
    "Mi5jcnQwQgYDVR0fBDswOTA3oDWgM4YxaHR0cDovL2NybDMuZGlnaWNlcnQuY29t\n"
    "L0RpZ2lDZXJ0R2xvYmFsUm9vdEcyLmNybDA9BgNVHSAENjA0MAsGCWCGSAGG/WwC\n"
    "ATAHBgVngQwBATAIBgZngQwBAgEwCAYGZ4EMAQICMAgGBmeBDAECAzANBgkqhkiG\n"
    "9w0BAQsFAAOCAQEAkPFwyyiXaZd8dP3A+iZ7U6utzWX9upwGnIrXWkOH7U1MVl+t\n"
    "wcW1BSAuWdH/SvWgKtiwla3JLko716f2b4gp/DA/JIS7w7d7kwcsr4drdjPtAFVS\n"
    "slme5LnQ89/nD/7d+MS5EHKBCQRfz5eeLjJ1js+aWNJXMX43AYGyZm0pGrFmCW3R\n"
    "bpD0ufovARTFXFZkAdl9h6g4U5+LXUZtXMYnhIHUfoyMo5tS58aI7Dd8KvvwVVo4\n"
    "chDYABPPTHPbqjc1qCmBaZx2vN4Ye5DUys/vZwP9BFohFrH/6j/f3IL16/RZkiMN\n"
    "JCqVJUzKoZHm1Lesh3Sz8W2jmdv51b2EQJ8HmA==\n"
    "-----END CERTIFICATE-----\n";

// Spotify Connection Class
class SpotConn
{
public:
  SpotConn()
  {
    // Constructor
  }

  bool getUserCode(const String &serverCode)
  {
    JsonDocument doc;
    String response;

    String auth = "Basic " + base64::encode(
                                 String(CLIENT_ID) + ":" + String(CLIENT_SECRET));

    String headers =
        "Authorization: " + auth + "\r\n"
                                   "Content-Type: application/x-www-form-urlencoded\r\n";

    String body =
        "grant_type=authorization_code"
        "&code=" +
        serverCode +
        "&redirect_uri=" + String(REDIRECT_URI);

    bool ok = httpsRequest(
        "accounts.spotify.com",
        "/api/token",
        "POST",
        headers,
        body,
        response);

    if (!ok)
    {
      Serial.println("HTTPS request failed");
      return false;
    }

    DeserializationError error = deserializeJson(doc, response);
    if (error)
    {
      Serial.print("JSON parse failed: ");
      Serial.println(error.c_str());
      return false;
    }

    accessToken = doc["access_token"].as<String>();
    refreshToken = doc["refresh_token"].as<String>();
    tokenExpireTime = doc["expires_in"].as<int>();
    tokenStartTime = millis();
    accessTokenSet = true;

    Serial.println("Access token: " + accessToken);
    Serial.println("Refresh token: " + refreshToken);

    return true;
  }

  bool refreshAuth()
  {
    JsonDocument doc;
    String response;

    accessTokenSet = false;

    String auth = "Basic " + base64::encode(
                                 String(CLIENT_ID) + ":" + String(CLIENT_SECRET));

    String headers =
        "Authorization: " + auth + "\r\n"
                                   "Content-Type: application/x-www-form-urlencoded\r\n";

    String body =
        "grant_type=refresh_token"
        "&refresh_token=" +
        refreshToken;

    bool ok = httpsRequest(
        "accounts.spotify.com",
        "/api/token",
        "POST",
        headers,
        body,
        response);

    if (!ok)
    {
      Serial.println("HTTPS refresh request failed");
      return false;
    }

    DeserializationError error = deserializeJson(doc, response);
    if (error)
    {
      Serial.print("JSON parse failed: ");
      Serial.println(error.c_str());
      return false;
    }

    accessToken = doc["access_token"].as<String>();

    // Optional refresh_token (Spotify sometimes omits it)
    if (!doc["refresh_token"].isNull())
    {
      refreshToken = doc["refresh_token"].as<String>();
    }

    tokenExpireTime = doc["expires_in"].as<int>();
    tokenStartTime = millis();
    accessTokenSet = true;

    Serial.println("Refreshed access token: " + accessToken);
    return true;
  }

  bool getTrackInfo()
  {
    JsonDocument doc;
    String response;
    bool success = false;

    String headers =
        "Authorization: Bearer " + accessToken + "\r\n";

    bool ok = httpsRequest(
        "api.spotify.com",
        "/v1/me/player",
        "GET",
        headers,
        "",
        response);

    if (!ok)
    {
      Serial.println("HTTPS player request failed");
      return false;
    }

    // Spotify returns 204 with an empty body
    if (response.length() == 0)
    {
      Serial.println("NOTE: No Active Device or No Song Playing.");
      isActive = false;
      volCtrl = false;
      success = true;
      drawScreen();
      return true;
    }

    DeserializationError error = deserializeJson(doc, response);
    if (error)
    {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return false;
    }

    // -------- DEVICE INFO --------
    if (!doc["device"].isNull())
    {
      JsonObject device = doc["device"];
      isActive = device["is_active"].as<bool>();
      volCtrl = device["supports_volume"].as<bool>();
      volume = device["volume_percent"].as<int>();

      encoder.setCount(volume / 5);
      Serial.print("Spotify Status: ");
      Serial.println(isActive);
    }
    else
    {
      isActive = false;
      volCtrl = false;
    }

    // -------- PROGRESS --------
    currentSongPositionMs =
        doc["progress_ms"].is<int>()
            ? doc["progress_ms"].as<int>()
            : 0;

    // -------- SONG ITEM --------
    if (!doc["item"].isNull())
    {
      JsonObject item = doc["item"];

      // Artist
      if (!item["artists"].isNull())
      {
        JsonArray artists = item["artists"].as<JsonArray>();
        currentSong.artist =
            (!artists.isNull() && artists.size() > 0)
                ? artists[0]["name"].as<String>()
                : "Unknown Artist";
      }
      else
      {
        currentSong.artist = "Unknown Artist";
      }

      // Song name
      currentSong.song = item["name"].as<String>();

      // Duration
      currentSong.durationMs = item["duration_ms"].as<int>();

      // Track ID
      String songId = item["uri"].as<String>();
      if (songId.startsWith("spotify:track:"))
      {
        songId = songId.substring(14);
      }
      currentSong.Id = songId;
    }
    else
    {
      currentSong.artist = "Unknown Artist";
      currentSong.song = "No Song Playing";
      currentSong.durationMs = 0;
      currentSong.Id = "";
    }

    // -------- PLAYBACK STATE --------
    isPlaying =
        doc["is_playing"].is<bool>()
            ? doc["is_playing"].as<bool>()
            : false;

    lastSongPositionMs = currentSongPositionMs;
    success = true;

    if (success)
    {
      drawScreen();
    }

    return success;
  }

  bool drawScreen()
  {
    display.clearDisplay();

    if (!isActive)
    {
      // Show the no active device screen
      display.drawBitmap(9, 8, no_active_device, 112, 49, 1);
      display.display();
      return true;
    }

    // Display content for active device
    display.setTextColor(SH110X_WHITE);
    display.setTextSize(1);

    // Truncate long strings
    String songName = truncateString(currentSong.song, 20);
    String artistName = truncateString(currentSong.artist, 20);

    // Display song info
    display.setCursor(0, 0);
    display.print(songName);

    display.setCursor(0, 15);
    display.print(artistName);

    // Draw progress bar
    int barY = 30;
    int barHeight = 8;
    int barWidth = map(currentSongPositionMs, 0, currentSong.durationMs, 0, SCREEN_WIDTH);

    display.drawRect(0, barY, SCREEN_WIDTH, barHeight, SH110X_WHITE);
    display.fillRect(0, barY, barWidth, barHeight, SH110X_WHITE);

    // Display play/pause state
    display.setCursor(45, 55);
    display.print(isPlaying ? "Playing" : "Paused");

    // Display volume
    display.setCursor(110, 55);
    display.print(volCtrl ? String(volume) : "N/A");

    display.display();
    return true;
  }

  bool togglePlay()
  {
    String path = "/v1/me/player/";
    path += isPlaying ? "pause" : "play";

    String headers =
        "Authorization: Bearer " + accessToken + "\r\n"
                                                 "Content-Type: application/json\r\n"
                                                 "Content-Length: 0\r\n";

    String response;

    bool ok = httpsRequest(
        "api.spotify.com",
        path.c_str(),
        "PUT",
        headers,
        "",
        response);

    if (ok)
    {
      isPlaying = !isPlaying;
      Serial.println(isPlaying ? "Now playing" : "Now paused");
    }
    else
    {
      Serial.println("Error toggling playback");
    }

    getTrackInfo();
    return ok;
  }

  bool adjustVolume(int vol)
  {
    vol = constrain(vol, 0, 100);

    String path =
        "/v1/me/player/volume?volume_percent=" + String(vol);

    String headers =
        "Authorization: Bearer " + accessToken + "\r\n"
                                                 "Content-Type: application/json\r\n"
                                                 "Content-Length: 0\r\n";

    String response;

    bool ok = httpsRequest(
        "api.spotify.com",
        path.c_str(),
        "PUT",
        headers,
        "",
        response);

    if (ok)
    {
      currVol = vol;
      Serial.println("Volume set to: " + String(vol));
    }
    else
    {
      Serial.println("Error setting volume");
    }

    getTrackInfo();
    return ok;
  }

  bool skipForward()
  {
    String headers =
        "Authorization: Bearer " + accessToken + "\r\n"
                                                 "Content-Type: application/json\r\n"
                                                 "Content-Length: 0\r\n";

    String response;

    bool ok = httpsRequest(
        "api.spotify.com",
        "/v1/me/player/next",
        "POST",
        headers,
        "",
        response);

    if (ok)
    {
      Serial.println("Skipped to next track");
    }
    else
    {
      Serial.println("Error skipping forward");
    }

    getTrackInfo();
    return ok;
  }

  bool skipBack()
  {
    String headers =
        "Authorization: Bearer " + accessToken + "\r\n"
                                                 "Content-Type: application/json\r\n"
                                                 "Content-Length: 0\r\n";

    String response;

    bool ok = httpsRequest(
        "api.spotify.com",
        "/v1/me/player/previous",
        "POST",
        headers,
        "",
        response);

    if (ok)
    {
      Serial.println("Skipped to previous track");
    }
    else
    {
      Serial.println("Error skipping backward");
    }

    getTrackInfo();
    return ok;
  }

  // Public variables
  bool accessTokenSet = false;
  unsigned long tokenStartTime;
  int tokenExpireTime;
  SongDetails currentSong;
  float currentSongPositionMs;
  float lastSongPositionMs;
  int currVol;
  bool isPlaying;
  bool isActive = false;
  bool volCtrl = false;
  int volume;

private:
  String accessToken;
  String refreshToken;
};

// Global Spotify Connection instance
SpotConn spotifyConnection;

SSLCert *cert;
HTTPSServer *secureServer;

// HTTPS Connection Request Helper
bool httpsRequest(
    const char *host,
    const char *path,
    const String &method,
    const String &headers,
    const String &body,
    String &responseBody)
{
  WiFiClientSecure secureClient;

  // Force close any existing connection
  if (secureClient.connected())
  {
    secureClient.stop();
    delay(100);
  }

  secureClient.setCACert(spotify_root_ca);
  secureClient.setTimeout(10000); // 10 second timeout
  secureClient.setHandshakeTimeout(10000);

  if (!secureClient.connect(host, 443))
  {
    Serial.println("HTTPS connection failed");
    return false;
  }

  // ---- Request line ----
  secureClient.print(method + " " + path + " HTTP/1.1\r\n");
  secureClient.print("Host: " + String(host) + "\r\n");
  secureClient.print("User-Agent: ESP32\r\n");
  secureClient.print("Connection: close\r\n");

  // ---- Custom headers ----
  secureClient.print(headers);

  // ---- Content-Length if body exists ----
  if (body.length() > 0)
  {
    secureClient.print("Content-Length: ");
    secureClient.print(body.length());
    secureClient.print("\r\n");
  }

  secureClient.print("\r\n");

  // ---- Body ----
  if (body.length() > 0)
  {
    secureClient.print(body);
  }

  // ---- Read headers ----
  while (secureClient.connected())
  {
    String line = secureClient.readStringUntil('\n');
    if (line == "\r")
      break;
  }

  // ---- Read body ----
  responseBody = "";
  while (secureClient.available())
  {
    responseBody += secureClient.readString();
  }

  secureClient.stop();
  delay(100);
  return true;
}

// Web server handlers
void handleRoot(HTTPRequest *req, HTTPResponse *res)
{
  Serial.println("Handling root request");

  // Static buffer (safer than stack allocation)
  static char page[8192];

  // Generate the page
  if (snprintf(page, sizeof(page), mainPage, CLIENT_ID, REDIRECT_URI) < 0)
  {
    res->setStatusCode(500);
    res->setStatusText("Internal Server Error");
    res->println("Failed to generate page");
    return;
  }

  // Set headers and send response
  res->setHeader("Content-Type", "text/html");
  res->println(page);
}

void handleCallbackPage(HTTPRequest *req, HTTPResponse *res)
{
  // Debug logging
  Serial.println("=== CALLBACK HEADERS ===");
  Serial.print("Request line: ");
  Serial.println(req->getRequestString().c_str());

  // Get the "code" parameter from query string
  std::string code;
  bool hasCode = req->getParams()->getQueryParameter("code", code);

  if (!spotifyConnection.accessTokenSet)
  {
    if (code.empty())
    {
      static char page[8192];
      if (snprintf(page, sizeof(page), errorPage, CLIENT_ID, REDIRECT_URI) < 0)
      {
        res->setStatusCode(500);
        res->setStatusText("Internal Server Error");
        res->println("Error page generation failed");
        return;
      }
      res->setHeader("Content-Type", "text/html");
      res->println(page);
    }
    else
    { // Convert std::string to String
      String codeStr = String(code.c_str());
      if (spotifyConnection.getUserCode(codeStr))
      {
        res->setHeader("Content-Type", "text/plain");
        res->print("Spotify setup complete. Refresh in: ");
        res->print(String(spotifyConnection.tokenExpireTime));
      }
      else
      {
        static char page[800];
        if (snprintf(page, sizeof(page), errorPage, CLIENT_ID, REDIRECT_URI) < 0)
        {
          res->setStatusCode(500);
          res->setStatusText("Internal Server Error");
          res->println("Error page generation failed");
          return;
        }
        res->setHeader("Content-Type", "text/html");
        res->println(page);
      }
    }
  }
  else
  {
    res->setHeader("Content-Type", "text/plain");
    res->println("Already authenticated");
  }
}

void handle404(HTTPRequest *req, HTTPResponse *res)
{
  // Discard request body if any
  req->discardRequestBody();

  // Set response status
  res->setStatusCode(404);
  res->setStatusText("Not Found");

  // Set content type
  res->setHeader("Content-Type", "text/html");

  // Write 404 page
  res->println("<!DOCTYPE html>");
  res->println("<html>");
  res->println("<head><title>Not Found</title></head>");
  res->println("<body><h1>404 Not Found</h1><p>The requested resource was not found on this server.</p></body>");
  res->println("</html>");
}

// Helper function to truncate strings
String truncateString(const String &input, int maxLength)
{
  if (input.length() <= maxLength)
    return input;
  return input.substring(0, maxLength - 3) + "...";
}

// Button handler function
void handleButtons()
{
  if (prevBtn.isPressed())
  {
    Serial.println("Previous button pressed");
    spotifyConnection.skipBack();
    spotifyConnection.getTrackInfo();
  }

  else if (playBtn.isPressed())
  {
    Serial.println("Play button pressed");
    spotifyConnection.togglePlay();
  }

  else if (nextBtn.isPressed())
  {
    Serial.println("Next button pressed");
    spotifyConnection.skipForward();
    spotifyConnection.getTrackInfo();
  }

  if (encSwBtn.isPressed())
  {
    Serial.println("Encoder switch pressed");
    spotifyConnection.togglePlay();
  }

  prevBtn.loop();
  playBtn.loop();
  nextBtn.loop();
  encSwBtn.loop();
}

// Volume control handler
void handleVolumeControl()
{
  if (spotifyConnection.volCtrl)
  {
    int newVolume = encoder.getCount() * 5;
    // Only update if volume change exceeds threshold
    if (abs(newVolume - spotifyConnection.currVol) > VOLUME_UPDATE_THRESHOLD)
    {
      spotifyConnection.adjustVolume(newVolume);
    }
  }
}

// Global flag for server state
bool serverOn = true;

void setup()
{
  // Initialize serial communication
  Serial.begin(9600);
  Serial.println("Starting ESP32 Spotify Player");

  // Initialize display
  delay(250); // Wait for OLED to initialize
  display.begin(I2C_ADDRESS, true);

  // Show splash screen
  display.clearDisplay();
  display.drawBitmap(9, 8, splash_screen, 112, 51, 1);
  display.display();
  delay(700);

  // Set up button pins
  pinMode(PLAY_BTN_PIN, INPUT_PULLUP);
  pinMode(PREV_BTN_PIN, INPUT_PULLUP);
  pinMode(NEXT_BTN_PIN, INPUT_PULLUP);
  pinMode(ENC_SW_PIN, INPUT_PULLUP);

  // Set up rotary encoder
  encoder.attachHalfQuad(ENC_DT_PIN, ENC_CLK_PIN);
/*ESP Spotify Player V2.1
- By Kaustubh Doval

OLED: 128x64
  SCK -> D22
  SDA -> D21

Buttons
  previous button   -> D5
  play/pause button -> D18
  next button       -> D19

Rotary Encoder
  CLK -> D4
  DT  -> D2
  SW  -> D15

SETUP:
(1) Change SSID and Password (line 45)
(2) Change Client Secret and ID (49)
(3) Run the program, You can see your ESPs IP on Serial Monitor (9600 Baud Rate) and on the OLED.
    You need to add the IP address of your ESP to REDIRECT_URI definition (line 51):
          https://YOUR_ESP_IP/callback
(4) Add this callback to Spotify Web API portal as well (ensure that it is exactly the same as REDIRECT_URI)
(5) You are good to go!
*/

// Increase Header Sizes
#undef HTTPS_CONNECTION_DATA_CHUNK_SIZE
#define HTTPS_CONNECTION_DATA_CHUNK_SIZE 4096

#undef HTTPS_REQUEST_MAX_REQUEST_LENGTH
#define HTTPS_REQUEST_MAX_REQUEST_LENGTH 8192

#undef HTTPS_MAX_HEADER_LENGTH
#define HTTPS_MAX_HEADER_LENGTH 4096

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <ESP32Encoder.h>
#include <ArduinoJson.h>
#include <base64.h>
#include <ezButton.h>

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPSServer.hpp>
#include <SSLCert.hpp>
#include <HTTPRequest.hpp>
#include <HTTPResponse.hpp>

// Load Env Variables
#include "secrets.h"

// Load other Files
#include "index.h"

// Pin Definitions
#define PREV_BTN_PIN 5
#define PLAY_BTN_PIN 18
#define NEXT_BTN_PIN 19
#define ENC_CLK_PIN 4
#define ENC_DT_PIN 2
#define ENC_SW_PIN 15

// OLED Definitions
#define I2C_ADDRESS 0x3c
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

// Objects
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
ESP32Encoder encoder;

ezButton prevBtn(PREV_BTN_PIN);
ezButton playBtn(PLAY_BTN_PIN);
ezButton nextBtn(NEXT_BTN_PIN);
ezButton encSwBtn(ENC_SW_PIN);

// Timing variables
unsigned long lastApiRefresh = 0;

// Song details struct
struct SongDetails
{
  int durationMs;
  String album;
  String artist;
  String song;
  String Id;
  bool isLiked;
};

// Forward declarations
String truncateString(const String &input, int maxLength);
void handleButtons();
void handleVolumeControl();
void handleRoot();
void handleCallbackPage();
bool httpsRequest(
    const char *host,
    const char *path,
    const String &method,
    const String &headers,
    const String &body,
    String &responseBody);

using namespace httpsserver;

// Some Bitmap Definitions
static const unsigned char PROGMEM no_active_device[] = {0x07, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf8, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0xe0, 0x07, 0xce, 0x78, 0x03, 0x80, 0x00, 0x41, 0x02, 0x00, 0x01, 0x00, 0x7c, 0x01, 0xe0, 0x04, 0x11, 0x44, 0x04, 0x40, 0x00, 0x40, 0x05, 0x00, 0x01, 0x00, 0x44, 0x01, 0xe0, 0x04, 0x10, 0x44, 0x04, 0x16, 0x39, 0xf3, 0x04, 0x44, 0x01, 0x00, 0x7c, 0x01, 0xe0, 0x07, 0x8e, 0x78, 0x03, 0x99, 0x44, 0x41, 0x0e, 0x44, 0x01, 0x00, 0x7c, 0x01, 0xe0, 0x04, 0x01, 0x40, 0x00, 0x59, 0x44, 0x41, 0x04, 0x3c, 0x01, 0x00, 0x7c, 0x01, 0xe0, 0x04, 0x11, 0x40, 0x04, 0x56, 0x44, 0x51, 0x04, 0x04, 0x01, 0x00, 0x00, 0x01, 0xe0, 0x07, 0xce, 0x40, 0x03, 0x90, 0x38, 0x23, 0x84, 0x44, 0x01, 0x00, 0x00, 0x01, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x38, 0x01, 0x1f, 0xff, 0x9f, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x10, 0x00, 0x91, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xf0, 0x00, 0xf1, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0xe0, 0x00, 0x00, 0x0f, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x7c, 0x01, 0xe0, 0x00, 0x00, 0x08, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x7c, 0x01, 0xe0, 0x00, 0x00, 0x08, 0x88, 0x62, 0x27, 0x2c, 0x00, 0x00, 0x01, 0x00, 0x7c, 0x01, 0xe0, 0x00, 0x00, 0x0f, 0x08, 0x12, 0x28, 0xb2, 0x00, 0x00, 0x01, 0x00, 0x44, 0x01, 0xe0, 0x00, 0x00, 0x08, 0x08, 0x71, 0xef, 0xa0, 0x00, 0x00, 0x01, 0x00, 0x7c, 0x01, 0xe0, 0x00, 0x00, 0x08, 0x08, 0x90, 0x28, 0x20, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0xe0, 0x00, 0x00, 0x08, 0x1c, 0x7a, 0x27, 0x20, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x01, 0xc0, 0x00, 0x00, 0x00, 0x01, 0xff, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xff, 0xfe, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xff, 0xfc, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf8, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x20, 0x00, 0x20, 0x02, 0x08, 0x00, 0x00, 0x3c, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02, 0x20, 0x00, 0x50, 0x02, 0x00, 0x00, 0x00, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x27, 0x00, 0x89, 0xcf, 0x98, 0x89, 0xc0, 0x22, 0x72, 0x26, 0x1c, 0x70, 0x00, 0x02, 0xa8, 0x80, 0x8a, 0x22, 0x08, 0x8a, 0x20, 0x22, 0x8a, 0x22, 0x22, 0x88, 0x00, 0x02, 0x68, 0x80, 0xfa, 0x02, 0x08, 0x8b, 0xe0, 0x22, 0xfa, 0x22, 0x20, 0xf8, 0x00, 0x02, 0x28, 0x80, 0x8a, 0x22, 0x88, 0x52, 0x00, 0x22, 0x81, 0x42, 0x22, 0x80, 0x00, 0x02, 0x27, 0x00, 0x89, 0xc1, 0x1c, 0x21, 0xc0, 0x3c, 0x70, 0x87, 0x1c, 0x70, 0x00};
static const unsigned char PROGMEM splash_screen[] = {0x07, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf8, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0xe0, 0x07, 0xce, 0x78, 0x03, 0x80, 0x00, 0x41, 0x02, 0x00, 0x01, 0x00, 0x7c, 0x01, 0xe0, 0x04, 0x11, 0x44, 0x04, 0x40, 0x00, 0x40, 0x05, 0x00, 0x01, 0x00, 0x44, 0x01, 0xe0, 0x04, 0x10, 0x44, 0x04, 0x16, 0x39, 0xf3, 0x04, 0x44, 0x01, 0x00, 0x7c, 0x01, 0xe0, 0x07, 0x8e, 0x78, 0x03, 0x99, 0x44, 0x41, 0x0e, 0x44, 0x01, 0x00, 0x7c, 0x01, 0xe0, 0x04, 0x01, 0x40, 0x00, 0x59, 0x44, 0x41, 0x04, 0x3c, 0x01, 0x00, 0x7c, 0x01, 0xe0, 0x04, 0x11, 0x40, 0x04, 0x56, 0x44, 0x51, 0x04, 0x04, 0x01, 0x00, 0x00, 0x01, 0xe0, 0x07, 0xce, 0x40, 0x03, 0x90, 0x38, 0x23, 0x84, 0x44, 0x01, 0x00, 0x00, 0x01, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x38, 0x01, 0x1f, 0xff, 0x9f, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x10, 0x00, 0x91, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xf0, 0x00, 0xf1, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0xe0, 0x00, 0x00, 0x0f, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x7c, 0x01, 0xe0, 0x00, 0x00, 0x08, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x7c, 0x01, 0xe0, 0x00, 0x00, 0x08, 0x88, 0x62, 0x27, 0x2c, 0x00, 0x00, 0x01, 0x00, 0x7c, 0x01, 0xe0, 0x00, 0x00, 0x0f, 0x08, 0x12, 0x28, 0xb2, 0x00, 0x00, 0x01, 0x00, 0x44, 0x01, 0xe0, 0x00, 0x00, 0x08, 0x08, 0x71, 0xef, 0xa0, 0x00, 0x00, 0x01, 0x00, 0x7c, 0x01, 0xe0, 0x00, 0x00, 0x08, 0x08, 0x90, 0x28, 0x20, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0xe0, 0x00, 0x00, 0x08, 0x1c, 0x7a, 0x27, 0x20, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x01, 0xc0, 0x00, 0x00, 0x00, 0x01, 0xff, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xff, 0xfe, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xff, 0xfc, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf8, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x02, 0x20, 0x00, 0x00, 0x80, 0x20, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x80, 0x02, 0x40, 0x00, 0x00, 0x80, 0x20, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0xa2, 0x02, 0x86, 0x22, 0x7b, 0xe8, 0xac, 0xb0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x22, 0x03, 0x01, 0x22, 0x80, 0x88, 0xb2, 0xc8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x9e, 0x02, 0x87, 0x22, 0x70, 0x88, 0xa2, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x82, 0x02, 0x49, 0x26, 0x08, 0xa9, 0xb2, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x22, 0x02, 0x27, 0x9a, 0xf0, 0x46, 0xac, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const unsigned char PROGMEM configuring[] = {0x80, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc1, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x42, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x22, 0x8c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x13, 0x0c, 0x07, 0x00, 0x00, 0x42, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x0a, 0xb4, 0x08, 0x80, 0x00, 0xa0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x48, 0x08, 0x1c, 0xb0, 0x86, 0x1c, 0x8a, 0xc6, 0x2c, 0x70, 0x00, 0x00, 0x05, 0xf0, 0x08, 0x22, 0xc9, 0xc2, 0x26, 0x8b, 0x22, 0x32, 0x98, 0x00, 0x00, 0x0b, 0x00, 0x08, 0x22, 0x88, 0x82, 0x26, 0x8a, 0x02, 0x22, 0x98, 0x00, 0x00, 0x14, 0xe0, 0x08, 0xa2, 0x88, 0x82, 0x1a, 0x9a, 0x02, 0x22, 0x68, 0xc3, 0x0c, 0x29, 0xb0, 0x07, 0x1c, 0x88, 0x87, 0x02, 0x6a, 0x07, 0x22, 0x08, 0xc3, 0x0c, 0x50, 0xd8, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x70, 0x00, 0x00, 0xa0, 0x6c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

const char *spotify_root_ca PROGMEM =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIEyDCCA7CgAwIBAgIQDPW9BitWAvR6uFAsI8zwZjANBgkqhkiG9w0BAQsFADBh\n"
    "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n"
    "d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH\n"
    "MjAeFw0yMTAzMzAwMDAwMDBaFw0zMTAzMjkyMzU5NTlaMFkxCzAJBgNVBAYTAlVT\n"
    "MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxMzAxBgNVBAMTKkRpZ2lDZXJ0IEdsb2Jh\n"
    "bCBHMiBUTFMgUlNBIFNIQTI1NiAyMDIwIENBMTCCASIwDQYJKoZIhvcNAQEBBQAD\n"
    "ggEPADCCAQoCggEBAMz3EGJPprtjb+2QUlbFbSd7ehJWivH0+dbn4Y+9lavyYEEV\n"
    "cNsSAPonCrVXOFt9slGTcZUOakGUWzUb+nv6u8W+JDD+Vu/E832X4xT1FE3LpxDy\n"
    "FuqrIvAxIhFhaZAmunjZlx/jfWardUSVc8is/+9dCopZQ+GssjoP80j812s3wWPc\n"
    "3kbW20X+fSP9kOhRBx5Ro1/tSUZUfyyIxfQTnJcVPAPooTncaQwywa8WV0yUR0J8\n"
    "osicfebUTVSvQpmowQTCd5zWSOTOEeAqgJnwQ3DPP3Zr0UxJqyRewg2C/Uaoq2yT\n"
    "zGJSQnWS+Jr6Xl6ysGHlHx+5fwmY6D36g39HaaECAwEAAaOCAYIwggF+MBIGA1Ud\n"
    "EwEB/wQIMAYBAf8CAQAwHQYDVR0OBBYEFHSFgMBmx9833s+9KTeqAx2+7c0XMB8G\n"
    "A1UdIwQYMBaAFE4iVCAYlebjbuYP+vq5Eu0GF485MA4GA1UdDwEB/wQEAwIBhjAd\n"
    "BgNVHSUEFjAUBggrBgEFBQcDAQYIKwYBBQUHAwIwdgYIKwYBBQUHAQEEajBoMCQG\n"
    "CCsGAQUFBzABhhhodHRwOi8vb2NzcC5kaWdpY2VydC5jb20wQAYIKwYBBQUHMAKG\n"
    "NGh0dHA6Ly9jYWNlcnRzLmRpZ2ljZXJ0LmNvbS9EaWdpQ2VydEdsb2JhbFJvb3RH\n"
    "Mi5jcnQwQgYDVR0fBDswOTA3oDWgM4YxaHR0cDovL2NybDMuZGlnaWNlcnQuY29t\n"
    "L0RpZ2lDZXJ0R2xvYmFsUm9vdEcyLmNybDA9BgNVHSAENjA0MAsGCWCGSAGG/WwC\n"
    "ATAHBgVngQwBATAIBgZngQwBAgEwCAYGZ4EMAQICMAgGBmeBDAECAzANBgkqhkiG\n"
    "9w0BAQsFAAOCAQEAkPFwyyiXaZd8dP3A+iZ7U6utzWX9upwGnIrXWkOH7U1MVl+t\n"
    "wcW1BSAuWdH/SvWgKtiwla3JLko716f2b4gp/DA/JIS7w7d7kwcsr4drdjPtAFVS\n"
    "slme5LnQ89/nD/7d+MS5EHKBCQRfz5eeLjJ1js+aWNJXMX43AYGyZm0pGrFmCW3R\n"
    "bpD0ufovARTFXFZkAdl9h6g4U5+LXUZtXMYnhIHUfoyMo5tS58aI7Dd8KvvwVVo4\n"
    "chDYABPPTHPbqjc1qCmBaZx2vN4Ye5DUys/vZwP9BFohFrH/6j/f3IL16/RZkiMN\n"
    "JCqVJUzKoZHm1Lesh3Sz8W2jmdv51b2EQJ8HmA==\n"
    "-----END CERTIFICATE-----\n";

// Spotify Connection Class
class SpotConn
{
public:
  SpotConn()
  {
    // Constructor
  }

  bool getUserCode(const String &serverCode)
  {
    JsonDocument doc;
    String response;

    String auth = "Basic " + base64::encode(
                                 String(CLIENT_ID) + ":" + String(CLIENT_SECRET));

    String headers =
        "Authorization: " + auth + "\r\n"
                                   "Content-Type: application/x-www-form-urlencoded\r\n";

    String body =
        "grant_type=authorization_code"
        "&code=" +
        serverCode +
        "&redirect_uri=" + String(REDIRECT_URI);

    bool ok = httpsRequest(
        "accounts.spotify.com",
        "/api/token",
        "POST",
        headers,
        body,
        response);

    if (!ok)
    {
      Serial.println("HTTPS request failed");
      return false;
    }

    DeserializationError error = deserializeJson(doc, response);
    if (error)
    {
      Serial.print("JSON parse failed: ");
      Serial.println(error.c_str());
      return false;
    }

    accessToken = doc["access_token"].as<String>();
    refreshToken = doc["refresh_token"].as<String>();
    tokenExpireTime = doc["expires_in"].as<int>();
    tokenStartTime = millis();
    accessTokenSet = true;

    Serial.println("Access token: " + accessToken);
    Serial.println("Refresh token: " + refreshToken);

    return true;
  }

  bool refreshAuth()
  {
    JsonDocument doc;
    String response;

    accessTokenSet = false;

    String auth = "Basic " + base64::encode(
                                 String(CLIENT_ID) + ":" + String(CLIENT_SECRET));

    String headers =
        "Authorization: " + auth + "\r\n"
                                   "Content-Type: application/x-www-form-urlencoded\r\n";

    String body =
        "grant_type=refresh_token"
        "&refresh_token=" +
        refreshToken;

    bool ok = httpsRequest(
        "accounts.spotify.com",
        "/api/token",
        "POST",
        headers,
        body,
        response);

    if (!ok)
    {
      Serial.println("HTTPS refresh request failed");
      return false;
    }

    DeserializationError error = deserializeJson(doc, response);
    if (error)
    {
      Serial.print("JSON parse failed: ");
      Serial.println(error.c_str());
      return false;
    }

    accessToken = doc["access_token"].as<String>();

    // Optional refresh_token (Spotify sometimes omits it)
    if (!doc["refresh_token"].isNull())
    {
      refreshToken = doc["refresh_token"].as<String>();
    }

    tokenExpireTime = doc["expires_in"].as<int>();
    tokenStartTime = millis();
    accessTokenSet = true;

    Serial.println("Refreshed access token: " + accessToken);
    return true;
  }

  bool getTrackInfo()
  {
    JsonDocument doc;
    String response;
    bool success = false;

    String headers =
        "Authorization: Bearer " + accessToken + "\r\n";

    bool ok = httpsRequest(
        "api.spotify.com",
        "/v1/me/player",
        "GET",
        headers,
        "",
        response);

    if (!ok)
    {
      Serial.println("HTTPS player request failed");
      return false;
    }

    // Spotify returns 204 with an empty body
    if (response.length() == 0)
    {
      Serial.println("NOTE: No Active Device or No Song Playing.");
      isActive = false;
      volCtrl = false;
      success = true;
      drawScreen();
      return true;
    }

    DeserializationError error = deserializeJson(doc, response);
    if (error)
    {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return false;
    }

    // -------- DEVICE INFO --------
    if (!doc["device"].isNull())
    {
      JsonObject device = doc["device"];
      isActive = device["is_active"].as<bool>();
      volCtrl = device["supports_volume"].as<bool>();
      volume = device["volume_percent"].as<int>();

      encoder.setCount(volume / 5);
      Serial.print("Spotify Status: ");
      Serial.println(isActive);
    }
    else
    {
      isActive = false;
      volCtrl = false;
    }

    // -------- PROGRESS --------
    currentSongPositionMs =
        doc["progress_ms"].is<int>()
            ? doc["progress_ms"].as<int>()
            : 0;

    // -------- SONG ITEM --------
    if (!doc["item"].isNull())
    {
      JsonObject item = doc["item"];

      // Artist
      if (!item["artists"].isNull())
      {
        JsonArray artists = item["artists"].as<JsonArray>();
        currentSong.artist =
            (!artists.isNull() && artists.size() > 0)
                ? artists[0]["name"].as<String>()
                : "Unknown Artist";
      }
      else
      {
        currentSong.artist = "Unknown Artist";
      }

      // Song name
      currentSong.song = item["name"].as<String>();

      // Duration
      currentSong.durationMs = item["duration_ms"].as<int>();

      // Track ID
      String songId = item["uri"].as<String>();
      if (songId.startsWith("spotify:track:"))
      {
        songId = songId.substring(14);
      }
      currentSong.Id = songId;
    }
    else
    {
      currentSong.artist = "Unknown Artist";
      currentSong.song = "No Song Playing";
      currentSong.durationMs = 0;
      currentSong.Id = "";
    }

    // -------- PLAYBACK STATE --------
    isPlaying =
        doc["is_playing"].is<bool>()
            ? doc["is_playing"].as<bool>()
            : false;

    lastSongPositionMs = currentSongPositionMs;
    success = true;

    if (success)
    {
      drawScreen();
    }

    return success;
  }

  bool drawScreen()
  {
    display.clearDisplay();

    if (!isActive)
    {
      // Show the no active device screen
      display.drawBitmap(9, 8, no_active_device, 112, 49, 1);
      display.display();
      return true;
    }

    // Display content for active device
    display.setTextColor(SH110X_WHITE);
    display.setTextSize(1);

    // Truncate long strings
    String songName = truncateString(currentSong.song, 20);
    String artistName = truncateString(currentSong.artist, 20);

    // Display song info
    display.setCursor(0, 0);
    display.print(songName);

    display.setCursor(0, 15);
    display.print(artistName);

    // Draw progress bar
    int barY = 30;
    int barHeight = 8;
    int barWidth = map(currentSongPositionMs, 0, currentSong.durationMs, 0, SCREEN_WIDTH);

    display.drawRect(0, barY, SCREEN_WIDTH, barHeight, SH110X_WHITE);
    display.fillRect(0, barY, barWidth, barHeight, SH110X_WHITE);

    // Display play/pause state
    display.setCursor(45, 55);
    display.print(isPlaying ? "Playing" : "Paused");

    // Display volume
    display.setCursor(110, 55);
    display.print(volCtrl ? String(volume) : "N/A");

    display.display();
    return true;
  }

  bool togglePlay()
  {
    String path = "/v1/me/player/";
    path += isPlaying ? "pause" : "play";

    String headers =
        "Authorization: Bearer " + accessToken + "\r\n"
                                                 "Content-Type: application/json\r\n"
                                                 "Content-Length: 0\r\n";

    String response;

    bool ok = httpsRequest(
        "api.spotify.com",
        path.c_str(),
        "PUT",
        headers,
        "",
        response);

    if (ok)
    {
      isPlaying = !isPlaying;
      Serial.println(isPlaying ? "Now playing" : "Now paused");
    }
    else
    {
      Serial.println("Error toggling playback");
    }

    getTrackInfo();
    return ok;
  }

  bool adjustVolume(int vol)
  {
    vol = constrain(vol, 0, 100);

    String path =
        "/v1/me/player/volume?volume_percent=" + String(vol);

    String headers =
        "Authorization: Bearer " + accessToken + "\r\n"
                                                 "Content-Type: application/json\r\n"
                                                 "Content-Length: 0\r\n";

    String response;

    bool ok = httpsRequest(
        "api.spotify.com",
        path.c_str(),
        "PUT",
        headers,
        "",
        response);

    if (ok)
    {
      currVol = vol;
      Serial.println("Volume set to: " + String(vol));
    }
    else
    {
      Serial.println("Error setting volume");
    }

    getTrackInfo();
    return ok;
  }

  bool skipForward()
  {
    String headers =
        "Authorization: Bearer " + accessToken + "\r\n"
                                                 "Content-Type: application/json\r\n"
                                                 "Content-Length: 0\r\n";

    String response;

    bool ok = httpsRequest(
        "api.spotify.com",
        "/v1/me/player/next",
        "POST",
        headers,
        "",
        response);

    if (ok)
    {
      Serial.println("Skipped to next track");
    }
    else
    {
      Serial.println("Error skipping forward");
    }

    getTrackInfo();
    return ok;
  }

  bool skipBack()
  {
    String headers =
        "Authorization: Bearer " + accessToken + "\r\n"
                                                 "Content-Type: application/json\r\n"
                                                 "Content-Length: 0\r\n";

    String response;

    bool ok = httpsRequest(
        "api.spotify.com",
        "/v1/me/player/previous",
        "POST",
        headers,
        "",
        response);

    if (ok)
    {
      Serial.println("Skipped to previous track");
    }
    else
    {
      Serial.println("Error skipping backward");
    }

    getTrackInfo();
    return ok;
  }

  // Public variables
  bool accessTokenSet = false;
  unsigned long tokenStartTime;
  int tokenExpireTime;
  SongDetails currentSong;
  float currentSongPositionMs;
  float lastSongPositionMs;
  int currVol;
  bool isPlaying;
  bool isActive = false;
  bool volCtrl = false;
  int volume;

private:
  String accessToken;
  String refreshToken;
};

// Global Spotify Connection instance
SpotConn spotifyConnection;

SSLCert *cert;
HTTPSServer *secureServer;

// HTTPS Connection Request Helper
bool httpsRequest(
    const char *host,
    const char *path,
    const String &method,
    const String &headers,
    const String &body,
    String &responseBody)
{
  WiFiClientSecure secureClient;

  // Force close any existing connection
  if (secureClient.connected())
  {
    secureClient.stop();
    delay(100);
  }

  secureClient.setCACert(spotify_root_ca);
  secureClient.setTimeout(10000); // 10 second timeout
  secureClient.setHandshakeTimeout(10000);

  if (!secureClient.connect(host, 443))
  {
    Serial.println("HTTPS connection failed");
    return false;
  }

  // ---- Request line ----
  secureClient.print(method + " " + path + " HTTP/1.1\r\n");
  secureClient.print("Host: " + String(host) + "\r\n");
  secureClient.print("User-Agent: ESP32\r\n");
  secureClient.print("Connection: close\r\n");

  // ---- Custom headers ----
  secureClient.print(headers);

  // ---- Content-Length if body exists ----
  if (body.length() > 0)
  {
    secureClient.print("Content-Length: ");
    secureClient.print(body.length());
    secureClient.print("\r\n");
  }

  secureClient.print("\r\n");

  // ---- Body ----
  if (body.length() > 0)
  {
    secureClient.print(body);
  }

  // ---- Read headers ----
  while (secureClient.connected())
  {
    String line = secureClient.readStringUntil('\n');
    if (line == "\r")
      break;
  }

  // ---- Read body ----
  responseBody = "";
  while (secureClient.available())
  {
    responseBody += secureClient.readString();
  }

  secureClient.stop();
  delay(100);
  return true;
}

// Web server handlers
void handleRoot(HTTPRequest *req, HTTPResponse *res)
{
  Serial.println("Handling root request");

  // Static buffer (safer than stack allocation)
  static char page[8192];

  // Generate the page
  if (snprintf(page, sizeof(page), mainPage, CLIENT_ID, REDIRECT_URI) < 0)
  {
    res->setStatusCode(500);
    res->setStatusText("Internal Server Error");
    res->println("Failed to generate page");
    return;
  }

  // Set headers and send response
  res->setHeader("Content-Type", "text/html");
  res->println(page);
}

void handleCallbackPage(HTTPRequest *req, HTTPResponse *res)
{
  // Debug logging
  Serial.println("=== CALLBACK HEADERS ===");
  Serial.print("Request line: ");
  Serial.println(req->getRequestString().c_str());

  // Get the "code" parameter from query string
  std::string code;
  bool hasCode = req->getParams()->getQueryParameter("code", code);

  if (!spotifyConnection.accessTokenSet)
  {
    if (code.empty())
    {
      static char page[8192];
      if (snprintf(page, sizeof(page), errorPage, CLIENT_ID, REDIRECT_URI) < 0)
      {
        res->setStatusCode(500);
        res->setStatusText("Internal Server Error");
        res->println("Error page generation failed");
        return;
      }
      res->setHeader("Content-Type", "text/html");
      res->println(page);
    }
    else
    { // Convert std::string to String
      String codeStr = String(code.c_str());
      if (spotifyConnection.getUserCode(codeStr))
      {
        res->setHeader("Content-Type", "text/plain");
        res->print("Spotify setup complete. Refresh in: ");
        res->print(String(spotifyConnection.tokenExpireTime));
      }
      else
      {
        static char page[800];
        if (snprintf(page, sizeof(page), errorPage, CLIENT_ID, REDIRECT_URI) < 0)
        {
          res->setStatusCode(500);
          res->setStatusText("Internal Server Error");
          res->println("Error page generation failed");
          return;
        }
        res->setHeader("Content-Type", "text/html");
        res->println(page);
      }
    }
  }
  else
  {
    res->setHeader("Content-Type", "text/plain");
    res->println("Already authenticated");
  }
}

void handle404(HTTPRequest *req, HTTPResponse *res)
{
  // Discard request body if any
  req->discardRequestBody();

  // Set response status
  res->setStatusCode(404);
  res->setStatusText("Not Found");

  // Set content type
  res->setHeader("Content-Type", "text/html");

  // Write 404 page
  res->println("<!DOCTYPE html>");
  res->println("<html>");
  res->println("<head><title>Not Found</title></head>");
  res->println("<body><h1>404 Not Found</h1><p>The requested resource was not found on this server.</p></body>");
  res->println("</html>");
}

// Helper function to truncate strings
String truncateString(const String &input, int maxLength)
{
  if (input.length() <= maxLength)
    return input;
  return input.substring(0, maxLength - 3) + "...";
}

// Button handler function
void handleButtons()
{
  if (prevBtn.isPressed())
  {
    Serial.println("Previous button pressed");
    spotifyConnection.skipBack();
    spotifyConnection.getTrackInfo();
  }

  else if (playBtn.isPressed())
  {
    Serial.println("Play button pressed");
    spotifyConnection.togglePlay();
  }

  else if (nextBtn.isPressed())
  {
    Serial.println("Next button pressed");
    spotifyConnection.skipForward();
    spotifyConnection.getTrackInfo();
  }

  if (encSwBtn.isPressed())
  {
    Serial.println("Encoder switch pressed");
    spotifyConnection.togglePlay();
  }

  prevBtn.loop();
  playBtn.loop();
  nextBtn.loop();
  encSwBtn.loop();
}

// Volume control handler
void handleVolumeControl()
{
  if (spotifyConnection.volCtrl)
  {
    int newVolume = encoder.getCount() * 5;
    // Only update if volume change exceeds threshold
    if (abs(newVolume - spotifyConnection.currVol) > VOLUME_UPDATE_THRESHOLD)
    {
      spotifyConnection.adjustVolume(newVolume);
    }
  }
}

// Global flag for server state
bool serverOn = true;

void setup()
{
  // Initialize serial communication
  Serial.begin(9600);
  Serial.println("Starting ESP32 Spotify Player");

  // Initialize display
  delay(250); // Wait for OLED to initialize
  display.begin(I2C_ADDRESS, true);

  // Show splash screen
  display.clearDisplay();
  display.drawBitmap(9, 8, splash_screen, 112, 51, 1);
  display.display();
  delay(700);

  // Set up button pins
  pinMode(PLAY_BTN_PIN, INPUT_PULLUP);
  pinMode(PREV_BTN_PIN, INPUT_PULLUP);
  pinMode(NEXT_BTN_PIN, INPUT_PULLUP);
  pinMode(ENC_SW_PIN, INPUT_PULLUP);

  // Set up rotary encoder
  encoder.attachHalfQuad(ENC_DT_PIN, ENC_CLK_PIN);

  // Create SSL Certificate
  cert = new SSLCert();
  // secureServer->setMaxHeaderLength(4096);

  // Now, we use the function createSelfSignedCert to create private key and certificate.
  // The function takes the following paramters:
  // - Key size: 1024 or 2048 bit should be fine here, 4096 on the ESP might be "paranoid mode"
  //   (in generel: shorter key = faster but less secure)
  // - Distinguished name: The name of the host as used in certificates.
  //   If you want to run your own DNS, the part after CN (Common Name) should match the DNS
  //   entry pointing to your ESP32. You can try to insert an IP there, but that's not really good style.
  // - Dates for certificate validity (optional, default is 2019-2029, both included)
  //   Format is YYYYMMDDhhmmss
  int createCertResult = createSelfSignedCert(
      *cert,
      KEYSIZE_1024,
      "CN=myesp32.local,O=FancyCompany,C=DE",
      "20190101000000",
      "20300101000000");

  // Now check if creating that worked
  if (createCertResult != 0)
  {
    Serial.printf("Creating certificate failed. Error Code = 0x%02X, check SSLCert.hpp for details", createCertResult);
    while (true)
      delay(500);
  }
  Serial.println("Creating the certificate was successful");

  // Setup Server using new certificate
  // secureServer = new HTTPSServer(cert);
  secureServer = new HTTPSServer(cert, 443, 4);

  // Connect to WiFi
  WiFi.begin(WIFI_SSID, PASSWORD);
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("\nConnected to WiFi");

  // Set up web server
  ResourceNode *nodeRoot = new ResourceNode("/", "GET", &handleRoot);
  ResourceNode *nodeCallback = new ResourceNode("/callback", "GET", &handleCallbackPage);
  ResourceNode *node404 = new ResourceNode("", "GET", &handle404);

  // Register nodes
  secureServer->registerNode(nodeRoot);
  secureServer->registerNode(nodeCallback);
  secureServer->setDefaultNode(node404); // For 404 handling

  // Start HTTPS server
  Serial.println("Starting HTTPS server...");
  secureServer->setDefaultHeader("Connection", "close");
  secureServer->start();

  if (secureServer->isRunning())
  {
    Serial.println("HTTPS server ready.");
    Serial.print("Access via: https://");
    Serial.println(WiFi.localIP());
  }

  // Show configuration screen
  display.clearDisplay();
  display.setTextColor(1);
  display.setTextWrap(false);
  display.setCursor(5, 38);
  display.print("ESP IP:" + WiFi.localIP().toString());
  display.drawBitmap(14, 15, configuring, 102, 15, 1);
  display.display();
}

void loop()
{
  unsigned long currentMillis = millis();

  // If not authenticated, handle web server requests
  if (!spotifyConnection.accessTokenSet)
  {
    secureServer->loop();
    return;
  }

  // Close server after authentication
  if (serverOn)
  {
    secureServer->stop();
    serverOn = false;
  }

  // Refresh access token if needed
  if ((currentMillis - spotifyConnection.tokenStartTime) / 1000 > spotifyConnection.tokenExpireTime - 60)
  {
    Serial.println("Refreshing token");
    if (spotifyConnection.refreshAuth())
    {
      Serial.println("Token refreshed successfully");
    }
  }

  // Update track info periodically
  if (currentMillis - lastApiRefresh > API_REFRESH_INTERVAL)
  {
    spotifyConnection.getTrackInfo();
    lastApiRefresh = currentMillis;
  }

  // Handle user input
  handleButtons();

  // Handle volume control
  handleVolumeControl();
}
  // Create SSL Certificate
  cert = new SSLCert();
  // secureServer->setMaxHeaderLength(4096);

  // Now, we use the function createSelfSignedCert to create private key and certificate.
  // The function takes the following paramters:
  // - Key size: 1024 or 2048 bit should be fine here, 4096 on the ESP might be "paranoid mode"
  //   (in generel: shorter key = faster but less secure)
  // - Distinguished name: The name of the host as used in certificates.
  //   If you want to run your own DNS, the part after CN (Common Name) should match the DNS
  //   entry pointing to your ESP32. You can try to insert an IP there, but that's not really good style.
  // - Dates for certificate validity (optional, default is 2019-2029, both included)
  //   Format is YYYYMMDDhhmmss
  int createCertResult = createSelfSignedCert(
      *cert,
      KEYSIZE_1024,
      "CN=myesp32.local,O=FancyCompany,C=DE",
      "20190101000000",
      "20300101000000");

  // Now check if creating that worked
  if (createCertResult != 0)
  {
    Serial.printf("Creating certificate failed. Error Code = 0x%02X, check SSLCert.hpp for details", createCertResult);
    while (true)
      delay(500);
  }
  Serial.println("Creating the certificate was successful");

  // Setup Server using new certificate
  // secureServer = new HTTPSServer(cert);
  secureServer = new HTTPSServer(cert, 443, 4);

  // Connect to WiFi
  WiFi.begin(WIFI_SSID, PASSWORD);
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("\nConnected to WiFi");

  // Set up web server
  ResourceNode *nodeRoot = new ResourceNode("/", "GET", &handleRoot);
  ResourceNode *nodeCallback = new ResourceNode("/callback", "GET", &handleCallbackPage);
  ResourceNode *node404 = new ResourceNode("", "GET", &handle404);

  // Register nodes
  secureServer->registerNode(nodeRoot);
  secureServer->registerNode(nodeCallback);
  secureServer->setDefaultNode(node404); // For 404 handling

  // Start HTTPS server
  Serial.println("Starting HTTPS server...");
  secureServer->setDefaultHeader("Connection", "close");
  secureServer->start();

  if (secureServer->isRunning())
  {
    Serial.println("HTTPS server ready.");
    Serial.print("Access via: https://");
    Serial.println(WiFi.localIP());
  }

  // Show configuration screen
  display.clearDisplay();
  display.setTextColor(1);
  display.setTextWrap(false);
  display.setCursor(5, 38);
  display.print("ESP IP:" + WiFi.localIP().toString());
  display.drawBitmap(14, 15, configuring, 102, 15, 1);
  display.display();
}

void loop()
{
  unsigned long currentMillis = millis();

  // If not authenticated, handle web server requests
  if (!spotifyConnection.accessTokenSet)
  {
    secureServer->loop();
    return;
  }

  // Close server after authentication
  if (serverOn)
  {
    secureServer->stop();
    serverOn = false;
  }

  // Refresh access token if needed
  if ((currentMillis - spotifyConnection.tokenStartTime) / 1000 > spotifyConnection.tokenExpireTime - 60)
  {
    Serial.println("Refreshing token");
    if (spotifyConnection.refreshAuth())
    {
      Serial.println("Token refreshed successfully");
    }
  }

  // Update track info periodically
  if (currentMillis - lastApiRefresh > API_REFRESH_INTERVAL)
  {
    spotifyConnection.getTrackInfo();
    lastApiRefresh = currentMillis;
  }

  // Handle user input
  handleButtons();

  // Handle volume control
  handleVolumeControl();
}

  if (encSwBtn.isPressed())
  {
    Serial.println("Encoder switch pressed");
    spotifyConnection.togglePlay();
  }

  prevBtn.loop();
  playBtn.loop();
  nextBtn.loop();
  encSwBtn.loop();
}

// Volume control handler
void handleVolumeControl()
{
  if (spotifyConnection.volCtrl)
  {
    int newVolume = encoder.getCount() * 5;
    // Only update if volume change exceeds threshold
    if (abs(newVolume - spotifyConnection.currVol) > VOLUME_UPDATE_THRESHOLD)
    {
      spotifyConnection.adjustVolume(newVolume);
    }
  }
}

// Global flag for server state
bool serverOn = true;

void setup()
{
  // Initialize serial communication
  Serial.begin(9600);
  Serial.println("Starting ESP32 Spotify Player");

  // Initialize display
  delay(250); // Wait for OLED to initialize
  display.begin(I2C_ADDRESS, true);

  // Show splash screen
  display.clearDisplay();
  display.drawBitmap(9, 8, splash_screen, 112, 51, 1);
  display.display();
  delay(700);

  // Set up button pins
  pinMode(PLAY_BTN_PIN, INPUT_PULLUP);
  pinMode(PREV_BTN_PIN, INPUT_PULLUP);
  pinMode(NEXT_BTN_PIN, INPUT_PULLUP);
  pinMode(ENC_SW_PIN, INPUT_PULLUP);

  // Set up rotary encoder
  encoder.attachHalfQuad(ENC_DT_PIN, ENC_CLK_PIN);

  // Create SSL Certificate
  cert = new SSLCert();
  // secureServer->setMaxHeaderLength(4096);

  // Now, we use the function createSelfSignedCert to create private key and certificate.
  // The function takes the following paramters:
  // - Key size: 1024 or 2048 bit should be fine here, 4096 on the ESP might be "paranoid mode"
  //   (in generel: shorter key = faster but less secure)
  // - Distinguished name: The name of the host as used in certificates.
  //   If you want to run your own DNS, the part after CN (Common Name) should match the DNS
  //   entry pointing to your ESP32. You can try to insert an IP there, but that's not really good style.
  // - Dates for certificate validity (optional, default is 2019-2029, both included)
  //   Format is YYYYMMDDhhmmss
  int createCertResult = createSelfSignedCert(
      *cert,
      KEYSIZE_1024,
      "CN=myesp32.local,O=FancyCompany,C=DE",
      "20190101000000",
      "20300101000000");

  // Now check if creating that worked
  if (createCertResult != 0)
  {
    Serial.printf("Creating certificate failed. Error Code = 0x%02X, check SSLCert.hpp for details", createCertResult);
    while (true)
      delay(500);
  }
  Serial.println("Creating the certificate was successful");

  // Setup Server using new certificate
  // secureServer = new HTTPSServer(cert);
  secureServer = new HTTPSServer(cert, 443, 4);

  // Connect to WiFi
  WiFi.begin(WIFI_SSID, PASSWORD);
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("\nConnected to WiFi");

  // Set up web server
  ResourceNode *nodeRoot = new ResourceNode("/", "GET", &handleRoot);
  ResourceNode *nodeCallback = new ResourceNode("/callback", "GET", &handleCallbackPage);
  ResourceNode *node404 = new ResourceNode("", "GET", &handle404);

  // Register nodes
  secureServer->registerNode(nodeRoot);
  secureServer->registerNode(nodeCallback);
  secureServer->setDefaultNode(node404); // For 404 handling

  // Start HTTPS server
  Serial.println("Starting HTTPS server...");
  secureServer->setDefaultHeader("Connection", "close");
  secureServer->start();

  if (secureServer->isRunning())
  {
    Serial.println("HTTPS server ready.");
    Serial.print("Access via: https://");
    Serial.println(WiFi.localIP());
  }

  // Show configuration screen
  display.clearDisplay();
  display.setTextColor(1);
  display.setTextWrap(false);
  display.setCursor(5, 38);
  display.print("ESP IP:" + WiFi.localIP().toString());
  display.drawBitmap(14, 15, configuring, 102, 15, 1);
  display.display();
}

void loop()
{
  unsigned long currentMillis = millis();

  // If not authenticated, handle web server requests
  if (!spotifyConnection.accessTokenSet)
  {
    secureServer->loop();
    return;
  }

  // Close server after authentication
  if (serverOn)
  {
    secureServer->stop();
    serverOn = false;
  }

  // Refresh access token if needed
  if ((currentMillis - spotifyConnection.tokenStartTime) / 1000 > spotifyConnection.tokenExpireTime - 60)
  {
    Serial.println("Refreshing token");
    if (spotifyConnection.refreshAuth())
    {
      Serial.println("Token refreshed successfully");
    }
  }

  // Update track info periodically
  if (currentMillis - lastApiRefresh > API_REFRESH_INTERVAL)
  {
    spotifyConnection.getTrackInfo();
    lastApiRefresh = currentMillis;
  }

  // Handle user input
  handleButtons();

  // Handle volume control
  handleVolumeControl();
}