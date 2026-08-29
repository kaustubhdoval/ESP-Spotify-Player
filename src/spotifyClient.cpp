#include "spotifyClient.h"

// Spotify Root CA Certificate
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
    "CCsGAQUFBzABhhhodHRwOi8vb2NzcC5kaW5pY2VydC5jb20wQAYIKwYBBQUHMAKG\n"
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
    "-----END CERTIFICATE-----\n"
    // Starfield Root Certificate Authority - G2
    // accounts.spotify.com (the OAuth token endpoint) chains to this root,
    // while api.spotify.com chains to the DigiCert root above -- both are
    // needed since mbedTLS accepts multiple concatenated CA certs here.
    "-----BEGIN CERTIFICATE-----\n"
    "MIID3TCCAsWgAwIBAgIBADANBgkqhkiG9w0BAQsFADCBjzELMAkGA1UEBhMCVVMx\n"
    "EDAOBgNVBAgTB0FyaXpvbmExEzARBgNVBAcTClNjb3R0c2RhbGUxJTAjBgNVBAoT\n"
    "HFN0YXJmaWVsZCBUZWNobm9sb2dpZXMsIEluYy4xMjAwBgNVBAMTKVN0YXJmaWVs\n"
    "ZCBSb290IENlcnRpZmljYXRlIEF1dGhvcml0eSAtIEcyMB4XDTA5MDkwMTAwMDAw\n"
    "MFoXDTM3MTIzMTIzNTk1OVowgY8xCzAJBgNVBAYTAlVTMRAwDgYDVQQIEwdBcml6\n"
    "b25hMRMwEQYDVQQHEwpTY290dHNkYWxlMSUwIwYDVQQKExxTdGFyZmllbGQgVGVj\n"
    "aG5vbG9naWVzLCBJbmMuMTIwMAYDVQQDEylTdGFyZmllbGQgUm9vdCBDZXJ0aWZp\n"
    "Y2F0ZSBBdXRob3JpdHkgLSBHMjCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoC\n"
    "ggEBAL3twQP89o/8ArFvW59I2Z154qK3A2FWGMNHttfKPTUuiUP3oWmb3ooa/RMg\n"
    "nLRJdzIpVv257IzdIvpy3Cdhl+72WoTsbhm5iSzchFvVdPtrX8WJpRBSiUZV9Lh1\n"
    "HOZ/5FSuS/hVclcCGfgXcVnrHigHdMWdSL5stPSksPNkN3mSwOxGXn/hbVNMYq/N\n"
    "Hwtjuzqd+/x5AJhhdM8mgkBj87JyahkNmcrUDnXMN/uLicFZ8WJ/X7NfZTD4p7dN\n"
    "dloedl40wOiWVpmKs/B/pM293DIxfJHP4F8R+GuqSVzRmZTRouNjWwl2tVZi4Ut0\n"
    "HZbUJtQIBFnQmA4O5t78w+wfkPECAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAO\n"
    "BgNVHQ8BAf8EBAMCAQYwHQYDVR0OBBYEFHwMMh+n2TB/xH1oo2Kooc6rB1snMA0G\n"
    "CSqGSIb3DQEBCwUAA4IBAQARWfolTwNvlJk7mh+ChTnUdgWUXuEok21iXQnCoKjU\n"
    "sHU48TRqneSfioYmUeYs0cYtbpUgSpIB7LiKZ3sx4mcujJUDJi5DnUox9g61DLu3\n"
    "4jd/IroAow57UvtruzvE03lRTs2Q9GcHGcg8RnoNAX3FWOdt5oUwF5okxBDgBPfg\n"
    "8n/Uqgr/Qh037ZTlZFkSIHc40zI+OIF1lnP6aI+xy84fxez6nH7PfrHxBy22/L/K\n"
    "pL/QlwVKvOoYKAKQvVR4CSFx09F9HdkWsKlhPdAKACL8x3vLCWRFCztAgfd9fDL1\n"
    "mMpYjn0q7pBZc2T5NnReJaH1ZgUufzkVqSr7UIuOhWn0\n"
    "-----END CERTIFICATE-----\n";

// Global instances
SpotConn spotifyConnection;
SSLCert *cert;
HTTPSServer *secureServer;

static void (*externalDrawScreen)() = nullptr;

// Add a public function to set it
void setDrawScreenCallback(void (*callback)())
{
    externalDrawScreen = callback;
}

// SpotConn constructor
SpotConn::SpotConn() : accessTokenSet(false),
                       tokenStartTime(0),
                       tokenExpireTime(0),
                       currentSongPositionMs(0),
                       lastSongPositionMs(0),
                       currVol(0),
                       isPlaying(false),
                       isActive(false),
                       volCtrl(false),
                       volume(0),
                       lastConnectionTime(0),
                       requestCount(0)
{
    secureClient.setCACert(spotify_root_ca);
    secureClient.setTimeout(10000);
    secureClient.setHandshakeTimeout(10000);
}

// SpotConn method implementations
bool SpotConn::getUserCode(const String &serverCode)
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

bool SpotConn::refreshAuth()
{
    JsonDocument doc;
    String response;

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

bool SpotConn::getTrackInfo()
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
        externalDrawScreen();
        return true;
    }

    // /v1/me/player returns 8-15 KB, most of it `available_markets` arrays we
    // never look at. Filtering keeps those out of the JsonDocument entirely --
    // far less heap churn and a much faster parse on the C3.
    JsonDocument filter;
    filter["is_playing"] = true;
    filter["progress_ms"] = true;
    filter["device"]["is_active"] = true;
    filter["device"]["supports_volume"] = true;
    filter["device"]["volume_percent"] = true;
    filter["item"]["name"] = true;
    filter["item"]["duration_ms"] = true;
    filter["item"]["uri"] = true;
    filter["item"]["artists"][0]["name"] = true;

    DeserializationError error =
        deserializeJson(doc, response, DeserializationOption::Filter(filter));
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

    externalDrawScreen();

    return success;
}

bool SpotConn::togglePlay()
{
    String path = "/v1/me/player/";
    path += isPlaying ? "pause" : "play";

    // Update Screen BEFORE sending request
    bool oldState = isPlaying;
    isPlaying = !isPlaying;
    externalDrawScreen(); // Show change immediately

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
        // isPlaying already holds the new state from the optimistic update
        // above -- flipping again here would revert the screen.
        Serial.println(isPlaying ? "Now playing" : "Now paused");
    }
    else
    {
        Serial.println("Error toggling playback");
        isPlaying = oldState;
        externalDrawScreen(); // roll the optimistic update back on screen
    }

    return ok;
}

bool SpotConn::adjustVolume(int vol)
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

bool SpotConn::skipForward()
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

    return ok;
}

bool SpotConn::skipBack()
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

    return ok;
}

bool SpotConn::getStatus()
{
    return isPlaying;
}

bool SpotConn::getActiveStatus()
{
    return isActive;
}

float SpotConn::getCurrentPositionMs()
{
    return currentSongPositionMs;
}

int SpotConn::getCurrentVolume()
{
    return volume;
}

SongDetails SpotConn::getCurrentSong()
{
    return currentSong;
}

void SpotConn::initialize()
{
    // Create SSL Certificate
    cert = new SSLCert();

    // Now, we use the function createSelfSignedCert to create private key and certificate.
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
    secureServer = new HTTPSServer(cert, 443, 4);

    // Connect to WiFi
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, PASSWORD);
    Serial.print("Connecting to WiFi");

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        Serial.print(".");
    }

    // Default power save (WIFI_PS_MIN_MODEM) parks the radio between DTIM
    // beacons, adding up to a beacon interval of latency to every round trip.
    // This player is mains powered, so keep the radio awake.
    WiFi.setSleep(false);

    Serial.println("\nConnected to WiFi");
    Serial.printf("WiFi RSSI: %d dBm (channel %d)\n", WiFi.RSSI(), WiFi.channel());

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
}

bool SpotConn::ensureConnection(const char *host)
{
    unsigned long currentTime = millis();

    // Check if connection is stale or needs refresh
    bool needsReconnect = false;

    if (!secureClient.connected())
    {
        Serial.println("Connection lost, reconnecting...");
        needsReconnect = true;
    }
    else if (connectedHost != host)
    {
        Serial.println("Target host changed, reconnecting...");
        needsReconnect = true;
    }
    else if (currentTime - lastConnectionTime > CONNECTION_TIMEOUT)
    {
        Serial.println("Connection idle too long, reconnecting...");
        needsReconnect = true;
    }
    else if (requestCount >= MAX_REQUESTS_PER_CONNECTION)
    {
        Serial.println("Max requests reached, reconnecting...");
        needsReconnect = true;
    }

    if (needsReconnect)
    {
        secureClient.stop();

        if (!secureClient.connect(host, 443))
        {
            Serial.println("Connection failed");
            return false;
        }

        Serial.println("Connected to " + String(host));
        connectedHost = host;
        requestCount = 0;
    }

    lastConnectionTime = currentTime;
    requestCount++;
    return true;
}

// Reconnects *now*, while nothing is waiting on it, if the connection is in a
// state that would force ensureConnection() to reconnect on the next request.
// Without this the 1-3s TLS handshake lands on whichever request comes next --
// often a button press.
void SpotConn::maintainConnection(const char *host)
{
    if (!accessTokenSet)
    {
        return;
    }

    bool stale =
        !secureClient.connected() ||
        connectedHost != host ||
        (millis() - lastConnectionTime > CONNECTION_TIMEOUT) ||
        (requestCount >= MAX_REQUESTS_PER_CONNECTION - 2);

    if (!stale)
    {
        return;
    }

    // Back off after a failure, otherwise a downed network turns this into a
    // tight connect() loop that starves the rest of loop().
    static unsigned long prewarmRetryAfter = 0;
    if (prewarmRetryAfter != 0 && (long)(millis() - prewarmRetryAfter) < 0)
    {
        return;
    }

    Serial.println("Pre-warming TLS connection");
    secureClient.stop();

    if (!secureClient.connect(host, 443))
    {
        Serial.println("Pre-warm connect failed, will retry on next request");
        prewarmRetryAfter = millis() + 5000;
        return;
    }

    prewarmRetryAfter = 0;

    connectedHost = host;
    requestCount = 0;
    lastConnectionTime = millis();
}

void SpotConn::closeConnection()
{
    if (secureClient.connected())
    {
        secureClient.stop();
        Serial.println("Connection closed");
    }
    requestCount = 0;
}

// HTTPS Request Helper Function
bool httpsRequest(
    const char *host,
    const char *path,
    const String &method,
    const String &headers,
    const String &body,
    String &responseBody)
{
    // Use the persistent connection from spotifyConnection
    if (!spotifyConnection.ensureConnection(host))
    {
        Serial.println("Failed to ensure connection");
        return false;
    }

    WiFiClientSecure &client = spotifyConnection.secureClient;

    // Build the whole request in one buffer and write it once. Every separate
    // client.print() becomes its own TLS record and its own TCP segment, so
    // splitting the request across seven writes invited Nagle/delayed-ACK
    // stalls worth tens to hundreds of ms.
    String request;
    request.reserve(160 + headers.length() + body.length());

    request += method;
    request += ' ';
    request += path;
    request += " HTTP/1.1\r\nHost: ";
    request += host;
    request += "\r\nUser-Agent: ESP32\r\nConnection: keep-alive\r\n";
    request += headers;

    if (body.length() > 0)
    {
        request += "Content-Length: ";
        request += body.length();
        request += "\r\n";
    }

    request += "\r\n";
    request += body;

    client.print(request);

    // ---- Read the response ----
    // Nothing below may use readStringUntil()/readString(). Those go through
    // Stream::timedRead(), which on arduino-esp32 is a *busy spin with no
    // yield*, once per byte, against a non-blocking socket. On the dual-core
    // devkit that spin ran on core 1 while WiFi/lwIP owned core 0 and cost
    // nothing; the single-core C3 has no such luxury -- the spin starves the
    // lwIP task it is waiting on. That, plus a missed Content-Length dropping
    // us into readString()'s full 10s timeout spin, was the ~25s per request.
    String pending; // bytes pulled off the socket but not yet consumed
    int consumed = 0;
    bool headersDone = false;
    int contentLength = -1;
    int statusCode = 0;
    bool chunked = false;
    unsigned long lastData = millis();

    responseBody = "";

    // --- Read and parse the header block ---
    {
        char buf[512];

        while (!headersDone)
        {
            int n = client.read((uint8_t *)buf, sizeof(buf));

            if (n > 0)
            {
                pending.concat(buf, n);
                lastData = millis();

                int end = pending.indexOf("\r\n\r\n");
                if (end < 0)
                {
                    if (pending.length() > 4096)
                    {
                        Serial.println("Response headers too large, aborting");
                        spotifyConnection.closeConnection();
                        return false;
                    }
                    continue;
                }

                // Parse the header lines, then keep whatever followed them --
                // the first bytes of the body usually arrive in the same read.
                int lineStart = 0;
                bool isStatusLine = true;

                while (lineStart < end)
                {
                    int lineEnd = pending.indexOf("\r\n", lineStart);
                    if (lineEnd < 0 || lineEnd > end)
                    {
                        break;
                    }

                    String line = pending.substring(lineStart, lineEnd);
                    lineStart = lineEnd + 2;

                    if (isStatusLine)
                    {
                        isStatusLine = false;
                        // "HTTP/1.1 204 No Content" -> 204
                        int sp = line.indexOf(' ');
                        if (sp > 0)
                        {
                            statusCode = line.substring(sp + 1, sp + 4).toInt();
                        }
                        continue;
                    }

                    // HTTP header names are case-insensitive and Spotify's edge
                    // sends them lowercase. Matching "Content-Length:" exactly
                    // meant contentLength stayed -1 on every single response,
                    // which dropped us into the read-until-close path below.
                    String lower = line;
                    lower.toLowerCase();

                    if (lower.startsWith("content-length:"))
                    {
                        contentLength = line.substring(15).toInt();
                    }
                    else if (lower.indexOf("transfer-encoding:") >= 0 &&
                             lower.indexOf("chunked") >= 0)
                    {
                        chunked = true;
                    }
                }

                consumed = end + 4;
                headersDone = true;
            }
            else
            {
                if (!client.connected() && client.available() == 0)
                {
                    Serial.println("Connection closed before headers completed");
                    spotifyConnection.closeConnection();
                    return false;
                }
                if (millis() - lastData > 10000)
                {
                    Serial.println("Request timeout");
                    spotifyConnection.closeConnection();
                    return false;
                }
                delay(1); // real vTaskDelay -- yields the core, unlike yield()
            }
        }
    }

    // Body bytes that arrived alongside the headers.
    String carryOver = pending.substring(consumed);
    pending = String();

    // --- Read the body ---
    // 204/304 are defined to carry no body and Spotify sends them without a
    // Content-Length, so without this they fell through to the read-until-close
    // path and burned its full 2s idle timeout on every no-active-device poll.
    if (contentLength == 0 || statusCode == 204 || statusCode == 304)
    {
        return true;
    }
    else if (contentLength > 0)
    {
        responseBody.reserve(contentLength + 1);
        responseBody += carryOver;

        // Never let a long carry-over run past the declared body length
        if ((int)responseBody.length() > contentLength)
        {
            responseBody.remove(contentLength);
        }

        char buf[512];
        lastData = millis();

        while ((int)responseBody.length() < contentLength)
        {
            int want = contentLength - (int)responseBody.length();
            if (want > (int)sizeof(buf))
            {
                want = sizeof(buf);
            }

            int n = client.read((uint8_t *)buf, want);

            if (n > 0)
            {
                responseBody.concat(buf, n);
                lastData = millis();
            }
            else
            {
                if (!client.connected() && client.available() == 0)
                {
                    break;
                }
                if (millis() - lastData > 5000)
                {
                    Serial.println("Body read stalled, aborting");
                    break;
                }
                delay(1);
            }
        }
    }
    else if (chunked)
    {
        // Decode chunks out of a sliding window, topping it up from the socket.
        String window = carryOver;
        int pos = 0;
        char buf[512];
        lastData = millis();
        bool done = false;

        while (!done)
        {
            // Need a complete "<hex>\r\n" size line
            int crlf = window.indexOf("\r\n", pos);
            if (crlf < 0)
            {
                int n = client.read((uint8_t *)buf, sizeof(buf));
                if (n > 0)
                {
                    window.concat(buf, n);
                    lastData = millis();
                }
                else if ((!client.connected() && client.available() == 0) ||
                         millis() - lastData > 5000)
                {
                    break;
                }
                else
                {
                    delay(1);
                }
                continue;
            }

            int chunkSize = strtol(window.substring(pos, crlf).c_str(), NULL, 16);
            int dataStart = crlf + 2;

            if (chunkSize == 0)
            {
                break; // last chunk
            }

            // Wait until the whole chunk plus its trailing CRLF is buffered
            while ((int)window.length() < dataStart + chunkSize + 2)
            {
                int n = client.read((uint8_t *)buf, sizeof(buf));
                if (n > 0)
                {
                    window.concat(buf, n);
                    lastData = millis();
                }
                else if ((!client.connected() && client.available() == 0) ||
                         millis() - lastData > 5000)
                {
                    done = true;
                    break;
                }
                else
                {
                    delay(1);
                }
            }

            if (done)
            {
                break;
            }

            responseBody.concat(window.c_str() + dataStart, chunkSize);
            pos = dataStart + chunkSize + 2; // skip the chunk's trailing CRLF

            // Drop what we have consumed so the window cannot grow unbounded
            if (pos > 1024)
            {
                window.remove(0, pos);
                pos = 0;
            }
        }
    }
    else
    {
        // Neither Content-Length nor chunked: the only case where we genuinely
        // have to read until the peer closes. Bounded, and no per-byte spin.
        responseBody += carryOver;

        char buf[512];
        lastData = millis();

        while (client.connected() || client.available())
        {
            int n = client.read((uint8_t *)buf, sizeof(buf));
            if (n > 0)
            {
                responseBody.concat(buf, n);
                lastData = millis();
            }
            else
            {
                if (millis() - lastData > 2000)
                {
                    break;
                }
                delay(1);
            }
        }
    }

    return true;
}