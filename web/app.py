#!/usr/bin/env python3
"""
Flask Backend API for Live Deadlock Detector
Handles file upload, compilation, execution, and analysis
"""

from flask import Flask, request, jsonify, render_template
from flask_cors import CORS
import subprocess
import os
import time
from pathlib import Path

app = Flask(__name__)
CORS(app)  # Enable CORS for all routes

# Configuration
UPLOAD_FOLDER = Path(__file__).parent / 'uploads'
UPLOAD_FOLDER.mkdir(exist_ok=True)
WSL_PROJECT_PATH = os.path.expanduser('~/live-deadlock-detector')

app.config['UPLOAD_FOLDER'] = UPLOAD_FOLDER
app.config['MAX_CONTENT_LENGTH'] = 16 * 1024 * 1024  # 16MB max file size

def run_wsl_command(command, timeout=30):
    """Execute command in WSL and return output"""
    try:
        result = subprocess.run(
            ['wsl', '--', 'bash', '-c', command],
            capture_output=True,
            text=True,
            timeout=timeout
        )
        return {
            'success': result.returncode == 0,
            'output': result.stdout,
            'error': result.stderr,
            'returncode': result.returncode
        }
    except subprocess.TimeoutExpired:
        return {
            'success': False,
            'output': '',
            'error': 'Command timed out',
            'returncode': -1,
            'timeout': True
        }
    except Exception as e:
        return {
            'success': False,
            'output': '',
            'error': str(e),
            'returncode': -1
        }

@app.route('/')
def index():
    """Serve the main HTML page"""
    return render_template('index.html')

@app.route('/upload', methods=['POST'])
def upload_file():
    """Handle C file upload"""
    try:
        if 'file' not in request.files:
            return jsonify({'success': False, 'error': 'No file provided'}), 400
        
        file = request.files['file']
        
        if file.filename == '':
            return jsonify({'success': False, 'error': 'No file selected'}), 400
        
        if not file.filename.endswith('.c'):
            return jsonify({'success': False, 'error': 'Only .c files allowed'}), 400
        
        # Save file
        filename = 'uploaded_program.c'
        filepath = UPLOAD_FOLDER / filename
        file.save(filepath)
        
        # Copy to WSL
        wsl_upload_path = f"{WSL_PROJECT_PATH}/web/uploads/{filename}"
        copy_cmd = f"cp '/mnt/d/Operating System/CP/live-deadlock-detector/web/uploads/{filename}' {wsl_upload_path}"
        result = run_wsl_command(copy_cmd)
        
        if not result['success']:
            return jsonify({
                'success': False,
                'error': 'Failed to copy file to WSL',
                'details': result['error']
            }), 500
        
        return jsonify({
            'success': True,
            'filename': filename,
            'message': 'File uploaded successfully'
        })
    
    except Exception as e:
        return jsonify({'success': False, 'error': str(e)}), 500

@app.route('/compile', methods=['POST'])
def compile_program():
    """Compile the uploaded C program"""
    try:
        compile_cmd = f"""
cd {WSL_PROJECT_PATH}/web/uploads && \
gcc uploaded_program.c -pthread -o uploaded_program 2>&1
"""
        result = run_wsl_command(compile_cmd, timeout=10)
        
        if result['success']:
            return jsonify({
                'success': True,
                'output': result['output'] if result['output'] else 'Compilation successful! No errors or warnings.',
                'message': 'Program compiled successfully'
            })
        else:
            return jsonify({
                'success': False,
                'output': result['error'],
                'message': 'Compilation failed'
            })
    
    except Exception as e:
        return jsonify({'success': False, 'error': str(e)}), 500

@app.route('/run', methods=['POST'])
def run_program():
    """Run the program with LD_PRELOAD wrapper"""
    try:
        # Clean old event file
        clean_cmd = f"cd {WSL_PROJECT_PATH}/web/uploads && rm -f lock_events.bin"
        run_wsl_command(clean_cmd)
        
        # Run with wrapper (timeout after 3 seconds - deadlock will occur)
        run_cmd = f"""
cd {WSL_PROJECT_PATH}/web/uploads && \
timeout 3 env LD_PRELOAD={WSL_PROJECT_PATH}/liblocktrace.so ./uploaded_program 2>&1 || true
"""
        result = run_wsl_command(run_cmd, timeout=5)
        
        # Check if lock_events.bin was created
        check_cmd = f"cd {WSL_PROJECT_PATH}/web/uploads && ls -lh lock_events.bin 2>&1"
        check_result = run_wsl_command(check_cmd)
        
        output = result['output'] + '\n' + result['error']
        
        if 'lock_events.bin' in check_result['output']:
            # Events captured successfully
            output += '\n\nΓ£à Lock events captured to lock_events.bin'
            hung = 'timeout' in result['error'].lower() or result.get('timeout', False)
            
            return jsonify({
                'success': True,
                'output': output,
                'hung': hung,
                'message': 'Program executed (hung due to deadlock)' if hung else 'Program executed'
            })
        else:
            return jsonify({
                'success': False,
                'output': output,
                'message': 'Program ran but no events were captured'
            })
    
    except Exception as e:
        return jsonify({'success': False, 'error': str(e)}), 500

@app.route('/analyze', methods=['POST'])
def analyze_deadlock():
    """Analyze the captured lock events for deadlock"""
    try:
        # Run detector
        analyze_cmd = f"""
cd {WSL_PROJECT_PATH}/web/uploads && \
{WSL_PROJECT_PATH}/build/bin/detector lock_events.bin 2>&1
"""
        result = run_wsl_command(analyze_cmd, timeout=10)
        
        output = result['output'] + result['error']
        deadlock_detected = 'DEADLOCK DETECTED' in output
        
        return jsonify({
            'success': True,
            'output': output,
            'deadlock': deadlock_detected,
            'message': 'Deadlock detected!' if deadlock_detected else 'No deadlock detected'
        })
    
    except Exception as e:
        return jsonify({'success': False, 'error': str(e)}), 500

@app.route('/health', methods=['GET'])
def health():
    """Health check endpoint"""
    return jsonify({'status': 'ok', 'message': 'Backend running'})

if __name__ == '__main__':
    print("=" * 60)
    print("≡ƒÜÇ Live Deadlock Detector - Backend API")
    print("=" * 60)
    print(f"≡ƒôé Upload folder: {UPLOAD_FOLDER}")
    print(f"≡ƒÉº WSL project path: {WSL_PROJECT_PATH}")
    print("=" * 60)
    print("≡ƒîÉ Starting server on: http://localhost:5000")
    print("=" * 60)
    print("\nEndpoints:")
    print("  GET  /              ΓåÆ Serve web UI")
    print("  POST /upload        ΓåÆ Upload .c file")
    print("  POST /compile       ΓåÆ Compile program")
    print("  POST /run           ΓåÆ Run with wrapper")
    print("  POST /analyze       ΓåÆ Analyze deadlock")
    print("  GET  /health        ΓåÆ Health check")
    print("=" * 60)
    
    app.run(host='0.0.0.0', port=5000, debug=True)
