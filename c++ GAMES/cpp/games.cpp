#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>

#include <emscripten/emscripten.h>

namespace {

constexpr int kCanvasW = 900;
constexpr int kCanvasH = 540;

struct Vec2 {
  float x = 0.0f;
  float y = 0.0f;
};

static inline float clampf(float v, float lo, float hi) {
  return std::max(lo, std::min(hi, v));
}

static inline float len(const Vec2& v) { return std::sqrt(v.x * v.x + v.y * v.y); }

static inline Vec2 add(const Vec2& a, const Vec2& b) { return {a.x + b.x, a.y + b.y}; }
static inline Vec2 sub(const Vec2& a, const Vec2& b) { return {a.x - b.x, a.y - b.y}; }
static inline Vec2 mul(const Vec2& a, float s) { return {a.x * s, a.y * s}; }

static inline Vec2 norm(const Vec2& v) {
  float l = len(v);
  if (l < 1e-6f) return {0, 0};
  return {v.x / l, v.y / l};
}

static inline float dot(const Vec2& a, const Vec2& b) { return a.x * b.x + a.y * b.y; }

enum class GameId : int {
  None = 0,
  Football = 1,
  Snake = 2,
};

struct InputState {
  bool up = false;
  bool down = false;
  bool left = false;
  bool right = false;
  bool kick = false;
  bool restart = false;
};

// ---------------- Football ----------------
struct FootballState {
  // Field bounds in pixels
  float w = (float)kCanvasW;
  float h = (float)kCanvasH;

  Vec2 p1 = {80.0f, (float)kCanvasH / 2.0f};
  Vec2 p2 = {(float)kCanvasW - 80.0f, (float)kCanvasH / 2.0f};
  Vec2 v1 = {0, 0};
  Vec2 v2 = {0, 0};

  Vec2 ball = {(float)kCanvasW / 2.0f, (float)kCanvasH / 2.0f};
  Vec2 ballV = {220.0f, -80.0f};

  int s1 = 0;
  int s2 = 0;
  int winScore = 5;

  bool gameOver = false;
  int winner = 0; // 1 or 2
};

static void footballReset(FootballState& st) {
  st.p1 = {80.0f, (float)kCanvasH / 2.0f};
  st.p2 = {(float)kCanvasW - 80.0f, (float)kCanvasH / 2.0f};
  st.v1 = {0, 0};
  st.v2 = {0, 0};
  st.ball = {(float)kCanvasW / 2.0f, (float)kCanvasH / 2.0f};
  st.ballV = {220.0f, -80.0f};
  st.gameOver = false;
  st.winner = 0;
}

static void footballFullReset(FootballState& st) {
  st.s1 = 0;
  st.s2 = 0;
  footballReset(st);
}

static void footballStep(FootballState& st, float dt, const InputState& p1, const InputState& p2) {
  if (p1.restart || p2.restart) {
    footballFullReset(st);
    return;
  }

  if (st.gameOver) {
    // allow restart only
    return;
  }

  dt = clampf(dt, 0.0f, 0.05f);

  const float paddleR = 22.0f;
  const float paddleSpeed = 360.0f;
  const float friction = 0.90f;

  float a1 = 0.0f;
  if (p1.up) a1 -= 1.0f;
  if (p1.down) a1 += 1.0f;

  float a2 = 0.0f;
  if (p2.up) a2 -= 1.0f;
  if (p2.down) a2 += 1.0f;

  st.v1.y = a1 * paddleSpeed;
  st.v2.y = a2 * paddleSpeed;

  st.p1.y += st.v1.y * dt;
  st.p2.y += st.v2.y * dt;

  st.p1.y = clampf(st.p1.y, paddleR + 18.0f, st.h - paddleR - 18.0f);
  st.p2.y = clampf(st.p2.y, paddleR + 18.0f, st.h - paddleR - 18.0f);

  // Ball physics
  const float ballR = 14.0f;
  st.ball = add(st.ball, mul(st.ballV, dt));
  st.ballV = mul(st.ballV, std::pow(friction, dt * 60.0f));

  // walls (top/bottom)
  if (st.ball.y < ballR + 14.0f) {
    st.ball.y = ballR + 14.0f;
    st.ballV.y *= -1.0f;
  } else if (st.ball.y > st.h - ballR - 14.0f) {
    st.ball.y = st.h - ballR - 14.0f;
    st.ballV.y *= -1.0f;
  }

  // goals
  const float goalTop = st.h * 0.28f;
  const float goalBot = st.h * 0.72f;
  bool inGoalY = (st.ball.y > goalTop && st.ball.y < goalBot);
  if (inGoalY && st.ball.x < -ballR) {
    st.s2++;
    footballReset(st);
  } else if (inGoalY && st.ball.x > st.w + ballR) {
    st.s1++;
    footballReset(st);
  }

  if (st.s1 >= st.winScore) {
    st.gameOver = true;
    st.winner = 1;
  } else if (st.s2 >= st.winScore) {
    st.gameOver = true;
    st.winner = 2;
  }

  // side bumpers (when not inside goal opening)
  if (!inGoalY) {
    if (st.ball.x < ballR + 14.0f) {
      st.ball.x = ballR + 14.0f;
      st.ballV.x *= -1.0f;
    } else if (st.ball.x > st.w - ballR - 14.0f) {
      st.ball.x = st.w - ballR - 14.0f;
      st.ballV.x *= -1.0f;
    }
  }

  auto collidePaddle = [&](const Vec2& pPos, bool kick) {
    Vec2 d = sub(st.ball, pPos);
    float dist = len(d);
    float minDist = ballR + paddleR;
    if (dist < minDist && dist > 1e-5f) {
      Vec2 n = mul(d, 1.0f / dist);
      float penetration = minDist - dist;
      st.ball = add(st.ball, mul(n, penetration + 0.2f));

      float vn = dot(st.ballV, n);
      if (vn < 0.0f) {
        st.ballV = sub(st.ballV, mul(n, 1.85f * vn));
      }

      // add some "english"
      st.ballV.y += n.y * 40.0f;

      if (kick) {
        st.ballV = add(st.ballV, mul(n, 220.0f));
      }
    }
  };

  collidePaddle(st.p1, p1.kick);
  collidePaddle(st.p2, p2.kick);

  // ensure minimum motion so it doesn't die
  float sp = len(st.ballV);
  if (sp < 80.0f) {
    st.ballV = mul(norm(st.ballV.x == 0 && st.ballV.y == 0 ? Vec2{1, 0} : st.ballV), 80.0f);
  }
}

// ---------------- Snake ----------------
struct SnakeState {
  int gridW = 30;
  int gridH = 18;
  float cell = 28.0f;
  float offsetX = 30.0f;
  float offsetY = 18.0f;

  std::vector<int> xs;
  std::vector<int> ys;
  int dirX = 1;
  int dirY = 0;

  int foodX = 12;
  int foodY = 8;

  float stepAccum = 0.0f;
  float stepSec = 0.105f;

  bool gameOver = false;
  int score = 0;
};

static uint32_t rngState = 0xC0FFEEu;
static uint32_t xorshift32() {
  uint32_t x = rngState;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  rngState = x;
  return x;
}

static void snakePlaceFood(SnakeState& st) {
  for (int tries = 0; tries < 200; tries++) {
    int fx = (int)(xorshift32() % (uint32_t)st.gridW);
    int fy = (int)(xorshift32() % (uint32_t)st.gridH);
    bool onSnake = false;
    for (size_t i = 0; i < st.xs.size(); i++) {
      if (st.xs[i] == fx && st.ys[i] == fy) { onSnake = true; break; }
    }
    if (!onSnake) { st.foodX = fx; st.foodY = fy; return; }
  }
  st.foodX = st.gridW / 2;
  st.foodY = st.gridH / 2;
}

static void snakeReset(SnakeState& st) {
  st.xs.clear();
  st.ys.clear();
  int cx = st.gridW / 2;
  int cy = st.gridH / 2;
  for (int i = 0; i < 5; i++) {
    st.xs.push_back(cx - i);
    st.ys.push_back(cy);
  }
  st.dirX = 1;
  st.dirY = 0;
  st.stepAccum = 0.0f;
  st.gameOver = false;
  st.score = 0;
  snakePlaceFood(st);
}

static void snakeApplyInput(SnakeState& st, const InputState& in) {
  int ndx = st.dirX;
  int ndy = st.dirY;
  if (in.up) { ndx = 0; ndy = -1; }
  else if (in.down) { ndx = 0; ndy = 1; }
  else if (in.left) { ndx = -1; ndy = 0; }
  else if (in.right) { ndx = 1; ndy = 0; }

  // prevent reversal
  if (!(ndx == -st.dirX && ndy == -st.dirY)) {
    st.dirX = ndx; st.dirY = ndy;
  }
}

static void snakeStepOnce(SnakeState& st) {
  int headX = st.xs[0];
  int headY = st.ys[0];
  int nx = headX + st.dirX;
  int ny = headY + st.dirY;

  if (nx < 0 || ny < 0 || nx >= st.gridW || ny >= st.gridH) {
    st.gameOver = true;
    return;
  }

  for (size_t i = 0; i < st.xs.size(); i++) {
    if (st.xs[i] == nx && st.ys[i] == ny) {
      st.gameOver = true;
      return;
    }
  }

  bool ate = (nx == st.foodX && ny == st.foodY);

  st.xs.insert(st.xs.begin(), nx);
  st.ys.insert(st.ys.begin(), ny);
  if (!ate) {
    st.xs.pop_back();
    st.ys.pop_back();
  } else {
    st.score++;
    snakePlaceFood(st);
    st.stepSec = std::max(0.060f, st.stepSec * 0.985f);
  }
}

static void snakeStep(SnakeState& st, float dt, const InputState& in) {
  if (in.restart) { snakeReset(st); return; }
  if (st.gameOver) return;
  dt = clampf(dt, 0.0f, 0.05f);
  snakeApplyInput(st, in);

  st.stepAccum += dt;
  while (st.stepAccum >= st.stepSec) {
    st.stepAccum -= st.stepSec;
    snakeStepOnce(st);
    if (st.gameOver) break;
  }
}

// ---------------- Engine facade ----------------
static GameId gCurrent = GameId::None;
static FootballState gFootball;
static SnakeState gSnake;

static std::string gStateJson;

static std::string jsonEscape(const std::string& s) {
  std::string o;
  o.reserve(s.size() + 8);
  for (char c : s) {
    switch (c) {
      case '\\': o += "\\\\"; break;
      case '"': o += "\\\""; break;
      case '\n': o += "\\n"; break;
      case '\r': o += "\\r"; break;
      case '\t': o += "\\t"; break;
      default: o += c; break;
    }
  }
  return o;
}

static void setStateFootball() {
  const auto& st = gFootball;
  float goalTop = st.h * 0.28f;
  float goalBot = st.h * 0.72f;
  gStateJson =
    std::string("{\"game\":\"football\",\"w\":") + std::to_string((int)st.w) +
    ",\"h\":" + std::to_string((int)st.h) +
    ",\"p1\":{\"x\":" + std::to_string(st.p1.x) + ",\"y\":" + std::to_string(st.p1.y) + "}" +
    ",\"p2\":{\"x\":" + std::to_string(st.p2.x) + ",\"y\":" + std::to_string(st.p2.y) + "}" +
    ",\"ball\":{\"x\":" + std::to_string(st.ball.x) + ",\"y\":" + std::to_string(st.ball.y) + "}" +
    ",\"score\":{\"a\":" + std::to_string(st.s1) + ",\"b\":" + std::to_string(st.s2) + "}" +
    ",\"goal\":{\"top\":" + std::to_string(goalTop) + ",\"bot\":" + std::to_string(goalBot) + "}" +
    ",\"over\":" + (st.gameOver ? "true" : "false") +
    ",\"winner\":" + std::to_string(st.winner) +
    "}";
}

static void setStateSnake() {
  const auto& st = gSnake;
  std::string body = "[";
  for (size_t i = 0; i < st.xs.size(); i++) {
    body += "{\"x\":" + std::to_string(st.xs[i]) + ",\"y\":" + std::to_string(st.ys[i]) + "}";
    if (i + 1 < st.xs.size()) body += ",";
  }
  body += "]";

  gStateJson =
    std::string("{\"game\":\"snake\",\"gridW\":") + std::to_string(st.gridW) +
    ",\"gridH\":" + std::to_string(st.gridH) +
    ",\"cell\":" + std::to_string(st.cell) +
    ",\"offX\":" + std::to_string(st.offsetX) +
    ",\"offY\":" + std::to_string(st.offsetY) +
    ",\"snake\":" + body +
    ",\"food\":{\"x\":" + std::to_string(st.foodX) + ",\"y\":" + std::to_string(st.foodY) + "}" +
    ",\"score\":" + std::to_string(st.score) +
    ",\"over\":" + (st.gameOver ? "true" : "false") +
    "}";
}

} // namespace

extern "C" {

EMSCRIPTEN_KEEPALIVE
int engine_getCanvasW() { return kCanvasW; }

EMSCRIPTEN_KEEPALIVE
int engine_getCanvasH() { return kCanvasH; }

EMSCRIPTEN_KEEPALIVE
void engine_init(int gameId) {
  if (gameId == 1) {
    gCurrent = GameId::Football;
    footballFullReset(gFootball);
    setStateFootball();
  } else if (gameId == 2) {
    gCurrent = GameId::Snake;
    snakeReset(gSnake);
    setStateSnake();
  } else {
    gCurrent = GameId::None;
    gStateJson = "{\"game\":\"none\"}";
  }
}

EMSCRIPTEN_KEEPALIVE
void engine_step(float dt,
                 int p1_up, int p1_down, int p1_left, int p1_right, int p1_kick, int p1_restart,
                 int p2_up, int p2_down, int p2_left, int p2_right, int p2_kick, int p2_restart) {
  InputState i1;
  i1.up = p1_up != 0;
  i1.down = p1_down != 0;
  i1.left = p1_left != 0;
  i1.right = p1_right != 0;
  i1.kick = p1_kick != 0;
  i1.restart = p1_restart != 0;

  InputState i2;
  i2.up = p2_up != 0;
  i2.down = p2_down != 0;
  i2.left = p2_left != 0;
  i2.right = p2_right != 0;
  i2.kick = p2_kick != 0;
  i2.restart = p2_restart != 0;

  if (gCurrent == GameId::Football) {
    footballStep(gFootball, dt, i1, i2);
    setStateFootball();
  } else if (gCurrent == GameId::Snake) {
    // Use p1 inputs only for snake
    snakeStep(gSnake, dt, i1);
    setStateSnake();
  } else {
    gStateJson = "{\"game\":\"none\"}";
  }
}

EMSCRIPTEN_KEEPALIVE
const char* engine_getStateJson() {
  return gStateJson.c_str();
}

} // extern "C"

