# 🖼️ Art Gallery Guard Simulator

This project is an interactive simulation of the **Art Gallery Problem**, a classic problem in computational geometry. The goal is to determine how many guards are needed so that every point in a polygonal space (the “museum”) is visible to at least one guard.

## ✨ Features

- **Draw a polygonal museum:**
  - Press **`s`** to start drawing.
  - Click to place vertices.
  - Press **`e`** to finish.

- **Place guards:**
  - Press **`g`** to switch into guard placement mode.
  - Click inside the polygon to place guards (they wouldn't be much use outside).
  - Guards are displayed as dots.

- **Animate guard movement:**
  - Press **`a`** to animate all placed guards.
  - Guards move randomly but remain within the confines of the museum.
  - Each guard’s field of view is shown as a transparent polygon that updates as they move.

## 🎯 Purpose

This simulator provides a hands-on way to explore the **Art Gallery Problem**:
- Visualize how guards can cover polygonal spaces.
- Experiment with the minimum number of guards required.
- See dynamic guard coverage in action.

## 🛠️ Usage

1. Clone this repository:
   ```bash
   git clone https://github.com/JackRobs25/GuardProblem.git
   cd GuardProblem
   ```

2. Compile and run the program (requires g++ and OpenGL/GLUT):
   ```bash
   g++ guard.cpp -o guard -lGL -lGLU -lglut
   ./guard
   ```

## 📚 Background

The Art Gallery Problem was first posed by Victor Klee in 1973 and proved by Václav Chvátal:
> “⌊n/3⌋ guards are always sufficient and sometimes necessary to cover a polygon with n vertices.”

This project offers an intuitive way to explore the problem visually.

## 🎮 Controls

| Key | Action                          |
|-----|---------------------------------|
| `s` | Start drawing the polygon       |
| `e` | End drawing the polygon         |
| Mouse click | Place a polygon vertex / guard |
| `g` | Enter guard placement mode      |
| `a` | Animate guards                  |


## 👤 Author

Jack Roberts

