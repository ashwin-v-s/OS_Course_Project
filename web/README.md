# Web UI for Live Deadlock Detector

## Setup

### Install Dependencies (in WSL)
```bash
cd ~/live-deadlock-detector/web
pip3 install -r requirements.txt
```

### Start Backend Server
```bash
cd ~/live-deadlock-detector/web
python3 app.py
```

Server will start on: http://localhost:5000

## Usage

1. Open browser: http://localhost:5000
2. Upload a .c file with potential deadlock
3. Click "Compile Program"
4. Click "Run with Wrapper"
5. Click "Analyze Deadlock"
6. See the result!

## API Endpoints

- `POST /upload` - Upload C file
- `POST /compile` - Compile the program
- `POST /run` - Run with LD_PRELOAD wrapper
- `POST /analyze` - Analyze for deadlock
- `GET /health` - Health check

## Architecture

```
Browser (UI)
    Γåô HTTP requests
Flask Backend (app.py)
    Γåô WSL commands
WSL Linux
    Γåô LD_PRELOAD
Program Execution ΓåÆ lock_events.bin
    Γåô
Detector Analysis ΓåÆ Result
```
