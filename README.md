# Cube Othello (8x8x3) — Terminal Fallback Mode

A minimal C++17 implementation of the 3D cube othello game, rendered in a terminal with ASCII characters. This project demonstrates a flat-plane display approach and simple minimax AI for accessibility on low-resource environments.

**Q9 / A**: The board is displayed as a **flat 2D grid** using an oblique projection — no z-index layering or depth-based occlusion is rendered. Each cell at `(x, y)` is simply colored black (`B`) or white (`W`), ignoring the third dimension `z`.

**Q10 / A**: The AI uses a **very simple minimax** with evaluation function = (my stones − opponent's stones), search depth limited to 2 plies. No lookahead beyond stone-count comparison is performed.

---

## 📁 Project Structure

```
project-cubo_othello/
├── CMakeLists.txt          # CMake build configuration
├── README.md               # This file
├── requirements.txt        # Test dependencies (pytest, numpy)
└── src/
    ├── board.hpp           # CubeBoard + SimpleAI class definitions
    └── game_engine.hpp     # GameEngine controller class
```

## 🛠 Build Prerequisites

- **MinGW-w64** with g++ 10.2 or later (x86_64-posix-seh)
- CMake ≥ 3.15

Install MinGW-w64 on Windows:

```bash
winget install --exact-id BrechtSanders.WinLibs.POSIX.MSVCRT
# or use winget install msys2 then pacman -S mingw-w64-x86_64-gcc
```

## 🏗 Build & Run

```bash
cd build
cmake .. -DCMAKE_CXX_COMPILER=g++
cmake --build . --config Release -j4
```

Run the executable:

```bash
./cubo_othello.exe          # Human vs Human (random moves)
./cubo_othello.exe --ai     # AI mode (simple minimax, depth=2)
```

## 📜 License

MIT
