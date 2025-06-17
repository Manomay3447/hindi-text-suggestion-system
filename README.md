# 📝 Hindi Text Suggestion System

The **Hindi Text Suggestion System** is a high-performance hybrid architecture that provides **real-time word and phrase suggestions in Hindi** based on user input.
It combines a lightweight **Flask frontend**, a **C backend** for trie-based dictionary and n-gram search, and is deployable via **Apache with WSGI**.

This system is designed for speed and scalability, making it ideal for use in intelligent editors, search input boxes, and language learning tools.

---

## 🌟 Features

### ✅ Trie-Based Dictionary & N-gram Search
- Uses **compact trie structures** for fast dictionary and n-gram lookup.
- Supports **unigram**, **bigram**, **trigram**, **fourgram**, and **fivegram** models.
- Words are prioritized based on frequency and grammatical context.

### ✅ Fuzzy Matching
- Implements **edit-distance-based search** to correct spelling mistakes.
- Used when exact dictionary match fails.

### ✅ Contextual Suggestions
- N-gram tries provide **context-aware suggestions** based on up to the **last 4 Hindi words** typed.
- Prioritizes more probable next words based on real Hindi language usage.

### ✅ Real-Time Frontend
- Interactive Hindi typing interface using a `<textarea>`.
- The **suggestion box appears directly below the last typed word**, even across lines, without shifting layout.
- Suggestions update **dynamically** via backend API.

### ✅ Flask + Apache + WSGI Integration
- Lightweight **Flask server** receives frontend input and communicates with C backend via FIFO.
- Deployed using **Apache2 + mod_wsgi**, ensuring production-readiness and stability.

---

## 🔧 Technologies Used

| Layer        | Technology                |
|--------------|---------------------------|
| Frontend     | HTML, JavaScript, CSS     |
| Backend      | C (Trie, N-gram logic)    |
| Middleware   | Python Flask              |
| Server       | Apache2 + mod_wsgi        |
| Communication| Python <-> C via FIFO     |
| OS           | Linux (Ubuntu/Debian)     |

---

## 🚀 How It Works

1. User types Hindi text in the frontend.
2. On every keystroke, the **last few words and context** are sent to `/suggest` (Flask API).
3. The Flask backend:
   - Sends data to the C server using **named pipes (FIFO)**.
   - C server performs a **dictionary + n-gram search**.
   - Returns best-matched Hindi suggestions.
4. Suggestions are shown below the cursor in real-time.

---

## 📦 Setup Instructions

### 1) Clone the Repository
git clone git@github.com:yourusername/hindi-text-suggestion-system.git
cd hindi-text-suggestion-system

### 2) Start the C Server (in background or service)
./main Dictionary/ Input/

### 3) Deploy with Apache + WSGI
Make sure /etc/apache2/sites-enabled/hindi_suggestions.conf points to:
WSGIScriptAlias /suggest /var/www/hindi_suggestions/app.wsgi

### 4) Start Apache
sudo systemctl start apache2

### 5) Open WebBrowser
Enter http://localhost/
Try out Typing in Hindi and have Fun!

## ✍️ Future Improvements
User-specific frequency learning (adaptive suggestion ranking).
Unicode normalization for broader Devanagari coverage.
React-based frontend for better UX and mobile compatibility.

## 📄 License
This project is open-source and available under the MIT License.
