#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "MicroRC-Car";
const char* password = "password123";

WebServer server(80);

const int IN1 = 18;
const int IN2 = 19;
const int IN3 = 25;
const int IN4 = 26;

unsigned long lastCommand = 0;
const unsigned long FAILSAFE_MS = 600;
bool moving = false;

const char HTML_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,
 initial-scale=1, maximum-scale=1,
 user-scalable=no">
<title>Micro RC</title>
<style>
:root {
  --bg:#0b101b;
  --panel:#141f30;
  --cyan:#38d9ef;
  --text:#e2faff;
  --muted:#8191a8;
}
* { box-sizing:border-box; }
body {
  margin:0;
  min-height:100dvh;
  background:var(--bg);
  color:var(--text);
  font-family:system-ui,-apple-system,sans-serif;
  display:flex;
  justify-content:center;
  align-items:center;
  padding:18px;
  touch-action:manipulation;
}
.dashboard {
  width:100%;
  max-width:430px;
}
header {
  display:flex;
  align-items:center;
  justify-content:space-between;
  margin-bottom:22px;
}
.brand { font-size:22px; font-weight:900; }
.brand span { color:var(--cyan); }
.subtitle {
  color:var(--muted);
  font-size:10px;
  letter-spacing:2px;
}
.connection {
  font-size:11px;
  padding:9px 12px;
  background:var(--panel);
  border-radius:20px;
}
.dot {
  display:inline-block;
  width:8px;
  height:8px;
  border-radius:50%;
  background:#ff5252;
  margin-right:6px;
}
.dot.online { background:#35dc93; }
.display {
  background:linear-gradient(145deg,#16283a,#101827);
  border:1px solid #24394d;
  border-radius:22px;
  padding:28px 16px;
  text-align:center;
}
.display small {
  color:var(--muted);
  letter-spacing:2px;
}
#action {
  font-size:29px;
  font-weight:900;
  margin:14px 0 5px;
  color:var(--cyan);
}
#detail { font-size:12px; color:var(--muted); }
.info {
  display:grid;
  grid-template-columns:1fr 1fr;
  gap:12px;
  margin-top:14px;
}
.info div {
  background:var(--panel);
  border-radius:14px;
  padding:14px;
}
.info small { color:var(--muted); }
.info strong { display:block; margin-top:5px; }
.controls-title {
  text-align:center;
  margin:28px 0 18px;
  font-size:11px;
  letter-spacing:2px;
  color:var(--muted);
}
.pad {
  display:grid;
  grid-template-columns:repeat(3,minmax(0,1fr));
  gap:12px;
  max-width:340px;
  margin:auto;
}
.drive {
  aspect-ratio:1;
  background:#1b2c41;
  border:1px solid #3b6276;
  border-radius:20px;
  color:#8beafa;
  font-size:32px;
  font-weight:bold;
  cursor:pointer;
  touch-action:none;
  user-select:none;
  -webkit-user-select:none;
}
.drive.active {
  background:#087e91;
  color:white;
  transform:scale(.96);
}
#forward { grid-column:2; grid-row:1; }
#left { grid-column:1; grid-row:2; }
#stop { grid-column:2; grid-row:2; }
#right { grid-column:3; grid-row:2; }
#reverse { grid-column:2; grid-row:3; }
#stop {
  background:#8b2636;
  color:white;
  border-color:#bc4455;
  font-size:16px;
}
footer {
  text-align:center;
  color:var(--muted);
  font-size:12px;
  margin-top:30px;
}
</style>
</head>
<body>
<div class="dashboard">
<header>
  <div>
    <div class="brand">MICRO<span>RC</span></div>
    <div class="subtitle">V2.0</div>
  </div>
  <div class="connection">
    <span class="dot" id="dot"></span>
    <span id="connection">CHECKING</span>
  </div>
</header>

<section class="display">
  <small>DRIVE STATUS</small>
  <div id="action">STANDBY</div>
  <div id="detail">Hold a button to drive</div>
</section>

<div class="info">
  <div>
    <small>CONTROL</small>
    <strong>Wi-Fi AP</strong>
  </div>
  <div>
    <small>SAFETY</small>
    <strong>Auto Stop</strong>
  </div>
</div>

<div class="controls-title">DIRECTIONAL CONTROLS</div>
<div class="pad">
  <button class="drive" id="forward"
    data-cmd="forward">▲</button>
  <button class="drive" id="left"
    data-cmd="left">◀</button>
  <button class="drive" id="stop">STOP</button>
  <button class="drive" id="right"
    data-cmd="right">▶</button>
  <button class="drive" id="reverse"
    data-cmd="reverse">▼</button>
</div>

<footer>
  Hold to drive · Release to stop<br>
  Keyboard: W A S D / Arrow Keys / Space
</footer>
</div>

<script>
const action = document.getElementById('action');
const detail = document.getElementById('detail');
const dot = document.getElementById('dot');
const connection = document.getElementById('connection');

let active = null;
let timer = null;
let pending = false;
let requestController = null;

const labels = {
  forward:'FORWARD',
  reverse:'REVERSE',
  left:'TURN LEFT',
  right:'TURN RIGHT',
  stop:'STANDBY'
};

function showConnection(ok) {
  dot.classList.toggle('online',ok);
  connection.textContent = ok ? 'CONNECTED' : 'OFFLINE';
}

async function send(cmd) {
  // Only one outstanding request at a time.
  if (pending) return;
  pending = true;
  requestController = new AbortController();
  const timeout = setTimeout(
    () => requestController.abort(), 350
  );
  try {
    const r = await fetch('/' + cmd, {
      cache:'no-store',
      signal:requestController.signal
    });
    showConnection(r.ok);
  } catch(e) {
    showConnection(false);
  } finally {
    clearTimeout(timeout);
    pending = false;
    requestController = null;
  }
}

function start(cmd) {
  if (active === cmd) return;
  stop();
  active = cmd;
  action.textContent = labels[cmd];
  detail.textContent = 'Driving · Hold to continue';
  document.getElementById(cmd).classList.add('active');
  send(cmd);
  timer = setInterval(() => {
    if (active === cmd) send(cmd);
  }, 150);
}

function stop() {
  if (timer) clearInterval(timer);
  timer = null;
  if (active)
    document.getElementById(active).classList.remove('active');
  active = null;
  action.textContent = 'STANDBY';
  detail.textContent = 'Motors stopped';
  // Retry stop briefly if a previous request is in flight.
  const tryStop = () => {
    if (!pending) send('stop');
    else setTimeout(tryStop, 40);
  };
  tryStop();
}

document.querySelectorAll('[data-cmd]').forEach(btn => {
  btn.addEventListener('pointerdown',e => {
    e.preventDefault();
    btn.setPointerCapture(e.pointerId);
    start(btn.dataset.cmd);
  });
  btn.addEventListener('pointerup',stop);
  btn.addEventListener('pointercancel',stop);
  btn.addEventListener('lostpointercapture',stop);
});
document.getElementById('stop')
  .addEventListener('pointerdown',stop);

const keys = {
  w:'forward', ArrowUp:'forward',
  s:'reverse', ArrowDown:'reverse',
  a:'left', ArrowLeft:'left',
  d:'right', ArrowRight:'right'
};
let heldKey = null;
document.addEventListener('keydown',e => {
  if (e.code === 'Space') {
    e.preventDefault();
    heldKey = null;
    stop();
    return;
  }
  const cmd = keys[e.key];
  if (!cmd) return;
  e.preventDefault();
  if (e.repeat) return;
  heldKey = e.key;
  start(cmd);
});
document.addEventListener('keyup',e => {
  if (e.key === heldKey) {
    heldKey = null;
    stop();
  }
});
window.addEventListener('blur',stop);
document.addEventListener('visibilitychange',() => {
  if (document.hidden) stop();
});

// Check initial connection.
send('stop');
</script>
</body>
</html>
)rawliteral";

void motorsOff() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  moving = false;
}

void drive(int a, int b, int c, int d) {
  digitalWrite(IN1, a);
  digitalWrite(IN2, b);
  digitalWrite(IN3, c);
  digitalWrite(IN4, d);
  moving = true;
  lastCommand = millis();
}

void handleCommand() {
  String path = server.uri();

  if (path == "/forward")
    drive(HIGH, LOW, HIGH, LOW);
  else if (path == "/reverse")
    drive(LOW, HIGH, LOW, HIGH);
  else if (path == "/left")
    drive(LOW, HIGH, HIGH, LOW);
  else if (path == "/right")
    drive(HIGH, LOW, LOW, HIGH);
  else if (path == "/stop")
    motorsOff();
  else {
    server.send(404, "text/plain", "Unknown command");
    return;
  }

  server.send(200, "text/plain", "OK");
}

void setup() {
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  motorsOff();

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);

  server.on("/", []() {
    server.send_P(200, "text/html", HTML_PAGE);
  });

  server.on("/forward", handleCommand);
  server.on("/reverse", handleCommand);
  server.on("/left", handleCommand);
  server.on("/right", handleCommand);
  server.on("/stop", handleCommand);

  server.begin();
}

void loop() {
  server.handleClient();

  if (moving && millis() - lastCommand > FAILSAFE_MS) {
    motorsOff();
  }
}
