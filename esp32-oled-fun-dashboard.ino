#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ---------- Wi-Fi Configuration ----------
// Replace with your network credentials
const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Fallback Access Point if home Wi-Fi is unreachable
const char* AP_SSID  = "ESP32-OLED";
const char* AP_PASS  = "12345678";

WebServer server(80);

enum Mode { M_TEXT, M_EMOJI, M_ANIM, M_SNAKE, M_DINO };
Mode mode = M_TEXT;

const unsigned long FRAME_MS = 50;   // ~20 FPS
unsigned long lastFrame = 0;
unsigned long animFrame = 0;
int animId = 0;

// =====================================================================
//                             WEB PAGE
// =====================================================================
const char HTML_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1, user-scalable=no">
<title>ESP32 OLED Fun</title>
<style>
  * { box-sizing: border-box; }
  body { font-family: sans-serif; text-align: center; margin: 0; padding: 16px; background: #eef2f5; color: #222; -webkit-user-select: none; user-select: none; }
  .card { background: #fff; max-width: 380px; margin: auto; padding: 18px; border-radius: 14px; box-shadow: 0 4px 10px rgba(0,0,0,.1); }
  h2 { margin: 0 0 12px; }
  .tabs { display: flex; gap: 6px; margin-bottom: 14px; }
  .tabs button { flex: 1; padding: 10px 0; border: none; border-radius: 8px; background: #dde5ec; font-size: 14px; cursor: pointer; }
  .tabs button.on { background: #007bff; color: #fff; }
  .panel { display: none; }
  .grid { display: grid; grid-template-columns: repeat(3, 1fr); gap: 10px; }
  .grid button { padding: 14px 0; font-size: 28px; border: none; border-radius: 10px; background: #f1f5f9; touch-action: manipulation; cursor: pointer; }
  .grid button small { display: block; font-size: 11px; color: #555; margin-top: 2px; }
  input[type=text] { width: 100%; padding: 12px; margin: 8px 0; border: 1px solid #ccc; border-radius: 6px; font-size: 16px; }
  .go { width: 100%; background: #007bff; color: #fff; border: none; padding: 12px; font-size: 16px; border-radius: 8px; cursor: pointer; }
  .pad { display: grid; grid-template-columns: repeat(3, 70px); gap: 8px; justify-content: center; margin: 14px 0; }
  .pad button { height: 70px; font-size: 26px; border: none; border-radius: 12px; background: #007bff; color: #fff; touch-action: manipulation; cursor: pointer; }
  .jump { width: 100%; height: 76px; font-size: 20px; background: #28a745; color: #fff; border: none; border-radius: 12px; touch-action: manipulation; cursor: pointer; }
  p.hint { font-size: 12px; color: #666; }
</style>
</head>
<body>
<div class="card">
  <h2>ESP32 OLED Fun</h2>

  <div class="tabs">
    <button id="t0" class="on" onclick="tab(0)">Text</button>
    <button id="t1" onclick="tab(1)">Emoji</button>
    <button id="t2" onclick="tab(2)">Anim</button>
    <button id="t3" onclick="tab(3)">Games</button>
  </div>

  <div id="p0" class="panel" style="display:block">
    <input type="text" id="msg" placeholder="Type letters/words..." maxlength="60">
    <button class="go" onclick="sendText()">Show on Display</button>
    <p class="hint">Plain ASCII only. Use Emoji tab for icons.</p>
  </div>

  <div id="p1" class="panel">
    <div class="grid">
      <button onclick="s('/emoji?id=0')">&#x1F600;</button>
      <button onclick="s('/emoji?id=1')">&#x1F609;</button>
      <button onclick="s('/emoji?id=2')">&#x1F622;</button>
      <button onclick="s('/emoji?id=3')">&#x1F62E;</button>
      <button onclick="s('/emoji?id=4')">&#x1F60E;</button>
      <button onclick="s('/emoji?id=5')">&#x2764;&#xFE0F;</button>
      <button onclick="s('/emoji?id=6')">&#x1F606;</button>
    </div>
  </div>

  <div id="p2" class="panel">
    <div class="grid">
      <button onclick="s('/anim?id=0')">&#x26BD;<small>Bounce</small></button>
      <button onclick="s('/anim?id=1')">&#x1F440;<small>Eyes</small></button>
      <button onclick="s('/anim?id=2')">&#x1F493;<small>Heartbeat</small></button>
      <button onclick="s('/anim?id=3')">&#x1F680;<small>Warp</small></button>
      <button onclick="s('/anim?id=4')">&#x1F30A;<small>Wave</small></button>
    </div>
  </div>

  <div id="p3" class="panel">
    <div class="grid" style="grid-template-columns:1fr 1fr">
      <button onclick="s('/game?g=snake')">&#x1F40D;<small>Snake</small></button>
      <button onclick="s('/game?g=dino')">&#x1F996;<small>Dino Run</small></button>
    </div>
    <div class="pad">
      <span></span><button onpointerdown="k('U')">&#9650;</button><span></span>
      <button onpointerdown="k('L')">&#9664;</button>
      <button onpointerdown="k('D')">&#9660;</button>
      <button onpointerdown="k('R')">&#9654;</button>
    </div>
    <button class="jump" onpointerdown="k('J')">JUMP / Restart</button>
    <p class="hint">Keyboard: arrow keys / WASD, Space = jump</p>
  </div>
</div>

<script>
  function s(u) { fetch(u).catch(function () {}); }
  function k(c) { s('/key?k=' + c); }
  function tab(i) {
    for (var j = 0; j < 4; j++) {
      document.getElementById('p' + j).style.display = (j == i) ? 'block' : 'none';
      document.getElementById('t' + j).className = (j == i) ? 'on' : '';
    }
  }
  function sendText() {
    var m = document.getElementById('msg').value;
    if (!m) return;
    fetch('/send', {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body: 'message=' + encodeURIComponent(m)
    }).catch(function () {});
  }
  document.addEventListener('keydown', function (e) {
    if (document.activeElement && document.activeElement.tagName === 'INPUT') return;
    var map = { ArrowUp: 'U', ArrowDown: 'D', ArrowLeft: 'L', ArrowRight: 'R',
                w: 'U', s: 'D', a: 'L', d: 'R', ' ': 'J' };
    var c = map[e.key];
    if (c) { e.preventDefault(); k(c); }
  });
</script>
</body>
</html>
)rawliteral";

// =====================================================================
//                            TEXT & DRAWING
// =====================================================================
void showMessage(const String &msg) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextWrap(true);

  int len = msg.length();
  int size = (len <= 3) ? 4 : (len <= 24 ? 2 : 1);
  display.setTextSize(size);

  if (len <= 3) {
    int w = len * 6 * size;
    display.setCursor((SCREEN_WIDTH - w) / 2, (SCREEN_HEIGHT - 8 * size) / 2);
  } else {
    display.setCursor(0, 0);
  }
  display.print(msg);
  display.display();
}

void drawHeart(int cx, int cy, int s, uint16_t color) {
  display.fillCircle(cx - s, cy, s, color);
  display.fillCircle(cx + s, cy, s, color);
  display.fillTriangle(cx - 2 * s + 1, cy + (int)(s * 0.35),
                       cx + 2 * s - 1, cy + (int)(s * 0.35),
                       cx, cy + (int)(s * 2.4), color);
}

void arcThick(int cx, int cy, int r, int a0, int a1, uint16_t color) {
  for (int a = a0; a <= a1; a += 4) {
    float rad = a * 0.0174533f;
    int x = cx + (int)(cos(rad) * r);
    int y = cy + (int)(sin(rad) * r);
    display.fillCircle(x, y, 1, color);
  }
}

void eyesDots(int r) {
  display.fillCircle(54, 24, r, SSD1306_BLACK);
  display.fillCircle(74, 24, r, SSD1306_BLACK);
}

void smileArc() {
  arcThick(64, 34, 16, 25, 155, SSD1306_BLACK);
}

// =====================================================================
//                               EMOJI
// =====================================================================
void drawEmoji(int id) {
  display.clearDisplay();

  if (id == 5) {
    drawHeart(64, 25, 11, SSD1306_WHITE);
    display.display();
    return;
  }

  display.fillCircle(64, 32, 28, SSD1306_WHITE);

  switch (id) {
    case 0:
      eyesDots(3);
      smileArc();
      break;
    case 1:
      display.fillCircle(54, 24, 3, SSD1306_BLACK);
      display.drawFastHLine(69, 24, 10, SSD1306_BLACK);
      display.drawFastHLine(69, 25, 10, SSD1306_BLACK);
      smileArc();
      break;
    case 2:
      eyesDots(3);
      display.drawLine(46, 21, 58, 17, SSD1306_BLACK);
      display.drawLine(82, 21, 70, 17, SSD1306_BLACK);
      arcThick(64, 52, 14, 205, 335, SSD1306_BLACK);
      break;
    case 3:
      eyesDots(4);
      display.fillCircle(64, 45, 6, SSD1306_BLACK);
      break;
    case 4:
      display.fillRect(43, 19, 18, 9, SSD1306_BLACK);
      display.fillRect(67, 19, 18, 9, SSD1306_BLACK);
      display.drawFastHLine(61, 21, 6, SSD1306_BLACK);
      display.drawFastHLine(46, 21, 4, SSD1306_WHITE);
      display.drawFastHLine(70, 21, 4, SSD1306_WHITE);
      smileArc();
      break;
    case 6:
      arcThick(54, 26, 5, 200, 340, SSD1306_BLACK);
      arcThick(74, 26, 5, 200, 340, SSD1306_BLACK);
      display.fillCircle(64, 38, 12, SSD1306_BLACK);
      display.fillRect(50, 26, 29, 12, SSD1306_WHITE);
      break;
    default:
      eyesDots(3);
      smileArc();
      break;
  }
  display.display();
}

// =====================================================================
//                            ANIMATIONS
// =====================================================================
float ballX, ballY, ballVx, ballVy;
float trailX[8], trailY[8];
int trailHead = 0;

int eyeBlink = -1;
unsigned long nextBlinkAt = 0, nextLookAt = 0;
float lookX = 0, lookY = 0, lookTX = 0, lookTY = 0;

#define NSTARS 30
float starA[NSTARS], starR[NSTARS];

void startAnim(int id) {
  mode = M_ANIM;
  animId = id;
  animFrame = 0;

  if (id == 0) {
    ballX = 20; ballY = 20; ballVx = 2.3; ballVy = 1.7;
    for (int i = 0; i < 8; i++) { trailX[i] = ballX; trailY[i] = ballY; }
  } else if (id == 1) {
    eyeBlink = -1;
    nextBlinkAt = millis() + 2000;
    nextLookAt = millis() + 1000;
    lookX = lookY = lookTX = lookTY = 0;
  } else if (id == 3) {
    for (int i = 0; i < NSTARS; i++) {
      starA[i] = random(0, 628) / 100.0f;
      starR[i] = random(1, 80);
    }
  }
}

void animBall() {
  ballX += ballVx;
  ballY += ballVy;
  if (ballX < 5)   { ballX = 5;   ballVx = -ballVx; }
  if (ballX > 122) { ballX = 122; ballVx = -ballVx; }
  if (ballY < 5)   { ballY = 5;   ballVy = -ballVy; }
  if (ballY > 58)  { ballY = 58;  ballVy = -ballVy; }

  trailHead = (trailHead + 1) % 8;
  trailX[trailHead] = ballX;
  trailY[trailHead] = ballY;

  display.clearDisplay();
  display.drawRect(0, 0, 128, 64, SSD1306_WHITE);
  for (int i = 7; i >= 0; i--) {
    int idx = (trailHead - i + 8) % 8;
    int r = 4 - (i / 2);
    display.fillCircle((int)trailX[idx], (int)trailY[idx], max(1, r), SSD1306_WHITE);
  }
  display.display();
}

void animEyes() {
  static const int blinkH[7] = {40, 30, 18, 6, 18, 30, 40};
  unsigned long now = millis();

  if (eyeBlink < 0 && now >= nextBlinkAt) eyeBlink = 0;
  int h = 40;
  if (eyeBlink >= 0) {
    h = blinkH[eyeBlink];
    eyeBlink++;
    if (eyeBlink >= 7) {
      eyeBlink = -1;
      nextBlinkAt = now + random(2000, 4500);
    }
  }

  if (now >= nextLookAt) {
    lookTX = random(-8, 9);
    lookTY = random(-10, 11);
    nextLookAt = now + random(800, 2200);
  }
  lookX += (lookTX - lookX) * 0.35;
  lookY += (lookTY - lookY) * 0.35;

  display.clearDisplay();
  for (int e = 0; e < 2; e++) {
    int cx = (e == 0) ? 36 : 92;
    int rad = min(8, h / 2);
    display.fillRoundRect(cx - 18, 32 - h / 2, 36, h, rad, SSD1306_WHITE);
    if (h > 20) display.fillCircle(cx + (int)lookX, 32 + (int)lookY, 7, SSD1306_BLACK);
  }
  display.display();
}

void animHeart() {
  int p = animFrame % 28;
  float pulse;
  if (p < 4)       pulse = p / 4.0;
  else if (p < 8)  pulse = 1.0 - (p - 4) / 4.0;
  else if (p < 12) pulse = 0.7 * (p - 8) / 4.0;
  else if (p < 16) pulse = 0.7 * (1.0 - (p - 12) / 4.0);
  else             pulse = 0;

  int s = 8 + (int)(pulse * 5);
  display.clearDisplay();
  drawHeart(64, 32 - (int)(0.7 * s), s, SSD1306_WHITE);
  display.display();
}

void animWarp() {
  display.clearDisplay();
  for (int i = 0; i < NSTARS; i++) {
    float r0 = starR[i];
    starR[i] += 0.7 + starR[i] * 0.10;
    float r1 = starR[i];
    float c = cos(starA[i]);
    float sn = sin(starA[i]);
    int x0 = 64 + (int)(c * r0);
    int y0 = 32 + (int)(sn * r0 * 0.6);
    int x1 = 64 + (int)(c * r1);
    int y1 = 32 + (int)(sn * r1 * 0.6);

    if (x1 < 0 || x1 > 127 || y1 < 0 || y1 > 63) {
      starR[i] = random(1, 8);
      starA[i] = random(0, 628) / 100.0f;
      continue;
    }
    if (r1 < 12) display.drawPixel(x1, y1, SSD1306_WHITE);
    else         display.drawLine(x0, y0, x1, y1, SSD1306_WHITE);
  }
  display.display();
}

void animWave() {
  float t = animFrame * 0.25;
  display.clearDisplay();
  int px1 = 0, py1 = 32, px2 = 0, py2 = 32;
  for (int x = 0; x < 128; x++) {
    int y1 = 32 + (int)(18 * sin(x * 0.09 + t));
    int y2 = 32 + (int)(10 * sin(x * 0.15 - t * 1.3));
    if (x > 0) {
      display.drawLine(px1, py1, x, y1, SSD1306_WHITE);
      if (x % 3 != 0) display.drawLine(px2, py2, x, y2, SSD1306_WHITE);
    }
    px1 = x; py1 = y1; px2 = x; py2 = y2;
  }
  display.display();
}

void drawAnim() {
  animFrame++;
  switch (animId) {
    case 0: animBall();  break;
    case 1: animEyes();  break;
    case 2: animHeart(); break;
    case 3: animWarp();  break;
    case 4: animWave();  break;
  }
}

// =====================================================================
//                                GAMES
// =====================================================================
void drawGameOver(int score) {
  display.fillRect(6, 14, 116, 38, SSD1306_BLACK);
  display.drawRect(6, 14, 116, 38, SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(10, 19);
  display.print("GAME OVER");
  display.setTextSize(1);
  display.setCursor(14, 40);
  display.print("Score ");
  display.print(score);
  display.print(" - tap");
}

// ---------------------------- SNAKE ----------------------------
#define SNAKE_W    32
#define SNAKE_H    13
#define SNAKE_Y0   10
#define SNAKE_MAX  (SNAKE_W * SNAKE_H)

uint8_t snakeX[SNAKE_MAX];
uint8_t snakeY[SNAKE_MAX];
int snakeLen, dirX, dirY, nextDirX, nextDirY;
int foodX, foodY, snakeScore, snakeFrameCount, stepFrames;
bool snakeOver;

void placeFood() {
  while (true) {
    foodX = random(0, SNAKE_W);
    foodY = random(0, SNAKE_H);
    bool onSnake = false;
    for (int i = 0; i < snakeLen; i++) {
      if (snakeX[i] == foodX && snakeY[i] == foodY) { onSnake = true; break; }
    }
    if (!onSnake) return;
  }
}

void snakeReset() {
  snakeLen = 3;
  for (int i = 0; i < snakeLen; i++) { snakeX[i] = 10 - i; snakeY[i] = 6; }
  dirX = nextDirX = 1;
  dirY = nextDirY = 0;
  snakeScore = 0;
  snakeOver = false;
  snakeFrameCount = 0;
  stepFrames = 4;
  placeFood();
}

void updateSnake() {
  if (snakeOver) return;
  if (++snakeFrameCount < stepFrames) return;
  snakeFrameCount = 0;

  dirX = nextDirX;
  dirY = nextDirY;
  int nx = snakeX[0] + dirX;
  int ny = snakeY[0] + dirY;

  if (nx < 0 || nx >= SNAKE_W || ny < 0 || ny >= SNAKE_H) { snakeOver = true; return; }

  bool eat = (nx == foodX && ny == foodY);
  int check = eat ? snakeLen : snakeLen - 1;
  for (int i = 0; i < check; i++) {
    if (snakeX[i] == nx && snakeY[i] == ny) { snakeOver = true; return; }
  }

  if (eat && snakeLen < SNAKE_MAX) snakeLen++;
  for (int i = snakeLen - 1; i > 0; i--) {
    snakeX[i] = snakeX[i - 1];
    snakeY[i] = snakeY[i - 1];
  }
  snakeX[0] = nx;
  snakeY[0] = ny;

  if (eat) {
    snakeScore++;
    stepFrames = max(2, 4 - snakeScore / 6);
    if (snakeLen >= SNAKE_MAX) snakeOver = true; else placeFood();
  }
}

void drawSnake() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("SNAKE  Score:");
  display.print(snakeScore);
  display.drawFastHLine(0, 9, 128, SSD1306_WHITE);

  display.drawRect(foodX * 4, SNAKE_Y0 + foodY * 4, 3, 3, SSD1306_WHITE);
  for (int i = 0; i < snakeLen; i++) {
    display.fillRect(snakeX[i] * 4, SNAKE_Y0 + snakeY[i] * 4, 3, 3, SSD1306_WHITE);
  }
  if (snakeOver) drawGameOver(snakeScore);
  display.display();
}

// ---------------------------- DINO RUN ----------------------------
#define GROUND_Y 56
#define DINO_X   14

struct Obstacle { float x; uint8_t w; uint8_t h; bool on; };
Obstacle obs[3];
float dinoY, dinoVy, dinoSpeed, scrollX;
bool dinoOnGround, dinoJump, dinoOver;
int dinoScore, spawnGap;
unsigned long dinoFrames;

void dinoReset() {
  dinoY = GROUND_Y;
  dinoVy = 0;
  dinoOnGround = true;
  dinoJump = false;
  dinoOver = false;
  dinoScore = 0;
  dinoSpeed = 3.0;
  dinoFrames = 0;
  scrollX = 0;
  spawnGap = 70;
  for (int i = 0; i < 3; i++) obs[i].on = false;
}

void updateDino() {
  if (dinoOver) return;
  dinoFrames++;

  if (dinoJump && dinoOnGround) { dinoVy = -5.5; dinoOnGround = false; }
  dinoJump = false;
  dinoY += dinoVy;
  dinoVy += 0.8;
  if (dinoY >= GROUND_Y) { dinoY = GROUND_Y; dinoVy = 0; dinoOnGround = true; }

  dinoScore = dinoFrames / 4;
  dinoSpeed = 3.0 + dinoScore / 100.0;
  if (dinoSpeed > 6.5) dinoSpeed = 6.5;
  scrollX += dinoSpeed;

  float dl = DINO_X + 1, dr = DINO_X + 8, dt = dinoY - 9, db = dinoY;
  for (int i = 0; i < 3; i++) {
    if (!obs[i].on) continue;
    obs[i].x -= dinoSpeed;
    if (obs[i].x + obs[i].w < 0) { obs[i].on = false; continue; }

    float oxL = obs[i].x, oxR = obs[i].x + obs[i].w;
    float oyT = GROUND_Y - obs[i].h, oyB = GROUND_Y;
    if (dr > oxL && dl < oxR && db > oyT && dt < oyB) dinoOver = true;
  }

  bool canSpawn = true;
  for (int i = 0; i < 3; i++) {
    if (obs[i].on && obs[i].x > 128 - spawnGap) canSpawn = false;
  }
  if (canSpawn) {
    for (int i = 0; i < 3; i++) {
      if (!obs[i].on) {
        obs[i].x = 128;
        obs[i].w = (uint8_t)random(4, 8);
        obs[i].h = (uint8_t)random(8, 15);
        obs[i].on = true;
        spawnGap = random(60, 110) + (int)(dinoSpeed * 8);
        break;
      }
    }
  }
}

void drawDino() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("DINO  Score:");
  display.print(dinoScore);

  display.drawFastHLine(0, GROUND_Y, 128, SSD1306_WHITE);
  for (int i = 0; i < 6; i++) {
    int gx = (((i * 27 - (int)scrollX) % 128) + 128) % 128;
    display.drawPixel(gx, GROUND_Y + 3, SSD1306_WHITE);
  }

  int top = (int)dinoY - 10;
  int x = DINO_X;
  display.fillRect(x + 3, top, 6, 5, SSD1306_WHITE);
  display.drawPixel(x + 6, top + 1, SSD1306_BLACK);
  display.fillRect(x, top + 4, 7, 5, SSD1306_WHITE);
  display.drawPixel(x - 1, top + 5, SSD1306_WHITE);
  display.fillRect(x + 1, top + 9, 2, 1, SSD1306_WHITE);
  if (!dinoOnGround || ((dinoFrames / 3) % 2 == 0)) {
    display.fillRect(x + 4, top + 9, 2, 1, SSD1306_WHITE);
  }

  for (int i = 0; i < 3; i++) {
    if (!obs[i].on) continue;
    display.fillRect((int)obs[i].x, GROUND_Y - obs[i].h, obs[i].w, obs[i].h, SSD1306_WHITE);
  }

  if (dinoOver) drawGameOver(dinoScore);
  display.display();
}

void onKey(char k) {
  if (mode == M_SNAKE) {
    if (snakeOver) { snakeReset(); return; }
    if (k == 'U' && dirY == 0) { nextDirX = 0;  nextDirY = -1; }
    if (k == 'D' && dirY == 0) { nextDirX = 0;  nextDirY = 1;  }
    if (k == 'L' && dirX == 0) { nextDirX = -1; nextDirY = 0;  }
    if (k == 'R' && dirX == 0) { nextDirX = 1;  nextDirY = 0;  }
  } else if (mode == M_DINO) {
    if (k == 'J' || k == 'U') {
      if (dinoOver) dinoReset();
      else dinoJump = true;
    }
  }
}

// =====================================================================
//                           WEB HANDLERS
// =====================================================================
void handleRoot() {
  server.send(200, "text/html", HTML_PAGE);
}

void handleSend() {
  String raw = server.arg("message");
  String clean = "";
  for (unsigned int i = 0; i < raw.length(); i++) {
    char c = raw[i];
    if (c >= 32 && c <= 126) clean += c;
  }
  if (clean.length() > 0) {
    mode = M_TEXT;
    Serial.println("Received: " + clean);
    showMessage(clean);
  }
  server.send(200, "text/plain", "ok");
}

void handleEmoji() {
  mode = M_EMOJI;
  drawEmoji(server.arg("id").toInt());
  server.send(200, "text/plain", "ok");
}

void handleAnim() {
  startAnim(server.arg("id").toInt());
  server.send(200, "text/plain", "ok");
}

void handleGame() {
  String g = server.arg("g");
  if (g == "snake") { mode = M_SNAKE; snakeReset(); }
  if (g == "dino")  { mode = M_DINO;  dinoReset();  }
  server.send(200, "text/plain", "ok");
}

void handleKey() {
  String k = server.arg("k");
  if (k.length() > 0) onKey(k.charAt(0));
  server.send(200, "text/plain", "ok");
}

// =====================================================================
//                            SETUP / LOOP
// =====================================================================
void setup() {
  Serial.begin(115200);
  delay(500);

  Wire.begin(21, 22);
  Wire.setClock(400000);
  Wire.setTimeOut(100);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    display.begin(SSD1306_SWITCHCAPVCC, 0x3D);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 10);
  display.println("Connecting WiFi...");
  display.display();

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - t0 < 20000)) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  String ip;
  String line1;
  if (WiFi.status() == WL_CONNECTED) {
    ip = WiFi.localIP().toString();
    line1 = "WiFi Connected!";
  } else {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASS);
    ip = WiFi.softAPIP().toString();
    line1 = String("Join: ") + AP_SSID;
  }
  Serial.println("Open: http://" + ip);

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(line1);
  display.setCursor(0, 18);
  display.println("Open on phone:");
  display.setCursor(0, 36);
  display.println(ip);
  display.display();

  server.on("/", HTTP_GET, handleRoot);
  server.on("/send", HTTP_POST, handleSend);
  server.on("/emoji", HTTP_GET, handleEmoji);
  server.on("/anim", HTTP_GET, handleAnim);
  server.on("/game", HTTP_GET, handleGame);
  server.on("/key", HTTP_GET, handleKey);
  server.begin();
}

void loop() {
  server.handleClient();

  unsigned long now = millis();
  if (now - lastFrame >= FRAME_MS) {
    lastFrame = now;
    switch (mode) {
      case M_ANIM:  drawAnim(); break;
      case M_SNAKE: updateSnake(); drawSnake(); break;
      case M_DINO:  updateDino();  drawDino();  break;
      default: break;
    }
  }
}