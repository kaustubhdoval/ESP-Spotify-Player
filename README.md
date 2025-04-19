  <h3 align="center">ESP Spotify Player</h3>

  <p align="center">
    An ESP32 based Spotify Player.
    <br />
    <a href="https://github.com/kaustubhdoval/ESP-Spotify-Player"><strong>Explore the docs »</strong></a>
    <br />
    <br />
    ·
    <a href="https://github.com/kaustubhdoval/ESP-Spotify-Player/issues">Report Bug</a>
    ·
    <a href="https://github.com/kaustubhdoval/ESP-Spotify-Player/issues">Request Feature</a>
    ·
  </p>
</p>

A Physical Spotify Controller using an ESP32, 3 Buttons, Rotary Encoder and a 128x64 OLED Screen (SH1106). 
<br/> <br/>
## User Guide
1. Plug the ESP to Power
2. Navigate to the ESP_IP and login to Your Spotify Account
3. Your Device should now be able to Control Music!
  
<br /><br />
## Setup
You need to have an application on the Spotify API Dashboard. Make sure that it has Web API enabled. You need to make the following modifications to the provided code:
  1. Change SSID and Password (line 45)
  2. Change Client Secret and ID (Spotify Web API Credentials) (line 49)
  3.  Run the program, You can see your ESPs IP on Serial Monitor (9600 Baud Rate) and on the OLED.
    You need to add the IP address of your ESP to REDIRECT_URI definition (line 51):
          http://YOUR_ESP_IP/callback

Now you need to add this Redirect URI to the Spotify App as well. After that is done, upload the updated code to the ESP and you are good to go!
<br /><br />
## Wiring
##### OLED <br />
SCK -> D22 <br />
SDA -> D21

##### Buttons <br />
  previous button   -> D5 <br />
  play/pause button -> D18 <br /> 
  next button       -> D19 <br />

##### Rotary Encoder <br />
  CLK -> D4 <br />
  DT  -> D2 <br />
  SW  -> D15
<br /><br />

## Dependencies
*	adafruit/Adafruit GFX Library@^1.12.0
* adafruit/Adafruit SH110X@^2.1.12
* madhephaestus/ESP32Encoder@^0.11.7
* bblanchon/ArduinoJson@^7.4.1
* arduinogetstarted/ezButton@^1.0.6

## Credits
* Heavily inspired by [MakeItForLess's Spotify Player](https://gitlab.com/makeitforless/spotify_controller)

