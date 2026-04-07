# bookmark-keeper

A lightweight desktop bookmark manager built with **C++**, **ImGui**, **GLFW**, and **SQLite**.

Designed for speed and simplicity and lets you quickly store, view, copy, and delete URLs in a minimal UI.

---

## Features

- ⚡ Fast, minimal, no bloat
- 📋 Add and delete URLs with automatic labeling
- 🗂 Persistent storage using SQLite
- 🖱 Select, double-click, and manage entries via ImGui UI
- 📎 Copy URLs to clipboard

---

## Preview

>

---

## Dependencies

- C++17
- GLFW
- OpenGL
- ImGui
- SQLite3

---

## Project Structure

,
├── src/
│   └── main.cpp
├── include/
│   └── third_party/
│       ├── imgui/
│       └── sqlite3/
├── build/
├── Makefile
└── README.md

---

## Build Instructions

### Linux

sudo apt install libglfw3-dev libsqlite3-dev libgl1-mesa-dev

make
./main

---

### Windows (MinGW)

Requirements:
- MinGW-w64
- GLFW
- SQLite3

make
main.exe

---

## Usage

- Select: Click row
- Copy: Hover over a row and right click
- Add URL: Type in text box and press Enter or 'Add' button
- Delete URL: Select a row and press the Delete keyboard key or 'Delete' button

---

## 🗄 Database

CREATE TABLE BOOKMARKS (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    label TEXT,
    url TEXT,
    date TEXT
);
