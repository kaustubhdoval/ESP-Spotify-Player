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

    // ---- Request line ----
    client.print(method + " " + path + " HTTP/1.1\r\n");
    client.print("Host: " + String(host) + "\r\n");
    client.print("User-Agent: ESP32\r\n");
    client.print("Connection: keep-alive\r\n"); // Changed from "close"

    // ---- Custom headers ----
    client.print(headers);

    // ---- Content-Length if body exists ----
    if (body.length() > 0)
    {
        client.print("Content-Length: ");
        client.print(body.length());
        client.print("\r\n");
    }

    client.print("\r\n");

    // ---- Body ----
    if (body.length() > 0)
    {
        client.print(body);
    }

    // ---- Read status line ----
    unsigned long timeout = millis();
    while (client.available() == 0)
    {
        if (millis() - timeout > 10000)
        {
            Serial.println("Request timeout");
            spotifyConnection.closeConnection(); // Force reconnect on timeout
            return false;
        }
        delay(10);
    }

    // Read and parse status line
    String statusLine = client.readStringUntil('\n');

    // ---- Read headers ----
    int contentLength = -1;
    bool chunked = false;

    while (client.connected() || client.available())
    {
        String line = client.readStringUntil('\n');
        if (line == "\r")
            break;

        // Parse Content-Length
        if (line.startsWith("Content-Length:"))
        {
            contentLength = line.substring(15).toInt();
        }
        // Check for chunked encoding
        if (line.indexOf("Transfer-Encoding: chunked") >= 0)
        {
            chunked = true;
        }
    }

    // ---- Read body ----
    responseBody = "";

    if (contentLength == 0)
    {
        // No body (e.g., 204 response)
        return true;
    }
    else if (contentLength > 0)
    {
        // Read exact content length
        int totalRead = 0;
        while (totalRead < contentLength && client.connected())
        {
            if (client.available())
            {
                char c = client.read();
                responseBody += c;
                totalRead++;
            }
        }
    }
    else if (chunked)
    {
        // Handle chunked encoding
        while (client.connected() || client.available())
        {
            String chunkSizeLine = client.readStringUntil('\n');
            int chunkSize = strtol(chunkSizeLine.c_str(), NULL, 16);

            if (chunkSize == 0)
                break; // Last chunk

            for (int i = 0; i < chunkSize && client.available(); i++)
            {
                responseBody += (char)client.read();
            }
            client.readStringUntil('\n');
        }
    }
    else
    {
        // No Content-Length header, read until connection closes or timeout
        timeout = millis();
        while (client.connected() || client.available())
        {
            if (client.available())
            {
                responseBody += client.readString();
                timeout = millis();
            }
            if (millis() - timeout > 2000)
                break; // 2 second timeout for remaining data
        }
    }

    return true;
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
        externalDrawScreen();
    }

    return success;
}

bool SpotConn::togglePlay()
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

    getTrackInfo();
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

    getTrackInfo();
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
        delay(100);

        if (!secureClient.connect(host, 443))
        {
            Serial.println("Connection failed");
            return false;
        }

        Serial.println("Connected to " + String(host));
        requestCount = 0;
    }

    lastConnectionTime = currentTime;
    requestCount++;
    return true;
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