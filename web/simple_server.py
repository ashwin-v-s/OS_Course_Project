#!/usr/bin/env python3
"""
Simple HTTP Server for Live Deadlock Detector (NO Flask needed!)
Uses only Python built-in libraries
"""

import http.server
import socketserver
import json
import subprocess
import os
import urllib.parse
from pathlib import Path
import io

PORT = 5000
WSL_PROJECT_PATH = os.path.expanduser('~/live-deadlock-detector')
UPLOAD_FOLDER = Path(__file__).parent / 'uploads'
UPLOAD_FOLDER.mkdir(exist_ok=True)

def run_wsl_command(command, timeout=30):
    """Execute command in WSL and return output"""
    try:
        # Already in WSL, just run bash
        result = subprocess.run(
            ['bash', '-c', command],
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

class DeadlockDetectorHandler(http.server.SimpleHTTPRequestHandler):
    
    def do_GET(self):
        """Handle GET requests"""
        if self.path == '/' or self.path == '/index.html':
            self.send_response(200)
            self.send_header('Content-type', 'text/html')
            self.end_headers()
            
            # Read and serve index.html
            index_path = Path(__file__).parent / 'templates' / 'index.html'
            if index_path.exists():
                with open(index_path, 'r') as f:
                    self.wfile.write(f.read().encode())
            else:
                self.wfile.write(b'<h1>Error: index.html not found</h1>')
        
        elif self.path.startswith('/static/'):
            # Serve static files
            file_path = Path(__file__).parent / self.path[1:]
            if file_path.exists():
                if self.path.endswith('.css'):
                    content_type = 'text/css'
                elif self.path.endswith('.js'):
                    content_type = 'application/javascript'
                else:
                    content_type = 'text/plain'
                
                self.send_response(200)
                self.send_header('Content-type', content_type)
                self.end_headers()
                with open(file_path, 'rb') as f:
                    self.wfile.write(f.read())
            else:
                self.send_error(404)
        
        elif self.path == '/health':
            self.send_json_response({'status': 'ok', 'message': 'Backend running'})
        
        else:
            self.send_error(404)
    
    def do_POST(self):
        """Handle POST requests"""
        if self.path == '/upload':
            self.handle_upload()
        elif self.path == '/compile':
            self.handle_compile()
        elif self.path == '/run':
            self.handle_run()
        elif self.path == '/analyze':
            self.handle_analyze()
        else:
            self.send_error(404)
    
    def handle_upload(self):
        """Handle file upload"""
        try:
            content_length = int(self.headers['Content-Length'])
            content_type = self.headers['Content-Type']
            
            if 'multipart/form-data' not in content_type:
                self.send_json_response({'success': False, 'error': 'Invalid content type'}, 400)
                return
            
            # Read the entire body
            body = self.rfile.read(content_length)
            
            # Parse boundary
            boundary = content_type.split('boundary=')[1].encode()
            
            # Split by boundary
            parts = body.split(b'--' + boundary)
            
            file_content = None
            filename = None
            
            for part in parts:
                if b'Content-Disposition' in part and b'filename=' in part:
                    # Extract filename
                    header_end = part.find(b'\r\n\r\n')
                    if header_end != -1:
                        headers = part[:header_end].decode('utf-8', errors='ignore')
                        for line in headers.split('\r\n'):
                            if 'filename=' in line:
                                filename = line.split('filename=')[1].strip('"')
                        
                        # Extract file content
                        file_content = part[header_end+4:].rstrip(b'\r\n')
            
            if not filename:
                self.send_json_response({'success': False, 'error': 'No file selected'}, 400)
                return
            
            if not filename.endswith('.c'):
                self.send_json_response({'success': False, 'error': 'Only .c files allowed'}, 400)
                return
            
            # Save file
            save_filename = 'uploaded_program.c'
            filepath = UPLOAD_FOLDER / save_filename
            
            with open(filepath, 'wb') as f:
                f.write(file_content)
            
            self.send_json_response({
                'success': True,
                'filename': save_filename,
                'message': 'File uploaded successfully'
            })
        
        except Exception as e:
            self.send_json_response({'success': False, 'error': str(e)}, 500)
    
    def handle_compile(self):
        """Compile the uploaded C program"""
        try:
            compile_cmd = f"""
cd {WSL_PROJECT_PATH}/web/uploads && \
gcc uploaded_program.c -pthread -o uploaded_program 2>&1
"""
            result = run_wsl_command(compile_cmd, timeout=10)
            
            if result['success']:
                self.send_json_response({
                    'success': True,
                    'output': result['output'] if result['output'] else 'Compilation successful! No errors or warnings.',
                    'message': 'Program compiled successfully'
                })
            else:
                self.send_json_response({
                    'success': False,
                    'output': result['error'],
                    'message': 'Compilation failed'
                })
        
        except Exception as e:
            self.send_json_response({'success': False, 'error': str(e)}, 500)
    
    def handle_run(self):
        """Run the program with LD_PRELOAD wrapper"""
        try:
            # Clean old event file
            clean_cmd = f"cd {WSL_PROJECT_PATH}/web/uploads && rm -f lock_events.bin"
            run_wsl_command(clean_cmd)
            
            # Run with wrapper
            run_cmd = f"""
cd {WSL_PROJECT_PATH}/web/uploads && \
timeout 3 env LD_PRELOAD={WSL_PROJECT_PATH}/liblocktrace.so ./uploaded_program 2>&1 || true
"""
            result = run_wsl_command(run_cmd, timeout=5)
            
            # Check if events captured
            check_cmd = f"cd {WSL_PROJECT_PATH}/web/uploads && ls -lh lock_events.bin 2>&1"
            check_result = run_wsl_command(check_cmd)
            
            output = result['output'] + '\n' + result['error']
            
            if 'lock_events.bin' in check_result['output']:
                output += '\n\nΓ£à Lock events captured to lock_events.bin'
                hung = 'timeout' in result['error'].lower() or result.get('timeout', False)
                
                self.send_json_response({
                    'success': True,
                    'output': output,
                    'hung': hung,
                    'message': 'Program executed (hung due to deadlock)' if hung else 'Program executed'
                })
            else:
                self.send_json_response({
                    'success': False,
                    'output': output,
                    'message': 'Program ran but no events were captured'
                })
        
        except Exception as e:
            self.send_json_response({'success': False, 'error': str(e)}, 500)
    
    def handle_analyze(self):
        """Analyze the captured lock events for deadlock"""
        try:
            analyze_cmd = f"""
cd {WSL_PROJECT_PATH}/web/uploads && \
{WSL_PROJECT_PATH}/build/bin/detector lock_events.bin 2>&1
"""
            result = run_wsl_command(analyze_cmd, timeout=10)
            
            output = result['output'] + result['error']
            deadlock_detected = 'DEADLOCK DETECTED' in output
            
            self.send_json_response({
                'success': True,
                'output': output,
                'deadlock': deadlock_detected,
                'message': 'Deadlock detected!' if deadlock_detected else 'No deadlock detected'
            })
        
        except Exception as e:
            self.send_json_response({'success': False, 'error': str(e)}, 500)
    
    def send_json_response(self, data, status=200):
        """Send JSON response"""
        self.send_response(status)
        self.send_header('Content-type', 'application/json')
        self.send_header('Access-Control-Allow-Origin', '*')
        self.end_headers()
        self.wfile.write(json.dumps(data).encode())

if __name__ == '__main__':
    os.chdir(Path(__file__).parent)
    
    print("=" * 60)
    print("≡ƒÜÇ Live Deadlock Detector - Simple Backend")
    print("=" * 60)
    print(f"≡ƒôé Upload folder: {UPLOAD_FOLDER}")
    print(f"≡ƒÉº WSL project path: {WSL_PROJECT_PATH}")
    print("=" * 60)
    print(f"≡ƒîÉ Starting server on: http://localhost:{PORT}")
    print("=" * 60)
    print("\nPress Ctrl+C to stop the server")
    print("=" * 60)
    
    with socketserver.TCPServer(("", PORT), DeadlockDetectorHandler) as httpd:
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\n\nΓ£à Server stopped")
