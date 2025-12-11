# MineSweeper 💣

A terminal-based Minesweeper game. I learned how to play this game by writing one.

## Features

- Classic Minesweeper gameplay in your terminal
- Customizable grid size and bomb percentage
- Safe first click (no bombs around initial position)
- Automatic empty cell expansion
- Keyboard controls for smooth gameplay

## Requirements

- C++11 or later
- Unix-like system (macOS, Linux)
- Terminal with ANSI escape sequence support

## Build

```bash
make
```

Or compile manually:

```bash
g++ -std=c++11 main.cpp -o main
```

## Usage

Show help:

```bash
./MineSweeper
./MineSweeper --help
./MineSweeper -h
```

Start with default settings (10x10, 10% bombs):

```bash
./MineSweeper start
```

Start with custom settings:

```bash
./MineSweeper start <rows> <cols> <bomb_percentage>
```

Example:

```bash
./MineSweeper start 15 20 20
```

## Controls

| Key       | Action           |
| --------- | ---------------- |
| `w/a/s/d` | Move cursor      |
| `space`   | Open cell        |
| `f`       | Flag/unflag cell |
| `q`       | Quit game        |

## Game Rules

- Open all cells without bombs to win
- Flag all bombs correctly to win
- Opening a bomb ends the game
- Numbers show adjacent bomb count
- Empty cells auto-expand

## License

MIT

---

_This README was generated with the assistance of AI._
