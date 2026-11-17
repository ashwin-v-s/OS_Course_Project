# 🤝 WORK DIVISION - Web UI Development

## ⏰ Time: 3-4 hours
## 🎯 Goal: Add web interface to existing working system

---

## 👤 **PERSON 1 (YOU) - Backend API**

### **Task: Build Flask Backend**
**Time:** 2 hours

### What to Build:
```
web/
└── app.py        # Flask API server
```

### API Endpoints:

#### 1. `GET /`
- Serves the main HTML page
- Returns: `templates/index.html`

#### 2. `POST /upload`
- Receives uploaded .c file
- Saves to `web/uploads/`
- Returns: `{success: true, filename: "program.c"}`

#### 3. `POST /compile`
- Compiles the uploaded C file
- Command: `gcc uploads/{filename} -pthread -o uploads/program`
- Returns: `{success: true/false, output: "compilation log"}`

#### 4. `POST /run`
- Runs with LD_PRELOAD wrapper
- Command: `timeout 3 LD_PRELOAD=../liblocktrace.so ./uploads/program`
- Captures output
- Returns: `{success: true, output: "program output", hung: true/false}`

#### 5. `POST /analyze`
- Runs detector on lock_events.bin
- Command: `../build/bin/detector lock_events.bin`
- Returns: `{success: true, output: "detector result", deadlock: true/false}`

### Requirements:
- Flask (Python web framework)
- subprocess (to run shell commands)
- CORS enabled (for API calls)

---

## 👤 **PERSON 2 (YOUR FRIEND) - Frontend UI**

### **Task: Build Web Interface**
**Time:** 2 hours

### What to Build:
```
web/
├── templates/
│   └── index.html       # Main page
└── static/
    ├── style.css        # Styling
    └── script.js        # Interaction logic
```

### UI Components:

#### 1. **File Upload Section**
```
┌────────────────────────────────────┐
│  📁 Select C Program File          │
│  [Choose File] banking_deadlock.c  │
│  [Upload] button                   │
└────────────────────────────────────┘
```

#### 2. **Control Panel**
```
┌────────────────────────────────────┐
│  [Compile Program] button          │
│  [Run with Wrapper] button         │
│  [Analyze Deadlock] button         │
└────────────────────────────────────┘
```

#### 3. **Terminal Output Display**
```
┌────────────────────────────────────┐
│  📟 Program Output:                │
│  ┌──────────────────────────────┐ │
│  │ [locktrace] Initialized...   │ │
│  │ Thread 1 started...          │ │
│  │ Thread 2 started...          │ │
│  │ Program hung (deadlock!)     │ │
│  └──────────────────────────────┘ │
└────────────────────────────────────┘
```

#### 4. **Analysis Result Display**
```
┌────────────────────────────────────┐
│  🔍 Deadlock Analysis:             │
│  ┌──────────────────────────────┐ │
│  │ 🚨 DEADLOCK DETECTED!        │ │
│  │                              │ │
│  │ Cycle: Thread 458 ⇄ 459     │ │
│  │                              │ │
│  │ Thread 458:                  │ │
│  │   Owns: Lock 0x...           │ │
│  │   Wants: Lock 0x...          │ │
│  │                              │ │
│  │ 💡 Recommendation:           │ │
│  │ Use consistent lock ordering │ │
│  └──────────────────────────────┘ │
└────────────────────────────────────┘
```

### Features:
- Clean, modern design
- Color coding (green=success, red=deadlock, yellow=running)
- Loading indicators during operations
- Error handling and display

---

## 🔄 **INTEGRATION WORKFLOW**

### Step 1: Both clone the repo
```bash
git clone <your-github-repo-url>
cd live-deadlock-detector
```

### Step 2: Person 1 creates branch for backend
```bash
git checkout -b backend-api
# Work on web/app.py
git add web/app.py
git commit -m "Add Flask backend API"
git push origin backend-api
```

### Step 3: Person 2 creates branch for frontend
```bash
git checkout -b frontend-ui
# Work on web/templates/ and web/static/
git add web/templates/ web/static/
git commit -m "Add web UI"
git push origin frontend-ui
```

### Step 4: Merge both branches
```bash
git checkout main
git merge backend-api
git merge frontend-ui
git push origin main
```

---

## 🎯 **TESTING TOGETHER**

### Once both are done:

1. **Person 1** starts Flask server:
   ```bash
   cd web
   python3 app.py
   # Server runs on: http://localhost:5000
   ```

2. **Person 2** opens browser:
   ```
   http://localhost:5000
   ```

3. Test the flow:
   - Upload `banking_deadlock.c`
   - Click "Compile"
   - Click "Run"
   - Click "Analyze"
   - See deadlock result!

---

## 📞 **COMMUNICATION POINTS**

### Person 1 tells Person 2:
- ✅ "API endpoints are ready"
- ✅ "Use `/upload` for file upload"
- ✅ "Use `/compile`, `/run`, `/analyze` for operations"
- ✅ "All return JSON with `{success, output}` format"

### Person 2 tells Person 1:
- ✅ "UI is ready"
- ✅ "Upload button sends POST to `/upload`"
- ✅ "Control buttons call respective APIs"
- ✅ "Results display in designated divs"

---

## ⚡ **QUICK START COMMANDS**

### Person 1 (Backend):
```bash
cd ~/live-deadlock-detector
git pull
git checkout -b backend-api
cd web
# Tell Kiro: "Build Flask API according to WORK_DIVISION.md"
```

### Person 2 (Frontend):
```bash
git clone <repo-url>
cd live-deadlock-detector
git checkout -b frontend-ui
cd web
# Tell Kiro: "Build web UI according to WORK_DIVISION.md"
```

---

## 🎉 **SUCCESS CRITERIA**

✅ Upload .c file through web interface
✅ Click button to compile → see output
✅ Click button to run → see it hang
✅ Click button to analyze → see deadlock result
✅ Everything looks clean and professional
✅ No errors, smooth workflow

---

## ⏰ **TIMELINE**

```
Hour 1: Person 1 builds API, Person 2 builds HTML structure
Hour 2: Person 1 tests API, Person 2 adds CSS styling
Hour 3: Person 2 adds JavaScript, both test integration
Hour 4: Polish, fix bugs, practice demo
```

---

## 💡 **TIPS**

### For Person 1:
- Use `subprocess.run()` for shell commands
- Capture stdout and stderr
- Handle timeouts properly
- Test each endpoint individually first

### For Person 2:
- Use `fetch()` API for HTTP requests
- Show loading indicators during operations
- Handle errors gracefully
- Make it look professional (teacher sees this!)

---

**Ready to divide and conquer! 🚀**
