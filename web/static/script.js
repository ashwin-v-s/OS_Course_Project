/**
 * Live Deadlock Detector — Frontend Logic
 * Talks to Flask backend at http://localhost:5000
 */

'use strict';

/* ═══════════════════════════════════════════════════════
   State
═══════════════════════════════════════════════════════ */
const state = {
  selectedFile:    null,
  uploadedFilename: null,
  compiled:        false,
  ran:             false,
};

const API = 'http://localhost:5000';

/* ═══════════════════════════════════════════════════════
   DOM helpers
═══════════════════════════════════════════════════════ */
const $  = (id) => document.getElementById(id);
const el = (tag, cls, html) => {
  const e = document.createElement(tag);
  if (cls)  e.className   = cls;
  if (html) e.innerHTML   = html;
  return e;
};

/* ═══════════════════════════════════════════════════════
   Terminal
═══════════════════════════════════════════════════════ */
function termClear() {
  $('terminal').innerHTML = '';
}

function termLine(text, cls = 'terminal-out') {
  const terminal = $('terminal');
  // remove cursor from previous last line
  terminal.querySelectorAll('.terminal-cursor').forEach(n => n.classList.remove('terminal-cursor'));

  const line = el('span', `terminal-line ${cls}`, escapeHtml(text));
  terminal.appendChild(line);
  terminal.scrollTop = terminal.scrollHeight;
}

function termPrompt(cmd) {
  termLine('$ ' + cmd, 'terminal-prompt terminal-cursor');
}

function termSpinner(msg) {
  const terminal = $('terminal');
  terminal.querySelectorAll('.terminal-cursor').forEach(n => n.classList.remove('terminal-cursor'));

  const line = document.createElement('span');
  line.className = 'terminal-line terminal-info';
  line.id = 'termSpinner';
  line.innerHTML = `<span class="terminal-spinner"></span>${escapeHtml(msg)}`;
  terminal.appendChild(line);
  terminal.scrollTop = terminal.scrollHeight;
}

function termSpinnerRemove() {
  const s = $('termSpinner');
  if (s) s.remove();
}

function termOutput(raw, errorHint = false) {
  if (!raw) return;
  raw.split('\n').forEach(line => {
    if (!line) return;
    const lc = line.toLowerCase();
    let cls = 'terminal-out';
    if (errorHint || lc.includes('error') || lc.includes('fatal'))
      cls = 'terminal-error';
    else if (lc.includes('warning'))
      cls = 'terminal-warn';
    else if (lc.includes('deadlock') || lc.includes('cycle'))
      cls = 'terminal-deadlock';
    else if (lc.includes('success') || lc.includes('passed') || lc.includes('no deadlock'))
      cls = 'terminal-success';
    termLine(line, cls);
  });
}

/* ═══════════════════════════════════════════════════════
   System status badge
═══════════════════════════════════════════════════════ */
function setStatus(label, type = '') {
  const badge = $('systemStatus');
  badge.className = `badge status-badge ${type}`;
  badge.innerHTML = `<i class="bi bi-circle-fill me-1"></i>${label}`;
}

/* ═══════════════════════════════════════════════════════
   Toast notifications
═══════════════════════════════════════════════════════ */
function showToast(msg, type = 'info') {
  let container = document.querySelector('.toast-container-custom');
  if (!container) {
    container = document.createElement('div');
    container.className = 'toast-container-custom';
    document.body.appendChild(container);
  }
  const toast = el('div', `toast-custom ${type}`, escapeHtml(msg));
  container.appendChild(toast);
  setTimeout(() => toast.remove(), 3200);
}

/* ═══════════════════════════════════════════════════════
   Button enable / disable helpers
═══════════════════════════════════════════════════════ */
function setBtn(id, enabled) {
  const btn = $(id);
  if (btn) btn.disabled = !enabled;
}

function lockAllControls() {
  ['uploadBtn', 'compileBtn', 'runBtn', 'analyzeBtn'].forEach(id => setBtn(id, false));
}

function unlockAfterUpload() {
  setBtn('uploadBtn', !!state.selectedFile);
  setBtn('compileBtn', true);
  setBtn('runBtn', false);
  setBtn('analyzeBtn', false);
}

function unlockAfterCompile() {
  setBtn('compileBtn', true);
  setBtn('runBtn', true);
  setBtn('analyzeBtn', false);
}

function unlockAfterRun() {
  setBtn('runBtn', true);
  setBtn('analyzeBtn', true);
}

/* ═══════════════════════════════════════════════════════
   File selection
═══════════════════════════════════════════════════════ */
function onFileSelected(input) {
  const file = input.files[0];
  if (!file) return;

  state.selectedFile = file;

  const chip = $('selectedFile');
  chip.classList.remove('d-none');
  $('selectedFileName').textContent = file.name;

  // Update drop zone text
  const dz = $('dropZone');
  dz.querySelector('.drop-zone-text').textContent = file.name;
  dz.querySelector('.drop-zone-icon').className = 'bi bi-file-earmark-check drop-zone-icon';
  dz.style.borderColor = 'var(--accent-green)';

  setBtn('uploadBtn', true);

  termClear();
  termLine(`Selected: ${file.name}`, 'terminal-info');
}

/* ═══════════════════════════════════════════════════════
   Drag & drop support
═══════════════════════════════════════════════════════ */
document.addEventListener('DOMContentLoaded', () => {
  const dz = $('dropZone');

  dz.addEventListener('dragover', (e) => {
    e.preventDefault();
    dz.classList.add('drag-over');
  });
  dz.addEventListener('dragleave', () => dz.classList.remove('drag-over'));
  dz.addEventListener('drop', (e) => {
    e.preventDefault();
    dz.classList.remove('drag-over');
    const file = e.dataTransfer.files[0];
    if (file && file.name.endsWith('.c')) {
      const input = $('fileInput');
      // Assign to DataTransfer to trigger onFileSelected
      const dt = new DataTransfer();
      dt.items.add(file);
      input.files = dt.files;
      onFileSelected(input);
    } else {
      showToast('Please drop a .c file', 'error');
    }
  });
});

/* ═══════════════════════════════════════════════════════
   1. Upload
═══════════════════════════════════════════════════════ */
async function uploadFile() {
  if (!state.selectedFile) return;

  setStatus('Uploading…', 'busy');
  lockAllControls();

  termClear();
  termPrompt(`POST /upload  →  ${state.selectedFile.name}`);
  termSpinner('Uploading file…');

  const formData = new FormData();
  formData.append('file', state.selectedFile);

  try {
    const res  = await fetch(`${API}/upload`, {
      method: 'POST',
      body:   formData,
      mode:   'cors',
    });
    const data = await res.json();
    termSpinnerRemove();

    if (data.success) {
      state.uploadedFilename = data.filename;
      termLine(`✓ Uploaded: ${data.filename}`, 'terminal-success');
      showUploadStatus(`Uploaded: ${data.filename}`, 'success');
      showToast('File uploaded', 'success');
      setStatus('Ready', '');
      unlockAfterUpload();
    } else {
      throw new Error(data.error || data.output || 'Upload failed');
    }
  } catch (err) {
    termSpinnerRemove();
    termLine(`✗ Upload error: ${err.message}`, 'terminal-error');
    showUploadStatus(`Error: ${err.message}`, 'error');
    showToast('Upload failed', 'error');
    setStatus('Error', 'error');
    setBtn('uploadBtn', true);
  }
}

function showUploadStatus(msg, type) {
  const div = $('uploadStatus');
  div.innerHTML = `<div class="upload-msg ${type}">${escapeHtml(msg)}</div>`;
}

/* ═══════════════════════════════════════════════════════
   2. Compile
═══════════════════════════════════════════════════════ */
async function compileProgram() {
  setStatus('Compiling…', 'busy');
  setBtn('compileBtn', false);

  termLine('', 'terminal-out');  // blank separator
  termPrompt('POST /compile');
  termSpinner('Compiling with gcc -pthread…');

  try {
    const res  = await fetch(`${API}/compile`, {
      method:  'POST',
      headers: { 'Content-Type': 'application/json' },
      body:    JSON.stringify({ filename: state.uploadedFilename }),
      mode:    'cors',
    });
    const data = await res.json();
    termSpinnerRemove();

    if (data.output) termOutput(data.output, !data.success);

    if (data.success) {
      termLine('✓ Compilation successful', 'terminal-success');
      showToast('Compiled successfully', 'success');
      state.compiled = true;
      setStatus('Compiled', '');
      unlockAfterCompile();
    } else {
      termLine('✗ Compilation failed', 'terminal-error');
      showToast('Compilation failed', 'error');
      setStatus('Error', 'error');
      setBtn('compileBtn', true);
    }
  } catch (err) {
    termSpinnerRemove();
    termLine(`✗ ${err.message}`, 'terminal-error');
    showToast('Compile request failed', 'error');
    setStatus('Error', 'error');
    setBtn('compileBtn', true);
  }
}

/* ═══════════════════════════════════════════════════════
   3. Run
═══════════════════════════════════════════════════════ */
async function runProgram() {
  setStatus('Running…', 'busy');
  setBtn('runBtn', false);

  termLine('', 'terminal-out');
  termPrompt('POST /run  (LD_PRELOAD wrapper, 3 s timeout)');
  termSpinner('Running program — waiting for deadlock…');

  try {
    const res  = await fetch(`${API}/run`, {
      method:  'POST',
      headers: { 'Content-Type': 'application/json' },
      body:    JSON.stringify({ filename: state.uploadedFilename }),
      mode:    'cors',
    });
    const data = await res.json();
    termSpinnerRemove();

    if (data.output) termOutput(data.output);

    if (data.hung) {
      termLine('⚠  Program hung — deadlock detected!', 'terminal-deadlock');
      showToast('Program hung (deadlock!)', 'error');
    } else if (data.success) {
      termLine('✓ Program completed normally', 'terminal-success');
      showToast('Program finished', 'info');
    } else {
      termLine('✗ Program exited with error', 'terminal-error');
      showToast('Run failed', 'error');
    }

    state.ran = true;
    setStatus('Run complete', '');
    unlockAfterRun();
  } catch (err) {
    termSpinnerRemove();
    termLine(`✗ ${err.message}`, 'terminal-error');
    showToast('Run request failed', 'error');
    setStatus('Error', 'error');
    setBtn('runBtn', true);
  }
}

/* ═══════════════════════════════════════════════════════
   4. Analyze
═══════════════════════════════════════════════════════ */
async function analyzeDeadlock() {
  setStatus('Analyzing…', 'busy');
  setBtn('analyzeBtn', false);

  termLine('', 'terminal-out');
  termPrompt('POST /analyze  (DFS cycle detection)');
  termSpinner('Running deadlock analyzer…');

  // Clear previous analysis result
  $('analysisResult').innerHTML = `
    <div class="analysis-placeholder">
      <span class="terminal-spinner" style="width:20px;height:20px;border-width:3px"></span>
    </div>`;

  try {
    const res  = await fetch(`${API}/analyze`, {
      method:  'POST',
      headers: { 'Content-Type': 'application/json' },
      body:    JSON.stringify({}),
      mode:    'cors',
    });
    const data = await res.json();
    termSpinnerRemove();

    if (data.output) termOutput(data.output);

    renderAnalysis(data);
    setStatus('Analysis done', '');
    setBtn('analyzeBtn', true);
  } catch (err) {
    termSpinnerRemove();
    termLine(`✗ ${err.message}`, 'terminal-error');
    $('analysisResult').innerHTML = `
      <div class="analysis-placeholder">
        <i class="bi bi-exclamation-triangle analysis-placeholder-icon" style="color:var(--accent-red)"></i>
        <p class="text-muted mb-0">Analysis request failed: ${escapeHtml(err.message)}</p>
      </div>`;
    showToast('Analysis failed', 'error');
    setStatus('Error', 'error');
    setBtn('analyzeBtn', true);
  }
}

/* ── Render analysis result ─────────────────────────── */
function renderAnalysis(data) {
  const container = $('analysisResult');

  if (data.deadlock) {
    // ── DEADLOCK DETECTED ──────────────────────────────────
    termLine('🚨 DEADLOCK DETECTED', 'terminal-deadlock');

    const cycleHtml  = buildCycleHtml(data.output);
    const recHtml    = buildRecommendationHtml(data.output);

    container.innerHTML = `
      <div class="analysis-deadlock">
        <div class="analysis-title">
          <i class="bi bi-exclamation-octagon-fill me-2"></i>DEADLOCK DETECTED
        </div>
        ${cycleHtml}
        ${recHtml}
        <pre class="analysis-pre">${escapeHtml(data.output || '')}</pre>
      </div>`;
    showToast('Deadlock detected!', 'error');

  } else if (data.success) {
    // ── CLEAN ───────────────────────────────────────────────
    termLine('✓ No deadlock detected', 'terminal-success');

    container.innerHTML = `
      <div class="analysis-clean">
        <div class="analysis-title">
          <i class="bi bi-check-circle-fill me-2"></i>No Deadlock Detected
        </div>
        <p class="mb-2 small text-muted">
          The wait-for graph contains no cycles. All threads can eventually acquire their locks.
        </p>
        <pre class="analysis-pre">${escapeHtml(data.output || '')}</pre>
      </div>`;
    showToast('No deadlock found', 'success');

  } else {
    // ── ERROR ───────────────────────────────────────────────
    container.innerHTML = `
      <div class="analysis-deadlock">
        <div class="analysis-title" style="color:var(--accent-orange)">
          <i class="bi bi-exclamation-triangle-fill me-2"></i>Analyzer Error
        </div>
        <pre class="analysis-pre">${escapeHtml(data.output || data.error || 'Unknown error')}</pre>
      </div>`;
  }
}

/* ── Extract cycle path from raw output ─────────────── */
function buildCycleHtml(raw) {
  if (!raw) return '';

  // Look for patterns like "Thread 123 → Thread 456 → Thread 123"
  // or "Cycle: T1 -> T2 -> T3 -> T1"
  const cycleMatch = raw.match(/[Cc]ycle[:\s]+(.+)/);
  if (!cycleMatch) return '';

  const parts = cycleMatch[1]
    .split(/→|->|⇄|<->/)
    .map(s => s.trim())
    .filter(Boolean);

  if (parts.length < 2) return '';

  const nodes = parts.map(p => `<span class="cycle-node">${escapeHtml(p)}</span>`);
  const html  = nodes.join('<span class="cycle-arrow"> → </span>');

  return `
    <div class="mt-2 mb-1 small text-muted fw-semibold">
      <i class="bi bi-arrow-repeat me-1"></i>Deadlock Cycle
    </div>
    <div class="cycle-path">${html}<span class="cycle-arrow"> ↩</span></div>`;
}

/* ── Extract recommendation from raw output ─────────── */
function buildRecommendationHtml(raw) {
  if (!raw) return '';

  // Look for "Recommendation:" or "Fix:" sections
  const recMatch = raw.match(/(?:[Rr]ecommendation|[Ff]ix|[Ss]uggest)[:\s]+([^\n]+(?:\n(?!\n).+)*)/);
  if (!recMatch) return '';

  return `
    <div class="recommendation-box">
      <strong><i class="bi bi-lightbulb me-1"></i>Recommendation:</strong>
      ${escapeHtml(recMatch[1].trim())}
    </div>`;
}

/* ═══════════════════════════════════════════════════════
   Copy terminal output
═══════════════════════════════════════════════════════ */
function copyTerminal() {
  const text = Array.from($('terminal').querySelectorAll('.terminal-line'))
    .map(n => n.textContent)
    .join('\n');
  navigator.clipboard.writeText(text).then(() => showToast('Copied to clipboard', 'info'));
}

/* ═══════════════════════════════════════════════════════
   Reset
═══════════════════════════════════════════════════════ */
function resetAll() {
  state.selectedFile     = null;
  state.uploadedFilename = null;
  state.compiled         = false;
  state.ran              = false;

  // Reset file input
  const input = $('fileInput');
  input.value = '';

  // Reset drop zone
  const dz = $('dropZone');
  dz.querySelector('.drop-zone-text').textContent = 'Click or drag & drop';
  dz.querySelector('.drop-zone-icon').className = 'bi bi-cloud-upload drop-zone-icon';
  dz.style.borderColor = '';

  $('selectedFile').classList.add('d-none');
  $('uploadStatus').innerHTML = '';
  $('analysisResult').innerHTML = `
    <div class="analysis-placeholder">
      <i class="bi bi-clock-history analysis-placeholder-icon"></i>
      <p class="mb-0 text-muted">
        Run the program first, then click <strong>Analyze Deadlock</strong>.
      </p>
    </div>`;

  termClear();
  termLine('┌─────────────────────────────────────┐', 'terminal-welcome');
  termLine('│  Live Deadlock Detector v1.0        │', 'terminal-welcome');
  termLine('│  Upload a .c file to begin.         │', 'terminal-welcome');
  termLine('└─────────────────────────────────────┘', 'terminal-welcome');

  lockAllControls();
  setStatus('Ready', '');
  showToast('Reset', 'info');
}

/* ═══════════════════════════════════════════════════════
   Utility
═══════════════════════════════════════════════════════ */
function escapeHtml(str) {
  if (typeof str !== 'string') return String(str ?? '');
  return str
    .replace(/&/g,  '&amp;')
    .replace(/</g,  '&lt;')
    .replace(/>/g,  '&gt;')
    .replace(/"/g,  '&quot;')
    .replace(/'/g,  '&#39;');
}
