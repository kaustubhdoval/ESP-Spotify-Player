const char mainPage[] PROGMEM = R"=====(
<html>
  <head>
    <title>Spotify Auth</title>
  </head>
  <body>
    <center>
      <b>Hello World!.... </b>
      <a
        href="https://accounts.spotify.com/authorize?response_type=code&client_id=%s&redirect_uri=%s&scope=user-modify-playback-state user-read-currently-playing user-read-playback-state user-library-modify user-library-read"
        >Log in to spotify</a
      >
    </center>
  </body>
</html>
)====="; const char errorPage[] PROGMEM = R"=====(
<html>
  <head>
    <title>Spotify Auth B</title>
  </head>
  <body>
    <center>
      <b>Hello World.... </b>
      <a
        href="https://accounts.spotify.com/authorize?response_type=code&client_id=%s&redirect_uri=%s&scope=user-modify-playback-state user-read-currently-playing user-read-playback-state user-library-modify user-library-read"
        >Log in to spotify</a
      >
    </center>
  </body>
</html>
)=====";
