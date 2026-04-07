# 📌 URL Keeper

A lightweight desktop bookmark manager built with **C++**, **ImGui**, **GLFW**, and **SQLite**.

Designed for speed and simplicity, URL Keeper lets you quickly store, view, copy, and delete URLs in a minimal UI.

---

## ✨ Features

- 📋 Add and store URLs with automatic labeling
- 🗂 Persistent storage using SQLite
- 🖱 Select, double-click, and manage entries via ImGui UI
- 📎 Copy URLs to clipboard
- ❌ Delete individual entries or clear all
- ⚡ Fast, minimal, no bloat

---

## 🖼 Preview

(Add a screenshot here)

---

## 🏗 Dependencies

- C++17
- GLFW
- OpenGL
- ImGui
- SQLite3

---

## 📁 Project Structure

.
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

## ⚙️ Build Instructions

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

### macOS

brew install glfw sqlite

make

---

## 🧠 Usage

- Add URL: type and press Enter
- Select: click row
- Copy: right click
- Delete: Delete key or button

---

## 🗄 Database

CREATE TABLE BOOKMARKS (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    label TEXT,
    url TEXT,
    date TEXT
);

---

## 🚀 Future Improvements

- Search/filter
- Edit entries
- Sorting
- Import/export
