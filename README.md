#  C Mini Web Server

A lightweight, multithreaded HTTP server written in pure **C**.
It supports:
- 📂 Serving static files from a `www/` directory 
- 🧵 Multi-client handling using **POSIX threads (pthreads)
- 🧾 Logging of all requests (`logs/access.log`)
- ⏱️ Response time measurement (in milliseconds) per request

---

## Features

- **Multithreading**: Each client connection is handled by a separate thread.
- **Static file serving**: Serves HTML, CSS, and image files from the `www/` directory.
- **Logging**: Logs each request with client IP, file requested, HTTP status, and response time.
- **Response timing**: Measures request processing time in milliseconds.

---

## 📁 Project Structure

mini-web-server/
├── server.c 
├── Makefile 
├── README.md # Project description
├── .gitignore 
├── www/ 
│ ├── index.html
│ ├── style.css
│ └── logo.png
└── logs/ 
 └── server.log
