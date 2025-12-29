const char mainPage[] PROGMEM = R"=====(
<html><head><title>Spotify Auth</title>
<style>
body{font-family:Arial;background:#121212;color:#fff;text-align:center;margin-top:28%;}
a.spotify{display:inline-block;margin:15px;padding:12px 24px;background:#1DB954;color:#fff;text-decoration:none;border-radius:25px;font-weight:bold;}
</style></head>
<body>
<b>Hello World!</b>
<div><a class="spotify" href="https://accounts.spotify.com/authorize?response_type=code&client_id=%s&redirect_uri=%s&scope=user-modify-playback-state user-read-currently-playing user-read-playback-state user-library-modify user-library-read">Log in to Spotify</a></div>
</body></html>
)=====";

const char errorPage[] PROGMEM = R"=====(
<html><head><title>Error</title>
<style>
body{font-family:Arial,sans-serif;background:#121212;color:#fff;text-align:center;margin-top:50px;}
a.spotify{display:inline-block;margin:15px;padding:12px 24px;background:#1DB954;color:#fff;text-decoration:none;border-radius:25px;font-weight:bold;}
</style></head>
<body>
<b>Error!</b>
<div><a class="spotify" href="https://accounts.spotify.com/authorize?response_type=code&client_id=%s&redirect_uri=%s&scope=user-modify-playback-state user-read-currently-playing user-read-playback-state user-library-modify user-library-read">Retry</a></div>
</body></html>
)=====";