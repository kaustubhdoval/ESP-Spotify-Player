#ifndef SPOTIFYCLIENT_H
#define SPOTIFYCLIENT_H

// Increase Header Sizes
#undef HTTPS_CONNECTION_DATA_CHUNK_SIZE
#define HTTPS_CONNECTION_DATA_CHUNK_SIZE 4096

#undef HTTPS_REQUEST_MAX_REQUEST_LENGTH
#define HTTPS_REQUEST_MAX_REQUEST_LENGTH 8192

#undef HTTPS_MAX_HEADER_LENGTH
#define HTTPS_MAX_HEADER_LENGTH 4096

#include <Arduino.h>
#include <ArduinoJson.h>
#include <base64.h>
#include <ESP32Encoder.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPSServer.hpp>
#include <SSLCert.hpp>
#include <HTTPRequest.hpp>
#include <HTTPResponse.hpp>

#include "secrets.h"
#include "index.h"

using namespace httpsserver;

// Forward declarations
void handle404(HTTPRequest *req, HTTPResponse *res);
void handleRoot(HTTPRequest *req, HTTPResponse *res);
void handleCallbackPage(HTTPRequest *req, HTTPResponse *res);
void setDrawScreenCallback(void (*callback)());

// Spotify Root CA Certificate (extern declaration)
extern const char *spotify_root_ca;

// HTTPS Request Helper Function
bool httpsRequest(
    const char *host,
    const char *path,
    const String &method,
    const String &headers,
    const String &body,
    String &responseBody);

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

// Spotify Connection Class
class SpotConn
{
public:
    SpotConn();

    // Authentication methods
    bool getUserCode(const String &serverCode);
    bool refreshAuth();

    // Player control methods
    bool getTrackInfo();
    bool togglePlay();
    bool adjustVolume(int vol);
    bool skipForward();
    bool skipBack();

    // Status getters
    bool getStatus();
    bool getActiveStatus();
    float getCurrentPositionMs();
    int getCurrentVolume();
    SongDetails getCurrentSong();

    // Initialization
    void initialize();

    // Public member variables
    bool accessTokenSet;
    unsigned long tokenStartTime;
    int tokenExpireTime;
    SongDetails currentSong;
    float currentSongPositionMs;
    float lastSongPositionMs;
    int currVol;
    bool isPlaying;
    bool isActive;
    bool volCtrl;
    int volume;

private:
    String accessToken;
    String refreshToken;
};

// Global instances (extern declarations)
extern SpotConn spotifyConnection;
extern SSLCert *cert;
extern HTTPSServer *secureServer;

#endif