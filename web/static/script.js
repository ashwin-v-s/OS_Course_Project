// Live Deadlock Detector - Web UI JavaScript

const API_BASE = 'http://localhost:5000';
let uploadedFileName = '';

// Upload file
async function uploadFile() {
    const fileInput = document.getElementById('fileInput');
    const uploadStatus = document.getElementById('uploadStatus');
    
    if (!fileInput.files || fileInput.files.length === 0) {
        uploadStatus.innerHTML = '<span class="error-text">❌ Please select a file first</span>';
        return;
    }
    
    const file = fileInput.files[0];
    
    if (!file.name.endsWith('.c')) {
        uploadStatus.innerHTML = '<span class="error-text">❌ Only .c files are allowed</span>';
        return;
    }
    
    const formData = new FormData();
    formData.append('file', file);
    
    uploadStatus.innerHTML = '<span class="loading">⏳ Uploading...</span>';
    
    try {
        const response = await fetch(`${API_BASE}/upload`, {
            method: 'POST',
            body: formData
        });
        
        const data = await response.json();
        
        if (data.success) {
            uploadedFileName = data.filename;
            uploadStatus.innerHTML = `<span class="success-text">✅ Uploaded: ${file.name}</span>`;
            document.getElementById('compileBtn').disabled = false;
            document.getElementById('terminalOutput').textContent = `File uploaded successfully: ${file.name}\nReady to compile...`;
        } else {
            uploadStatus.innerHTML = `<span class="error-text">❌ Error: ${data.error}</span>`;
        }
    } catch (error) {
        uploadStatus.innerHTML = `<span class="error-text">❌ Upload failed: ${error.message}</span>`;
    }
}

// Compile program
async function compileProgram() {
    const terminalOutput = document.getElementById('terminalOutput');
    const compileBtn = document.getElementById('compileBtn');
    
    compileBtn.disabled = true;
    terminalOutput.textContent = '⏳ Compiling program...\n';
    terminalOutput.classList.add('loading');
    
    try {
        const response = await fetch(`${API_BASE}/compile`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            }
        });
        
        const data = await response.json();
        terminalOutput.classList.remove('loading');
        
        if (data.success) {
            terminalOutput.textContent = '✅ COMPILATION SUCCESSFUL\n\n' + data.output + '\n\nReady to run program...';
            document.getElementById('runBtn').disabled = false;
        } else {
            terminalOutput.textContent = '❌ COMPILATION FAILED\n\n' + data.output;
            compileBtn.disabled = false;
        }
    } catch (error) {
        terminalOutput.classList.remove('loading');
        terminalOutput.textContent = `❌ Compilation request failed: ${error.message}`;
        compileBtn.disabled = false;
    }
}

// Run program
async function runProgram() {
    const terminalOutput = document.getElementById('terminalOutput');
    const runBtn = document.getElementById('runBtn');
    
    runBtn.disabled = true;
    terminalOutput.textContent = '⏳ Running program with LD_PRELOAD wrapper...\n\n';
    terminalOutput.classList.add('loading');
    
    try {
        const response = await fetch(`${API_BASE}/run`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            }
        });
        
        const data = await response.json();
        terminalOutput.classList.remove('loading');
        
        if (data.success) {
            let output = '🚀 PROGRAM EXECUTION\n\n' + data.output;
            
            if (data.hung) {
                output += '\n\n⚠️  PROGRAM HUNG (Deadlock likely occurred!)';
                output += '\n    The program was terminated after timeout.';
                output += '\n    Lock events have been captured.';
            }
            
            output += '\n\n✅ Ready to analyze for deadlock...';
            
            terminalOutput.textContent = output;
            document.getElementById('analyzeBtn').disabled = false;
        } else {
            terminalOutput.textContent = '❌ EXECUTION FAILED\n\n' + data.output;
            runBtn.disabled = false;
        }
    } catch (error) {
        terminalOutput.classList.remove('loading');
        terminalOutput.textContent = `❌ Execution request failed: ${error.message}`;
        runBtn.disabled = false;
    }
}

// Analyze deadlock
async function analyzeDeadlock() {
    const analysisResult = document.getElementById('analysisResult');
    const analyzeBtn = document.getElementById('analyzeBtn');
    
    analyzeBtn.disabled = true;
    analysisResult.textContent = '⏳ Analyzing lock events for deadlock...\n\nRunning DFS cycle detection...\n';
    analysisResult.classList.add('loading');
    
    try {
        const response = await fetch(`${API_BASE}/analyze`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            }
        });
        
        const data = await response.json();
        analysisResult.classList.remove('loading');
        
        if (data.success) {
            analysisResult.innerHTML = data.output;
            
            // Highlight deadlock status
            if (data.deadlock) {
                analysisResult.innerHTML = '<span class="deadlock-detected">🚨 DEADLOCK DETECTED!</span>\n\n' + data.output;
            } else {
                analysisResult.innerHTML = '<span class="no-deadlock">✅ NO DEADLOCK DETECTED</span>\n\n' + data.output;
            }
        } else {
            analysisResult.textContent = '❌ ANALYSIS FAILED\n\n' + (data.error || 'Unknown error');
        }
        
        analyzeBtn.disabled = false;
    } catch (error) {
        analysisResult.classList.remove('loading');
        analysisResult.textContent = `❌ Analysis request failed: ${error.message}`;
        analyzeBtn.disabled = false;
    }
}

// Health check on load
window.addEventListener('load', async () => {
    try {
        const response = await fetch(`${API_BASE}/health`);
        const data = await response.json();
        if (data.status === 'ok') {
            console.log('✅ Backend is running');
        }
    } catch (error) {
        console.error('❌ Backend not reachable:', error);
        alert('⚠️  Backend server not running! Please start: python3 app.py');
    }
});
