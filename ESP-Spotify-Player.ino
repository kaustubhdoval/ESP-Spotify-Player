/*DIY ESP Spotify Player
- Kaustubh Doval

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
(3) Run the program, Serial monitor will output your ESPs IP.
    the IP address of your ESP should be in callback (line 51):
          http://YOUR_ESP_IP/callback
(4) Add this callback to spotify API portal as well
(5) You are good to go!
*/
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <ESP32Encoder.h>
#include <ArduinoJson.h>
#include <base64.h>

// Include WiFi and http client
#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <WiFiClientSecure.h>
#include <ssl_client.h>

// Load tabs attached to this sketch
#include "index.h"

// WiFi credentials
#define WIFI_SSID "YOUR_WIFI_SSID"
#define PASSWORD "YOUR_WIFI_PASSWORD"

// Spotify API credentials
#define CLIENT_ID "SPOTIFY_CLIENT_ID"
#define CLIENT_SECRET "SPOTIFY_CLIENT_SECRET"
#define REDIRECT_URI "http://YOUR_ESP_IP/callback"


//Pin Definitions
#define prev_btn_pin 5
#define play_btn_pin 18
#define next_btn_pin 19
#define enc_clk_pin 4
#define enc_dt_pin 2
#define enc_sw_pin 15

//OLED Definitions
#define I2C_ADDRESS 0x3c  //If diff OLED, this might be diff use i2c scanner
#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels
#define OLED_RESET -1     //   QT-PY / XIAO

//Create OLED Object
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//Rotary Encoder Object
ESP32Encoder encoder;

//Wifi Objects
WiFiClientSecure secureClient;
HTTPClient https;
WebServer server(80);  //Server on port 80

//Variables
bool isPlaying;  //if state = 0 -> Paused, if state = 1 -> Playing
int volume;

int play_btn_prevstate = HIGH;
int prev_btn_prevstate = HIGH;
int next_btn_prevstate = HIGH;
int prev_btn_state;
int play_btn_state;
int next_btn_state;

int sw_btn_prevstate = HIGH;
int sw_btn_state;

void change_state();
void prev_song();
void next_song();

String getValue(HTTPClient &http, String key) {
  bool found = false, look = false, seek = true;
  int ind = 0;
  String ret_str = "";

  int len = http.getSize();
  char char_buff[1];
  WiFiClient *stream = http.getStreamPtr();
  while (http.connected() && (len > 0 || len == -1)) {
    size_t size = stream->available();
    // Serial.print("Size: ");
    // Serial.println(size);
    if (size) {
      int c = stream->readBytes(char_buff, ((size > sizeof(char_buff)) ? sizeof(char_buff) : size));
      if (found) {
        if (seek && char_buff[0] != ':') {
          continue;
        } else if (char_buff[0] != '\n') {
          if (seek && char_buff[0] == ':') {
            seek = false;
            int c = stream->readBytes(char_buff, 1);
          } else {
            ret_str += char_buff[0];
          }
        } else {
          break;
        }

        // Serial.print("get: ");
        // Serial.println(get);
      } else if ((!look) && (char_buff[0] == key[0])) {
        look = true;
        ind = 1;
      } else if (look && (char_buff[0] == key[ind])) {
        ind++;
        if (ind == key.length()) found = true;
      } else if (look && (char_buff[0] != key[ind])) {
        ind = 0;
        look = false;
      }
    }
  }
  //   Serial.println(*(ret_str.end()));
  //   Serial.println(*(ret_str.end()-1));
  //   Serial.println(*(ret_str.end()-2));
  if (*(ret_str.end() - 1) == ',') {
    ret_str = ret_str.substring(0, ret_str.length() - 1);
  }
  return ret_str;
}

//http response struct
struct httpResponse {
  int responseCode;
  String responseMessage;
};

struct songDetails {
  int durationMs;
  String album;
  String artist;
  String song;
  String Id;
  bool isLiked;
};


//Create spotify connection class
//Code 'inspired' from Make it for Less
class SpotConn {
public:
  SpotConn() {
    // Set up WiFiClientSecure for SSL
    secureClient.setInsecure();  // Insecure means it won't validate SSL certificates
  }

  bool getUserCode(String serverCode) {
    https.begin(secureClient, "https://accounts.spotify.com/api/token");
    String auth = "Basic " + base64::encode(String(CLIENT_ID) + ":" + String(CLIENT_SECRET));
    https.addHeader("Authorization", auth);
    https.addHeader("Content-Type", "application/x-www-form-urlencoded");
    https.addHeader("Content-Length", "0");  // Add Content-Length header
    String requestBody = "grant_type=authorization_code&code=" + serverCode + "&redirect_uri=" + String(REDIRECT_URI);

    // Send the POST request to the Spotify API
    int httpResponseCode = https.POST(requestBody);

    if (httpResponseCode == HTTP_CODE_OK) {
      String response = https.getString();
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, response);
      accessToken = String((const char *)doc["access_token"]);
      refreshToken = String((const char *)doc["refresh_token"]);
      tokenExpireTime = doc["expires_in"];
      tokenStartTime = millis();
      accessTokenSet = true;
      Serial.println(accessToken);
      Serial.println(refreshToken);
    } else {
      Serial.print("Error: ");
      Serial.println(httpResponseCode);
      Serial.println(https.getString());  // Print the error response from Spotify
      https.end();
      return false;  // Early exit on failure
    }

    // Disconnect from the Spotify API
    https.end();
    return accessTokenSet;
  }

  bool refreshAuth() {
    https.begin(secureClient, "https://accounts.spotify.com/api/token");
    String auth = "Basic " + base64::encode(String(CLIENT_ID) + ":" + String(CLIENT_SECRET));
    https.addHeader("Authorization", auth);
    https.addHeader("Content-Type", "application/x-www-form-urlencoded");
    https.addHeader("Content-Length", "0");  // Add Content-Length header

    String requestBody = "grant_type=refresh_token&refresh_token=" + String(refreshToken);
    int httpResponseCode = https.POST(requestBody);

    accessTokenSet = false;

    if (httpResponseCode == HTTP_CODE_OK) {
      String response = https.getString();
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, response);
      accessToken = String((const char *)doc["access_token"]);

      if (doc.containsKey("refresh_token")) {
        refreshToken = doc["refresh_token"].as<String>();
      }

      tokenExpireTime = doc["expires_in"];
      tokenStartTime = millis();
      accessTokenSet = true;
      Serial.println(accessToken);
      Serial.println(refreshToken);
    } else {
      Serial.print("Error: ");
      Serial.println(httpResponseCode);
      Serial.println(https.getString());  // Print the error response from Spotify
      https.end();
      return false;  // Early exit on failure
    }

    https.end();
    return accessTokenSet;
  }

  bool getTrackInfo() {
    String url = "https://api.spotify.com/v1/me/player/currently-playing";
    https.useHTTP10(true);
    https.begin(secureClient, url);
    String auth = "Bearer " + String(accessToken);
    https.addHeader("Authorization", auth);
    https.addHeader("Content-Length", "0");  // Add Content-Length header
    int httpResponseCode = https.GET();
    bool success = false;
    String songId = "";
    bool refresh = false;
    // Check if the request was successful
    if (httpResponseCode == 200) {
      String currentSongProgress = getValue(https, "progress_ms");
      currentSongPositionMs = currentSongProgress.toFloat();


      String artistName = getValue(https, "name");
      String songDuration = getValue(https, "duration_ms");
      currentSong.durationMs = songDuration.toInt();
      String songName = getValue(https, "name");
      songId = getValue(https, "uri");
      String isPlay = getValue(https, "is_playing");
      isPlaying = (isPlay == "true");
      Serial.println(isPlay);
      // Serial.println(songId);
      songId = songId.substring(15, songId.length() - 1);
      // Serial.println(songId);
      https.end();

      currentSong.artist = artistName.substring(1, artistName.length() - 1);
      currentSong.song = songName.substring(1, songName.length() - 1);
      currentSong.Id = songId;
      success = true;
    } else {
      Serial.print("Error getting track info: ");
      Serial.println(httpResponseCode);
      // String response = https.getString();
      // Serial.println(response);
      https.end();
    }


    // Disconnect from the Spotify API
    if (success) {
      drawScreen();
      lastSongPositionMs = currentSongPositionMs;
    }
    return success;
  }

  //Display screen
  bool drawScreen() {
    display.clearDisplay();  // Clear previous display contents
    display.setTextColor(SH110X_WHITE);
    display.setTextSize(1);

    // Ensure no string longer than 20 characters is printed
    String songName = currentSong.song.length() > 20 ? currentSong.song.substring(0, 17) + "..." : currentSong.song;
    String artistName = currentSong.artist.length() > 20 ? currentSong.artist.substring(0, 17) + "..." : currentSong.artist;

    // Display the song name
    display.setCursor(0, 0);
    display.print(songName);

    // Display the artist name
    display.setCursor(0, 15);
    display.print(artistName);

    int barY = 30;      // Set the Y-coordinate for the bar
    int barHeight = 8;  // Set the height of the bar

    // Calculate the width of the progress bar based on the current song position
    int barWidth = map(currentSongPositionMs, 0, currentSong.durationMs, 0, SCREEN_WIDTH);

    // Draw the background of the progress bar
    display.drawRect(0, barY, SCREEN_WIDTH, barHeight, SH110X_WHITE);

    // Draw the filled portion of the progress bar
    display.fillRect(0, barY, barWidth, barHeight, SH110X_WHITE);

    display.display();

    // Display play/pause state
    if (isPlaying) {
      display.setCursor(45, 55);
      display.print("Playing");
    } else {
      display.setCursor(45, 55);
      display.print("Paused");
    }

    // Display volume
    display.setCursor(110, 55);
    display.print(volume);

    display.display();

    Serial.println(currentSong.song);
    return true;
  }


  bool togglePlay() {
    String url = "https://api.spotify.com/v1/me/player/" + String(isPlaying ? "pause" : "play");
    isPlaying = !isPlaying;
    https.begin(secureClient, url);
    String auth = "Bearer " + String(accessToken);
    https.addHeader("Authorization", auth);
    https.addHeader("Content-Length", "0");  // Add Content-Length header
    int httpResponseCode = https.PUT("");
    bool success = false;
    // Check if the request was successful
    if (httpResponseCode == 204) {
      // String response = https.getString();
      Serial.println((isPlaying ? "Paused" : "Playing"));
      success = true;
    } else {
      Serial.print("Error pausing or playing: ");
      Serial.println(httpResponseCode);
      String response = https.getString();
      Serial.println(response);
    }

    // Disconnect from the Spotify API
    https.end();
    getTrackInfo();
    return success;
  }

  bool adjustVolume(int vol) {
    // Ensure volume is within valid range (0-100)
    if (vol < 0 || vol > 100) {
      Serial.println("Volume must be between 0 and 100.");
      return false;
    }

    String url = "https://api.spotify.com/v1/me/player/volume?volume_percent=" + String(vol);

    // Add authorization header
    String auth = "Bearer " + String(accessToken);

    // Begin the HTTP request
    https.begin(secureClient, url);
    https.addHeader("Authorization", auth);
    https.addHeader("Content-Length", "0");  // Add Content-Length header

    // PUT request, no body needed
    int httpResponseCode = https.PUT("");  // Empty body for the PUT request

    bool success = false;
    // Check if the request was successful
    if (httpResponseCode == 204) {  // 204 No Content indicates success
      currVol = vol;
      Serial.println("Volume adjusted successfully.");
      success = true;
      drawScreen();
    } else {
      Serial.print("Error setting volume: ");
      Serial.println(httpResponseCode);
      String response = https.getString();
      Serial.println(response);
      success = false;
    }

    // Disconnect from the Spotify API
    https.end();
    return success;
  }


  bool skipForward() {
    String url = "https://api.spotify.com/v1/me/player/next";
    https.begin(secureClient, url);
    String auth = "Bearer " + String(accessToken);
    https.addHeader("Authorization", auth);
    https.addHeader("Content-Length", "0");  // Add Content-Length header
    int httpResponseCode = https.POST("");
    bool success = false;
    // Check if the request was successful
    if (httpResponseCode == 204) {
      // String response = https.getString();
      Serial.println("skipping forward");
      success = true;
    } else {
      Serial.print("Error skipping forward: ");
      Serial.println(httpResponseCode);
      String response = https.getString();
      Serial.println(response);
    }


    // Disconnect from the Spotify API
    https.end();
    getTrackInfo();
    return success;
  }
  bool skipBack() {
    String url = "https://api.spotify.com/v1/me/player/previous";
    https.begin(secureClient, url);
    String auth = "Bearer " + String(accessToken);
    https.addHeader("Authorization", auth);
    https.addHeader("Content-Length", "0");  // Add Content-Length header
    int httpResponseCode = https.POST("");
    bool success = false;
    // Check if the request was successful
    if (httpResponseCode == 204) {
      // String response = https.getString();
      Serial.println("skipping backward");
      success = true;
    } else {
      Serial.print("Error skipping backward: ");
      Serial.println(httpResponseCode);
      String response = https.getString();
      Serial.println(response);
    }


    // Disconnect from the Spotify API
    https.end();
    getTrackInfo();
    return success;
  }
  bool accessTokenSet = false;
  long tokenStartTime;
  int tokenExpireTime;
  songDetails currentSong;
  float currentSongPositionMs;
  float lastSongPositionMs;
  int currVol;
private:
  WiFiClientSecure secureClient;
  HTTPClient https;
  String accessToken;
  String refreshToken;
};

//Spotify Connection object
SpotConn spotifyConnection;

//Web server callbacks
void handleRoot() {
  Serial.println("handling root");
  char page[500];
  sprintf(page, mainPage, CLIENT_ID, REDIRECT_URI);
  server.send(200, "text/html", String(page) + "\r\n");  //Send web page
}

void handleCallbackPage() {
  if (!spotifyConnection.accessTokenSet) {
    if (server.arg("code") == "") {  //Parameter not found
      char page[500];
      sprintf(page, errorPage, CLIENT_ID, REDIRECT_URI);
      server.send(200, "text/html", String(page));  //Send web page
    } else {                                        //Parameter found
      if (spotifyConnection.getUserCode(server.arg("code"))) {
        server.send(200, "text/html", "Spotify setup complete Auth refresh in :" + String(spotifyConnection.tokenExpireTime));
      } else {
        char page[500];
        sprintf(page, errorPage, CLIENT_ID, REDIRECT_URI);
        server.send(200, "text/html", String(page));  //Send web page
      }
    }
  } else {
    server.send(200, "text/html", "Spotify setup complete");
  }
}


long timeLoop;
long refreshLoop;
bool serverOn = true;

void setup() {
  //Wait for OLED to turn on + Start OLED
  delay(250);
  display.begin(I2C_ADDRESS, true);
  display.display();
  //Show Splashscreen + Clear
  delay(500);
  display.clearDisplay();

  //I CHANGED THIS
  spotifyConnection.drawScreen();

  Serial.begin(9600);
  Serial.println("Welcome to Project Log!");

  //Buttons PinMode
  pinMode(play_btn_pin, INPUT_PULLUP);
  pinMode(prev_btn_pin, INPUT_PULLUP);
  pinMode(next_btn_pin, INPUT_PULLUP);

  pinMode(enc_sw_pin, INPUT_PULLUP);

  encoder.attachHalfQuad(enc_dt_pin, enc_clk_pin);
  encoder.setCount(10);

  WiFi.begin(WIFI_SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi\n Ip is: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/callback", handleCallbackPage);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  //server.handleClient();
  if (spotifyConnection.accessTokenSet) {
    if (serverOn) {
      server.close();
      serverOn = false;
    }
    if ((millis() - spotifyConnection.tokenStartTime) / 1000 > spotifyConnection.tokenExpireTime) {
      Serial.println("refreshing token");
      if (spotifyConnection.refreshAuth()) {
        Serial.println("refreshed token");
      }
    }
    if ((millis() - refreshLoop) > 5000) {
      spotifyConnection.getTrackInfo();

      //spotifyConnection.drawScreen();
      refreshLoop = millis();
    }

    //Button Business
    prev_btn_state = digitalRead(prev_btn_pin);
    play_btn_state = digitalRead(play_btn_pin);
    next_btn_state = digitalRead(next_btn_pin);

    if (play_btn_prevstate == HIGH && play_btn_state == LOW) {
      change_state();
      delay(50);
    } else if (prev_btn_prevstate == HIGH && prev_btn_state == LOW) {
      prev_song();
      delay(50);
    } else if (next_btn_prevstate == HIGH && next_btn_state == LOW) {
      next_song();
      delay(50);
    }

    prev_btn_prevstate = prev_btn_state;
    play_btn_prevstate = play_btn_state;
    next_btn_prevstate = next_btn_state;

    sw_btn_state = digitalRead(enc_sw_pin);
    if (sw_btn_prevstate == HIGH && sw_btn_state == LOW) {
      Serial.println("Encoder Btn Pressed");
      delay(50);
    }
    sw_btn_prevstate = sw_btn_state;

    volume = encoder.getCount() * 5;
    if (abs(volume - spotifyConnection.currVol) > 2) {
      spotifyConnection.adjustVolume(volume);
    }
    timeLoop = millis();
  } else {
    server.handleClient();
  }
  // Serial.println(millis() - timeLoop);
  // timeLoop = millis();
}


void change_state() {
  Serial.println("Button PRESSED");
  //SPOTIFY REQUEST
  spotifyConnection.togglePlay();
}

void prev_song() {
  Serial.println("Previous Song");
  //SPOTIFY REQUEST
  spotifyConnection.skipBack();
}

void next_song() {
  Serial.println("Next Song");
  //SPOTIFY REQUEST
  spotifyConnection.skipForward();
}
