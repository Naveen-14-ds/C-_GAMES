## Web Gaming Platform (HTML/CSS + C++/WASM)

This project is a small browser-based gaming platform:

- **Frontend**: `index.html` + `styles.css` (launcher UI) and a tiny JS bridge in `app.js`
- **Backend core logic**: C++ compiled to **WebAssembly** (`wasm/games.wasm`) via **Emscripten**
- **Games included**
  - **Two-player Football** (arcade “soccer”): 2 players, ball physics, goals, score to win
  - **Snake** (classic console-style): grid snake, food, growth, game over

### Prerequisites

- **Emscripten SDK** installed and activated (so `em++` is available)
  - Install guide: `https://emscripten.org/docs/getting_started/downloads.html`

### Build (Windows PowerShell)

From this folder:

```powershell
mkdir wasm -ea 0
em++ .\cpp\games.cpp -O2 -std=c++17 `
  -sWASM=1 -sMODULARIZE=1 -sEXPORT_ES6=1 `
  -sENVIRONMENT=web `
  -sEXPORTED_RUNTIME_METHODS=['cwrap'] `
  -o .\wasm\games.js
```

Outputs:
- `wasm/games.js`
- `wasm/games.wasm`

### Run locally

You need a local server (WASM won’t load from `file://` reliably).

#### Option A: Python

```powershell
python -m http.server 5173
```

Open `http://localhost:5173/`.

#### Option B: Node

```powershell
npx serve -l 5173
```

### Controls

#### Football (two-player)

- **Player 1**: `W` / `S`
- **Player 2**: `↑` / `↓`
- **Kick/boost**: `Space` (adds a small impulse if close to the ball)
- **Restart**: `R`

#### Snake

- **Move**: Arrow keys or `WASD`
- **Restart**: `R`

