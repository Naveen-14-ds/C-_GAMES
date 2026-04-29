import createModule from "./wasm/games.js";

const $ = (id) => document.getElementById(id);

const statusEl = $("status");
const overlayEl = $("overlay");
const dotEl = $("dot");
const fpsEl = $("fps");
const arenaTitleEl = $("arenaTitle");
const arenaSubtitleEl = $("arenaSubtitle");
const scoreboardEl = $("scoreboard");
const canvas = $("canvas");
const ctx = canvas.getContext("2d");

const btnFootball = $("btnFootball");
const btnSnake = $("btnSnake");

const KEY = {
  W: "KeyW",
  S: "KeyS",
  A: "KeyA",
  D: "KeyD",
  UP: "ArrowUp",
  DOWN: "ArrowDown",
  LEFT: "ArrowLeft",
  RIGHT: "ArrowRight",
  SPACE: "Space",
  R: "KeyR",
};

const keysDown = new Set();

window.addEventListener("keydown", (e) => {
  keysDown.add(e.code);
  if (["ArrowUp","ArrowDown","ArrowLeft","ArrowRight","Space"].includes(e.code)) {
    e.preventDefault();
  }
});

window.addEventListener("keyup", (e) => {
  keysDown.delete(e.code);
});

const input = {
  p1: { up: 0, down: 0, left: 0, right: 0, kick: 0, restart: 0 },
  p2: { up: 0, down: 0, left: 0, right: 0, kick: 0, restart: 0 },
};

function sampleInputs(game) {
  input.p1.restart = keysDown.has(KEY.R) ? 1 : 0;
  input.p2.restart = input.p1.restart;

  if (game === "football") {
    input.p1.up = keysDown.has(KEY.W) ? 1 : 0;
    input.p1.down = keysDown.has(KEY.S) ? 1 : 0;
    input.p1.kick = keysDown.has(KEY.SPACE) ? 1 : 0;

    input.p2.up = keysDown.has(KEY.UP) ? 1 : 0;
    input.p2.down = keysDown.has(KEY.DOWN) ? 1 : 0;
    input.p2.kick = keysDown.has(KEY.SPACE) ? 1 : 0;
  }

  if (game === "snake") {
    input.p1.up = keysDown.has(KEY.UP) || keysDown.has(KEY.W) ? 1 : 0;
    input.p1.down = keysDown.has(KEY.DOWN) || keysDown.has(KEY.S) ? 1 : 0;
    input.p1.left = keysDown.has(KEY.LEFT) || keysDown.has(KEY.A) ? 1 : 0;
    input.p1.right = keysDown.has(KEY.RIGHT) || keysDown.has(KEY.D) ? 1 : 0;
  }
}

function clearCanvas() {
  ctx.clearRect(0, 0, canvas.width, canvas.height);
  ctx.fillStyle = "#0b0f15";
  ctx.fillRect(0, 0, canvas.width, canvas.height);
}

function drawFootball(st) {
  clearCanvas();

  ctx.fillStyle = "white";
  ctx.beginPath();
  ctx.arc(st.ball.x, st.ball.y, 10, 0, Math.PI * 2);
  ctx.fill();

  ctx.fillStyle = "blue";
  ctx.beginPath();
  ctx.arc(st.p1.x, st.p1.y, 20, 0, Math.PI * 2);
  ctx.fill();

  ctx.fillStyle = "red";
  ctx.beginPath();
  ctx.arc(st.p2.x, st.p2.y, 20, 0, Math.PI * 2);
  ctx.fill();

  scoreboardEl.textContent = `P1 ${st.score.a} - ${st.score.b} P2`;
}

function drawSnake(st) {
  clearCanvas();

  ctx.fillStyle = "lime";
  st.snake.forEach(seg => {
    ctx.fillRect(seg.x * st.cell, seg.y * st.cell, st.cell, st.cell);
  });

  ctx.fillStyle = "red";
  ctx.fillRect(st.food.x * st.cell, st.food.y * st.cell, st.cell, st.cell);

  scoreboardEl.textContent = `Score ${st.score}`;
}

let Module = null;
let engine_init = null;
let engine_step = null;
let engine_getStateJson = null;

let currentGame = "none";

function setArena(game) {
  currentGame = game;

  if (game === "football") {
    arenaTitleEl.textContent = "Two-Player Football";
    arenaSubtitleEl.textContent = "W/S vs ↑/↓ · Space to kick.";
  } else if (game === "snake") {
    arenaTitleEl.textContent = "Snake (Classic)";
    arenaSubtitleEl.textContent = "Use arrows or WASD.";
  }
}

function launch(gameId) {
  if (!Module) return;

  engine_init(gameId);

  if (gameId === 1) setArena("football");
  if (gameId === 2) setArena("snake");

  overlayEl.hidden = true;
  dotEl.classList.add("live");
}

btnFootball.onclick = () => launch(1);
btnSnake.onclick = () => launch(2);

let last = performance.now();

function loop(now) {
  const dt = (now - last) / 1000;
  last = now;

  if (Module && currentGame !== "none") {
    sampleInputs(currentGame);

    engine_step(
      dt,
      input.p1.up, input.p1.down, input.p1.left, input.p1.right, input.p1.kick, input.p1.restart,
      input.p2.up, input.p2.down, input.p2.left, input.p2.right, input.p2.kick, input.p2.restart
    );

    const state = JSON.parse(engine_getStateJson());

    if (state.game === "football") drawFootball(state);
    if (state.game === "snake") drawSnake(state);
  }

  requestAnimationFrame(loop);
}

statusEl.textContent = "Loading C++ engine…";

createModule({
  locateFile: (path) => `./wasm/${path}`,
}).then((mod) => {
  Module = mod;

  engine_init = Module.cwrap("engine_init", null, ["number"]);
  engine_step = Module.cwrap("engine_step", null, ["number","number","number","number","number","number","number","number","number","number","number","number","number"]);
  engine_getStateJson = Module.cwrap("engine_getStateJson", "string", []);

  statusEl.textContent = "Engine ready. Choose a game.";
  requestAnimationFrame(loop);
});