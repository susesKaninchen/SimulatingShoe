#pragma once
#include "defines.h"

// ── Helper: step state as string ──────────────────────────────────────────────
static const char* stepStateStr() {
    switch (sState) {
        case GROUND:         return "GROUND";
        case STANDING:       return "STANDING";
        case UP:             return "UP";
        case DOWN:           return "DOWN";
        default:             return "UNDEFINED";
    }
}

// ── Helper: valve system state as string ──────────────────────────────────────
static String valveStateStr(int s) {
    if (s == 0) return "Pump Front";
    if (s == 1) return "Pump Back";
    if (s == 2) return "Even Out";
    if (s == 3) return "Pause";
    if (s == 4) return "Open";
    if (s == 5) return "Even Out Forever";
    if (s == 6) return "Calibrating";
    if (s < 5000) return "Hold @ " + String(s);
    return "Pump -> " + String((int)(s * 0.1f));
}

// ─────────────────────────────────────────────────────────────────────────────
// Dashboard HTML (stored in flash / PROGMEM)
// ─────────────────────────────────────────────────────────────────────────────
const char DASHBOARD_HTML[] PROGMEM = R"***(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>SimulatingShoe — Demo</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{background:#0d1117;color:#c9d1d9;font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',sans-serif;font-size:14px;height:100vh;display:flex;flex-direction:column;overflow:hidden}
header{background:#161b22;border-bottom:1px solid #30363d;padding:10px 18px;display:flex;align-items:center;justify-content:space-between;flex-shrink:0}
header h1{font-size:17px;color:#58a6ff;font-weight:600}
.hdr-right{display:flex;align-items:center;gap:18px;font-size:12px;color:#8b949e}
.dot{width:9px;height:9px;border-radius:50%;background:#3fb950;display:inline-block;margin-right:5px;transition:background .3s}
.dot.off{background:#f85149}
.main{display:grid;grid-template-columns:minmax(200px,280px) 1fr;gap:12px;padding:12px;flex:1;overflow:hidden;min-height:0}
.col{display:flex;flex-direction:column;gap:10px;overflow:hidden}
.card{background:#161b22;border:1px solid #30363d;border-radius:8px;padding:12px;flex-shrink:0}
.card.grow{flex:1;min-height:0;display:flex;flex-direction:column}
.card h2{font-size:11px;text-transform:uppercase;letter-spacing:.5px;color:#8b949e;margin-bottom:10px}
/* ── Shoe SVG ── */
#shoe-wrap{display:flex;justify-content:center;flex:1;min-height:0}
#shoe-svg{width:100%;height:100%;max-height:320px}
/* ── Status grid ── */
.stat-grid{display:grid;grid-template-columns:1fr 1fr;gap:7px}
.stat{background:#21262d;border-radius:6px;padding:8px 10px}
.stat-label{font-size:10px;color:#8b949e;margin-bottom:3px;text-transform:uppercase;letter-spacing:.3px}
.stat-val{font-size:16px;font-weight:600;color:#e6edf3}
.stat-val.GROUND{color:#3fb950}
.stat-val.STANDING{color:#d29922}
.stat-val.UP{color:#58a6ff}
.stat-val.UNDEFINED{color:#6e7681}
/* ── Charts ── */
canvas.chart{width:100%;height:100%;display:block;background:#0d1117;border-radius:4px}
.chart-wrap{flex:1;position:relative;min-height:0;overflow:hidden}
.chart-wrap canvas{position:absolute;top:0;left:0;width:100%;height:100%}
.legend{display:flex;gap:14px;margin-top:5px;flex-wrap:wrap}
.leg{display:flex;align-items:center;gap:5px;font-size:11px;color:#8b949e}
.leg-line{width:22px;height:2px;border-radius:1px}
/* ── Raw values row ── */
.raw-grid{display:grid;grid-template-columns:repeat(4,1fr);gap:7px;margin-top:8px}
/* ── Controls ── */
.controls{background:#161b22;border-top:1px solid #30363d;padding:10px 16px;flex-shrink:0}
.ctrl-row{display:flex;gap:20px;align-items:flex-start;flex-wrap:wrap}
.ctrl-group h3{font-size:10px;color:#8b949e;text-transform:uppercase;letter-spacing:.4px;margin-bottom:7px}
.btn-row{display:flex;gap:6px;flex-wrap:wrap;align-items:center}
button{background:#21262d;color:#c9d1d9;border:1px solid #30363d;border-radius:6px;padding:5px 11px;cursor:pointer;font-size:12px;transition:all .15s;white-space:nowrap}
button:hover{background:#30363d;border-color:#58a6ff}
button.active{background:#1f6feb;border-color:#388bfd;color:#fff}
button.red:hover{background:#3d1414;border-color:#f85149;color:#f85149}
button.green{border-color:#3fb950;color:#3fb950}
button.green:hover{background:#1a3a1a}
input[type=number]{background:#0d1117;border:1px solid #30363d;border-radius:6px;color:#c9d1d9;padding:4px 8px;font-size:12px;width:85px}
input[type=range]{width:110px;accent-color:#388bfd}
label{font-size:11px;color:#8b949e}
</style>
</head>
<body>

<header>
  <div style="display:flex;align-items:center;gap:14px">
    <h1>SimulatingShoe</h1>
    <span style="color:#8b949e;font-size:12px">Trade Show Dashboard</span>
  </div>
  <div class="hdr-right">
    <span><span class="dot" id="dot"></span><span id="conn">Connecting…</span></span>
    <span id="heap"></span>
    <span id="ip-addr"></span>
  </div>
</header>

<div class="main">
  <!-- ── Left column ────────────────────────────────────────────── -->
  <div class="col">

    <!-- Shoe diagram -->
    <div class="card grow">
      <h2>Shoe — Live View</h2>
      <div id="shoe-wrap">
      <svg id="shoe-svg" viewBox="0 0 230 315" xmlns="http://www.w3.org/2000/svg">
        <defs>
          <filter id="glow" x="-60%" y="-60%" width="220%" height="220%">
            <feGaussianBlur stdDeviation="3.5" result="blur"/>
            <feMerge><feMergeNode in="blur"/><feMergeNode in="SourceGraphic"/></feMerge>
          </filter>
        </defs>

        <!-- ── Sole outline (figure-8 / waist shape) ── -->
        <!-- Cubic bezier: toe top → right side → waist → heel → left side → close -->
        <path d="M90,12
                 C 132,12 160,40 160,82
                 C 160,114 144,136 144,158
                 C 144,180 160,202 160,244
                 C 160,278 130,298 90,298
                 C 50,298 20,278 20,244
                 C 20,202 36,180 36,158
                 C 36,136 20,114 20,82
                 C 20,40 48,12 90,12 Z"
              fill="#1c2128" stroke="#404850" stroke-width="2"/>

        <!-- Connector channel (visible gap at waist) -->
        <rect x="36" y="150" width="108" height="16" rx="4"
              fill="#252d36" stroke="#35404a" stroke-width="1"/>

        <!-- ── FRONT / TOE chamber ── -->
        <ellipse id="ch-front" cx="90" cy="88" rx="50" ry="58"
                 fill="#1565C0" opacity="0.85"/>
        <text x="90" y="83" text-anchor="middle" fill="#ffffffdd"
              font-size="11" font-weight="600" letter-spacing="0.5">FRONT</text>
        <text id="lbl-front" x="90" y="98" text-anchor="middle"
              fill="#ffffffaa" font-size="10">0</text>

        <!-- ── HEEL chamber ── -->
        <ellipse id="ch-heel" cx="90" cy="240" rx="56" ry="50"
                 fill="#1565C0" opacity="0.85"/>
        <text x="90" y="235" text-anchor="middle" fill="#ffffffdd"
              font-size="11" font-weight="600" letter-spacing="0.5">HEEL</text>
        <text id="lbl-heel" x="90" y="250" text-anchor="middle"
              fill="#ffffffaa" font-size="10">0</text>

        <!-- ── Valves (right side, outside sole) ── -->
        <!-- Front inlet (zuluftV) -->
        <line x1="160" y1="78" x2="178" y2="68" stroke="#35404a" stroke-width="1" stroke-dasharray="3,2"/>
        <circle id="v-front" cx="188" cy="63" r="11"
                fill="#30363d" stroke="#505860" stroke-width="1.5"/>
        <text x="188" y="67" text-anchor="middle" fill="#cdd5de" font-size="8" font-weight="bold">F</text>

        <!-- Connector valve (verbinder) -->
        <line x1="144" y1="158" x2="178" y2="158" stroke="#35404a" stroke-width="1" stroke-dasharray="3,2"/>
        <circle id="v-conn" cx="188" cy="158" r="11"
                fill="#30363d" stroke="#505860" stroke-width="1.5"/>
        <text x="188" y="162" text-anchor="middle" fill="#cdd5de" font-size="8" font-weight="bold">C</text>

        <!-- Heel inlet (zuluftH) -->
        <line x1="160" y1="244" x2="178" y2="252" stroke="#35404a" stroke-width="1" stroke-dasharray="3,2"/>
        <circle id="v-heel" cx="188" cy="258" r="11"
                fill="#30363d" stroke="#505860" stroke-width="1.5"/>
        <text x="188" y="262" text-anchor="middle" fill="#cdd5de" font-size="8" font-weight="bold">H</text>

        <!-- Valve labels -->
        <text x="204" y="67"  fill="#5a6472" font-size="8">Front In</text>
        <text x="204" y="162" fill="#5a6472" font-size="8">Connect</text>
        <text x="204" y="262" fill="#5a6472" font-size="8">Heel In</text>

        <!-- ── Vibration motors (6x) ── -->
        <!-- Front chamber: 0 left, 1 right -->
        <circle id="m0" cx="50" cy="68" r="10" fill="#21262d" stroke="#388bfd" stroke-width="1.5"/>
        <text x="50" y="72" text-anchor="middle" fill="#8b949e" font-size="9">0</text>
        <circle id="m1" cx="130" cy="68" r="10" fill="#21262d" stroke="#388bfd" stroke-width="1.5"/>
        <text x="130" y="72" text-anchor="middle" fill="#8b949e" font-size="9">1</text>
        <!-- Front chamber lower: 2 left, 3 right -->
        <circle id="m2" cx="50" cy="118" r="10" fill="#21262d" stroke="#388bfd" stroke-width="1.5"/>
        <text x="50" y="122" text-anchor="middle" fill="#8b949e" font-size="9">2</text>
        <circle id="m3" cx="130" cy="118" r="10" fill="#21262d" stroke="#388bfd" stroke-width="1.5"/>
        <text x="130" y="122" text-anchor="middle" fill="#8b949e" font-size="9">3</text>
        <!-- Heel chamber: 4 left, 5 right -->
        <circle id="m4" cx="44" cy="248" r="10" fill="#21262d" stroke="#388bfd" stroke-width="1.5"/>
        <text x="44" y="252" text-anchor="middle" fill="#8b949e" font-size="9">4</text>
        <circle id="m5" cx="136" cy="248" r="10" fill="#21262d" stroke="#388bfd" stroke-width="1.5"/>
        <text x="136" y="252" text-anchor="middle" fill="#8b949e" font-size="9">5</text>
      </svg>
      </div>
    </div>

    <!-- Status cards -->
    <div class="card">
      <h2>Status</h2>
      <div class="stat-grid">
        <div class="stat">
          <div class="stat-label">Step State</div>
          <div class="stat-val UNDEFINED" id="step-state">—</div>
        </div>
        <div class="stat">
          <div class="stat-label">Mode</div>
          <div class="stat-val" id="sim-mode" style="font-size:13px">—</div>
        </div>
        <div class="stat">
          <div class="stat-label">Valve System</div>
          <div class="stat-val" id="valve-state" style="font-size:12px">—</div>
        </div>
        <div class="stat">
          <div class="stat-label">Vibration</div>
          <div class="stat-val" id="vib-state">—</div>
        </div>
      </div>
    </div>

  </div><!-- end left col -->

  <!-- ── Right column ───────────────────────────────────────────── -->
  <div class="col">

    <!-- Pressure chart -->
    <div class="card grow" style="min-height:160px">
      <h2>Pressure — Relative to Baseline (ADC counts)</h2>
      <div class="chart-wrap"><canvas id="pChart" class="chart"></canvas></div>
      <div class="legend">
        <div class="leg"><div class="leg-line" style="background:#58a6ff"></div>Heel</div>
        <div class="leg"><div class="leg-line" style="background:#3fb950"></div>Front</div>
      </div>
    </div>

    <!-- IMU chart -->
    <div class="card grow" style="min-height:130px">
      <h2>IMU — Acceleration (raw / 100)</h2>
      <div class="chart-wrap"><canvas id="imuChart" class="chart"></canvas></div>
      <div class="legend">
        <div class="leg"><div class="leg-line" style="background:#f78166"></div>X</div>
        <div class="leg"><div class="leg-line" style="background:#3fb950"></div>Y</div>
        <div class="leg"><div class="leg-line" style="background:#d29922"></div>Z</div>
      </div>
    </div>

    <!-- Raw numbers -->
    <div class="card" style="flex-shrink:0">
      <h2>Raw Sensor Values</h2>
      <div class="raw-grid">
        <div class="stat"><div class="stat-label">Heel Raw</div><div class="stat-val" id="r-heel" style="font-size:15px">—</div></div>
        <div class="stat"><div class="stat-label">Front Raw</div><div class="stat-val" id="r-front" style="font-size:15px">—</div></div>
        <div class="stat"><div class="stat-label">Heel Δ</div><div class="stat-val" id="r-dh" style="font-size:15px">—</div></div>
        <div class="stat"><div class="stat-label">Front Δ</div><div class="stat-val" id="r-dv" style="font-size:15px">—</div></div>
      </div>
    </div>

  </div><!-- end right col -->
</div><!-- end .main -->

<!-- ── Controls ───────────────────────────────────────────────────── -->
<div class="controls">
  <div class="ctrl-row">

    <div class="ctrl-group">
      <h3>Surface Simulation</h3>
      <div class="btn-row">
        <button id="bs-asphalt"  onclick="surf('asphalt')">Asphalt</button>
        <button id="bs-grass"    onclick="surf('grass')">Grass</button>
        <button id="bs-sand"     onclick="surf('sand')">Sand</button>
        <button id="bs-linoleum" onclick="surf('linoleum')">Linoleum</button>
        <button id="bs-gravel"   onclick="surf('gravel')">Gravel</button>
      </div>
    </div>

    <div class="ctrl-group">
      <h3>Pressure Control</h3>
      <div class="btn-row">
        <label>Target:</label>
        <input type="number" id="p-target" value="200" min="0" max="4000" step="50">
        <button class="green" onclick="holdAt()">Go &amp; Hold</button>
        <button onclick="pumpTo()">Pump &amp; Release</button>
        <button onclick="api('evenout')">Even Out</button>
        <button class="red" onclick="api('pause')">Pause</button>
        <button onclick="api('calibrate')">Calibrate</button>
      </div>
    </div>

    <div class="ctrl-group">
      <h3>Vibration</h3>
      <div class="btn-row">
        <button id="btn-vib" onclick="api('vibrate').then(now)">Toggle On/Off</button>
        <button onclick="api('vibmode/10').then(now)" title="All motors on continuously">Test Continuous</button>
        <button onclick="api('vibmode/0').then(now)">Off</button>
        <span style="color:#8b949e;font-size:11px;margin-left:4px">LEDC test:</span>
        <button onclick="testMotor(0)" title="GPIO shown in Serial">M0</button>
        <button onclick="testMotor(1)">M1</button>
        <button onclick="testMotor(2)">M2</button>
        <button onclick="testMotor(3)">M3</button>
        <button onclick="testMotor(4)">M4</button>
        <button onclick="testMotor(5)">M5</button>
        <span style="color:#8b949e;font-size:11px;margin-left:4px">Digital test:</span>
        <button onclick="digTest(0)" title="digitalWrite HIGH for 1s">M0</button>
        <button onclick="digTest(1)">M1</button>
        <button onclick="digTest(2)">M2</button>
        <button onclick="digTest(3)">M3</button>
        <button onclick="digTest(4)">M4</button>
        <button onclick="digTest(5)">M5</button>
        <label style="margin-left:6px">Strength:</label>
        <input type="range" id="vib-s" min="0" max="255" value="200"
               oninput="document.getElementById('vib-sv').textContent=this.value;api('vibstrength/'+this.value)">
        <span id="vib-sv" style="color:#8b949e;font-size:12px;min-width:24px">200</span>
      </div>
    </div>

  </div>
</div>

<script>
// ═══════════════════════════════════════════════════════════
// Rolling Chart
// ═══════════════════════════════════════════════════════════
class RollingChart {
  constructor(id, seriesColors, opts) {
    this.canvas = document.getElementById(id);
    this.ctx = this.canvas.getContext('2d');
    this.colors = seriesColors;
    this.data = seriesColors.map(() => []);
    this.maxPts = (opts && opts.maxPts) || 120;
    this.fixedMin = opts && opts.yMin !== undefined ? opts.yMin : null;
    this.fixedMax = opts && opts.yMax !== undefined ? opts.yMax : null;
    const ro = new ResizeObserver(() => this._resize());
    ro.observe(this.canvas.parentElement);  // observe wrapper, not canvas itself
    this._resize();
    window.addEventListener('resize', () => this._resize());
  }
  _resize() {
    const r = window.devicePixelRatio || 1;
    this.canvas.width  = this.canvas.offsetWidth  * r;
    this.canvas.height = this.canvas.offsetHeight * r;
    this._draw();
  }
  push(vals) {
    vals.forEach((v, i) => {
      this.data[i].push(v);
      if (this.data[i].length > this.maxPts) this.data[i].shift();
    });
    this._draw();
  }
  // Push an array of sample-arrays at once, redraw only once at the end
  pushAll(arr) {
    arr.forEach(vals => {
      vals.forEach((v, i) => {
        this.data[i].push(v);
        if (this.data[i].length > this.maxPts) this.data[i].shift();
      });
    });
    this._draw();
  }
  _draw() {
    const {ctx, canvas, data, colors, maxPts} = this;
    const W = canvas.width, H = canvas.height;
    const r = window.devicePixelRatio || 1;
    ctx.clearRect(0, 0, W, H);
    ctx.fillStyle = '#0d1117';
    ctx.fillRect(0, 0, W, H);

    // Y range
    let yMin = this.fixedMin, yMax = this.fixedMax;
    if (yMin === null) {
      const all = data.flat();
      if (!all.length) { yMin = -10; yMax = 10; }
      else {
        yMin = Math.min(...all); yMax = Math.max(...all);
        const pad = (yMax - yMin) * 0.12 || 5;
        yMin -= pad; yMax += pad;
      }
    }
    const range = yMax - yMin || 1;

    // Grid lines
    ctx.strokeStyle = '#21262d'; ctx.lineWidth = 1;
    ctx.font = `${10*r}px monospace`;
    ctx.fillStyle = '#484f58';
    for (let i = 0; i <= 4; i++) {
      const y = H * (1 - i/4);
      ctx.beginPath(); ctx.moveTo(0,y); ctx.lineTo(W,y); ctx.stroke();
      ctx.fillText(Math.round(yMin + range*i/4), 4*r, y - 3*r);
    }

    // Zero line
    if (yMin < 0 && yMax > 0) {
      const y0 = H * (1 - (0-yMin)/range);
      ctx.save();
      ctx.strokeStyle = '#3d444d'; ctx.lineWidth = 1;
      ctx.setLineDash([4*r,4*r]);
      ctx.beginPath(); ctx.moveTo(0,y0); ctx.lineTo(W,y0); ctx.stroke();
      ctx.restore();
    }

    // Series lines
    data.forEach((pts, si) => {
      if (pts.length < 2) return;
      ctx.beginPath();
      ctx.strokeStyle = colors[si];
      ctx.lineWidth = 1.8 * r;
      pts.forEach((v, i) => {
        const x = (i / (maxPts-1)) * W;
        const y = H * (1 - (v - yMin) / range);
        i === 0 ? ctx.moveTo(x,y) : ctx.lineTo(x,y);
      });
      ctx.stroke();
    });
  }
}

// ═══════════════════════════════════════════════════════════
// Initialise charts
// ═══════════════════════════════════════════════════════════
// 600 pts × 25 ms = 15 s of full-resolution history
const pChart   = new RollingChart('pChart',   ['#58a6ff','#3fb950'],           {maxPts:600, yMin:-200, yMax:2500});
const imuChart = new RollingChart('imuChart', ['#f78166','#3fb950','#d29922'], {maxPts:600});

// ═══════════════════════════════════════════════════════════
// Surface presets (vMode → surface button id)
// ═══════════════════════════════════════════════════════════
const SURF_MODE = {4:'asphalt', 5:'gravel', 6:'grass', 7:'sand'};
const VMODE_NAME = ['Off','Random','On Impact','Gait','Asphalt','Gravel','Grass','Sand',
                    'On Ground','In Air','Continuous'];

// ═══════════════════════════════════════════════════════════
// Pressure → RGB (blue → green → red)
// ═══════════════════════════════════════════════════════════
function pColor(rel) {
  const t = Math.max(0, Math.min(1, rel / 1500));
  let r, g, b;
  if (t < 0.5) {
    const f = t*2;
    r = Math.round(21  + f*(63-21));
    g = Math.round(101 + f*(185-101));
    b = Math.round(192 + f*(80-192));
  } else {
    const f = (t-0.5)*2;
    r = Math.round(63  + f*(198-63));
    g = Math.round(185 + f*(40-185));
    b = Math.round(80  + f*(40-80));
  }
  return `rgb(${r},${g},${b})`;
}

// ═══════════════════════════════════════════════════════════
// Update UI from JSON data object
// ═══════════════════════════════════════════════════════════
function update(d) {
  // Charts — use full-resolution buffer if available, else fall back to single point
  if (d.buf && d.buf.length) {
    pChart.pushAll(d.buf.map(s => [s[0], s[1]]));
    imuChart.pushAll(d.buf.map(s => [s[2], s[3], s[4]]));
  } else {
    pChart.push([d.prH, d.prV]);
    imuChart.push([d.ax/100, d.ay/100, d.az/100]);
  }

  // Raw numbers
  document.getElementById('r-heel').textContent  = d.pH;
  document.getElementById('r-front').textContent = d.pV;
  document.getElementById('r-dh').textContent    = (d.prH >= 0 ? '+' : '') + d.prH;
  document.getElementById('r-dv').textContent    = (d.prV >= 0 ? '+' : '') + d.prV;

  // Step state
  const ss = document.getElementById('step-state');
  ss.textContent = d.step;
  ss.className   = 'stat-val ' + d.step;

  // Simulation mode
  document.getElementById('sim-mode').textContent = VMODE_NAME[d.vMode] || ('Mode '+d.vMode);

  // Valve system
  document.getElementById('valve-state').textContent = d.valveLabel;

  // Vibration
  const vs = document.getElementById('vib-state');
  vs.textContent = d.vib ? 'ON' : 'OFF';
  vs.style.color = d.vib ? '#3fb950' : '#6e7681';

  // Heap
  document.getElementById('heap').textContent = 'Heap: ' + Math.round(d.heap/1024) + ' KB';

  // Chamber pressure colours
  document.getElementById('ch-heel').setAttribute('fill',  pColor(d.prH));
  document.getElementById('ch-front').setAttribute('fill', pColor(d.prV));
  document.getElementById('lbl-heel').textContent  = (d.prH >= 0 ? '+' : '') + d.prH;
  document.getElementById('lbl-front').textContent = (d.prV >= 0 ? '+' : '') + d.prV;

  // Valves
  const vc = (id, on) => {
    const el = document.getElementById(id);
    el.setAttribute('fill', on ? '#3fb950' : '#30363d');
    if (on) el.setAttribute('filter','url(#glow)');
    else    el.removeAttribute('filter');
  };
  vc('v-heel',  d.vH);
  vc('v-conn',  d.vC);
  vc('v-front', d.vF);

  // Motors
  for (let i = 0; i < 6; i++) {
    const el  = document.getElementById('m'+i);
    const val = d.m[i];
    if (val > 0) {
      const a = 0.3 + (val/255)*0.7;
      el.setAttribute('fill', `rgba(56,139,253,${a.toFixed(2)})`);
      el.setAttribute('filter','url(#glow)');
    } else {
      el.setAttribute('fill','#21262d');
      el.removeAttribute('filter');
    }
    document.getElementById('m'+i).querySelector ?
      void 0 : void 0; // labels update via fill
  }

  // Active surface button highlight
  document.querySelectorAll('[id^=bs-]').forEach(b => b.classList.remove('active'));
  const sf = SURF_MODE[d.vMode];
  if (sf) {
    const b = document.getElementById('bs-'+sf);
    if (b) b.classList.add('active');
  }
}

// ═══════════════════════════════════════════════════════════
// Fetch loop
// ═══════════════════════════════════════════════════════════
let fails = 0;
async function poll() {
  try {
    const r = await fetch('/api/status', {cache:'no-store'});
    if (!r.ok) throw new Error();
    const d = await r.json();
    update(d);
    fails = 0;
    document.getElementById('dot').className  = 'dot';
    document.getElementById('conn').textContent = 'Connected';
  } catch(e) {
    document.getElementById('dot').className  = 'dot off';
    document.getElementById('conn').textContent = 'No signal (' + (++fails) + ')';
  }
}

const now = () => new Promise(r=>setTimeout(r,120)).then(poll);
setInterval(poll, 500);
poll();

// ═══════════════════════════════════════════════════════════
// Control helpers
// ═══════════════════════════════════════════════════════════
const api  = path => fetch('/api/' + path, {cache:'no-store'});

function surf(name) { api('surface/' + name).then(now); }

function holdAt() {
  const v = parseInt(document.getElementById('p-target').value) || 0;
  // state < 5000 → holdValue(state)
  api('hold/' + Math.min(Math.max(v, 0), 4999)).then(now);
}

function pumpTo() {
  const v = parseInt(document.getElementById('p-target').value) || 0;
  // state >= 5000 → setValue(state * 0.1), so multiply by 10
  api('goto/' + Math.min(Math.max(v * 10, 0), 49999)).then(now);
}

// digitalWrite test — no PWM, just HIGH/LOW to check hardware
async function digTest(n) {
  await fetch('/api/digtest/' + n + '/1');
  setTimeout(() => fetch('/api/digtest/' + n + '/0'), 1000);
}

// Direct single-motor test — bypasses all mode/flag logic
// Runs motor at current strength for 800ms, then stops
async function testMotor(n) {
  const duty = parseInt(document.getElementById('vib-s').value) || 200;
  const r = await fetch('/api/motortest/' + n + '/' + duty);
  const txt = await r.text();
  console.log('[motortest]', txt);
  setTimeout(() => fetch('/api/motortest/' + n + '/0'), 800);
}
</script>
</body>
</html>
)***";

// ─────────────────────────────────────────────────────────────────────────────
// Web server task
// ─────────────────────────────────────────────────────────────────────────────
void TaskWebserver(void *pvParameters) {
    ArduinoOTA.setHostname(HOSTNAME);
    ArduinoOTA.begin();

    // ── Static dashboard ────────────────────────────────────────────────────
    server.on("/", []() {
        server.send_P(200, "text/html", DASHBOARD_HTML);
    });

    // ── JSON status endpoint ─────────────────────────────────────────────────
    server.on("/api/status", []() {
        // Static buffer lives in BSS — not on the stack, no heap alloc each call.
        // Size: ~400 bytes status + 120 samples * ~22 bytes = ~3040 → use 4096.
        static char buf[4096];

        int m0 = ledcRead(VIBRATOR_PINS[0]), m1 = ledcRead(VIBRATOR_PINS[1]), m2 = ledcRead(VIBRATOR_PINS[2]);
        int m3 = ledcRead(VIBRATOR_PINS[3]), m4 = ledcRead(VIBRATOR_PINS[4]), m5 = ledcRead(VIBRATOR_PINS[5]);

        char vlbl[24];
        int s = state;
        if      (s == 0) strncpy(vlbl, "Pump Front",   sizeof(vlbl));
        else if (s == 1) strncpy(vlbl, "Pump Back",    sizeof(vlbl));
        else if (s == 2) strncpy(vlbl, "Even Out",     sizeof(vlbl));
        else if (s == 3) strncpy(vlbl, "Pause",        sizeof(vlbl));
        else if (s == 4) strncpy(vlbl, "Open",         sizeof(vlbl));
        else if (s == 5) strncpy(vlbl, "Even Out Inf", sizeof(vlbl));
        else if (s == 6) strncpy(vlbl, "Calibrating",  sizeof(vlbl));
        else if (s < 5000) snprintf(vlbl, sizeof(vlbl), "Hold @ %d", s);
        else               snprintf(vlbl, sizeof(vlbl), "Pump -> %d", (int)(s * 0.1f));

        // ── Status header ────────────────────────────────────────────────────
        int pos = snprintf(buf, sizeof(buf),
            "{"
            "\"pH\":%d,\"pV\":%d,\"bH\":%d,\"bV\":%d,"
            "\"prH\":%d,\"prV\":%d,"
            "\"ax\":%d,\"ay\":%d,\"az\":%d,"
            "\"gx\":%d,\"gy\":%d,\"gz\":%d,"
            "\"step\":\"%s\","
            "\"vH\":%d,\"vC\":%d,\"vF\":%d,"
            "\"vib\":%s,\"vMode\":%d,"
            "\"m\":[%d,%d,%d,%d,%d,%d],"
            "\"state\":%d,\"valveLabel\":\"%s\","
            "\"heap\":%u,"
            "\"buf\":[",
            sensorValueH, sensorValueV,
            (int)baselineH, (int)baselineV,
            (int)(sensorValueH - baselineH),
            (int)(sensorValueV - baselineV),
            (int)accelX, (int)accelY, (int)accelZ,
            (int)gyroX,  (int)gyroY,  (int)gyroZ,
            stepStateStr(),
            digitalRead(zuluftHPin),
            digitalRead(verbinderPin),
            digitalRead(zuluftVPin),
            vibrate ? "true" : "false",
            vMode,
            m0, m1, m2, m3, m4, m5,
            state, vlbl,
            (unsigned)ESP.getFreeHeap()
        );

        // ── Ring buffer samples — snapshot fill count to avoid race ──────────
        int fill = chartBufFill;   // read once; TaskLoop may keep writing
        for (int i = 0; i < fill && pos < (int)sizeof(buf) - 32; i++) {
            ChartSample cs = chartBufGet(i);
            pos += snprintf(buf + pos, sizeof(buf) - pos,
                "%s[%d,%d,%d,%d,%d]",
                i > 0 ? "," : "",
                (int)cs.prH, (int)cs.prV,
                (int)cs.ax,  (int)cs.ay, (int)cs.az
            );
        }

        // ── Close JSON ───────────────────────────────────────────────────────
        snprintf(buf + pos, sizeof(buf) - pos, "]}");

        // Mark buffer as consumed — next poll only gets new samples.
        // Safe on single-core (Core 0): loop task is suspended while we run.
        chartBufFill = 0;

        server.sendHeader("Cache-Control", "no-store");
        server.send(200, "application/json", buf);
    });

    // ── Surface presets ──────────────────────────────────────────────────────
    server.on("/api/surface/asphalt",  []() { vibrate=true;  vMode=4; state=38000; server.send(200,"text/plain","OK"); });
    server.on("/api/surface/grass",    []() { vibrate=true;  vMode=6; state=5;     server.send(200,"text/plain","OK"); });
    server.on("/api/surface/sand",     []() { vibrate=true;  vMode=7; state=5;     server.send(200,"text/plain","OK"); });
    server.on("/api/surface/linoleum", []() { vibrate=false; vMode=0; state=32000; server.send(200,"text/plain","OK"); });
    server.on("/api/surface/gravel",   []() { vibrate=true;  vMode=5; state=34000; server.send(200,"text/plain","OK"); });

    // ── Pressure control ─────────────────────────────────────────────────────
    // Go & Hold: state < 5000 → holdValue(state) keeps pressure at target
    server.on(UriRegex("^\\/api\\/hold\\/([0-9]+)$"), []() {
        int v = server.pathArg(0).toInt();
        state = constrain(v, 0, 4999);
        server.send(200, "text/plain", "OK");
    });

    // Pump then release: state >= 5000 → setValue(state*0.1) then auto even-out
    server.on(UriRegex("^\\/api\\/goto\\/([0-9]+)$"), []() {
        int v = server.pathArg(0).toInt();
        state = constrain(v, 5000, 49990);   // caller sends value*10
        server.send(200, "text/plain", "OK");
    });

    server.on("/api/evenout",   []() { state = 2; server.send(200,"text/plain","OK"); });
    server.on("/api/pause",     []() { state = 3; server.send(200,"text/plain","OK"); });
    server.on("/api/calibrate", []() { state = 6; server.send(200,"text/plain","OK"); });

    // ── Vibration ────────────────────────────────────────────────────────────
    server.on("/api/vibrate", []() {
        vibrate = !vibrate;
        server.send(200, "text/plain", vibrate ? "ON" : "OFF");
    });

    // Set vibration mode directly (0=off, 10=continuous — useful for testing)
    server.on(UriRegex("^\\/api\\/vibmode\\/([0-9]+)$"), []() {
        vMode = constrain(server.pathArg(0).toInt(), 0, 10);
        vibrate = (vMode != 0);
        server.send(200, "text/plain", "OK");
    });

    server.on(UriRegex("^\\/api\\/vibstrength\\/([0-9]+)$"), []() {
        vibrationStrength = constrain(server.pathArg(0).toInt(), 0, 255);
        server.send(200, "text/plain", "OK");
    });

    // ── Direct motor test (bypasses vibrate flag and mode logic) ────────────
    // GET /api/motortest/{0-5}/{0-255}  — sets one motor via LEDC
    server.on(UriRegex("^\\/api\\/motortest\\/([0-5])\\/([0-9]+)$"), []() {
        int motor = server.pathArg(0).toInt();
        int duty  = constrain(server.pathArg(1).toInt(), 0, 255);
        int pin   = VIBRATOR_PINS[motor];
        ledcWrite(pin, duty);
        uint32_t rb = ledcRead(pin);
        char msg[64];
        snprintf(msg, sizeof(msg), "Motor %d GPIO %d duty=%d readback=%u", motor, pin, duty, rb);
        Serial.println(msg);
        server.send(200, "text/plain", msg);
    });

    // ── digitalWrite test — no LEDC, just pin HIGH/LOW to test hardware ──────
    // GET /api/digtest/{0-5}/{0or1}
    server.on(UriRegex("^\\/api\\/digtest\\/([0-5])\\/([01])$"), []() {
        int motor = server.pathArg(0).toInt();
        int level = server.pathArg(1).toInt();
        int pin   = VIBRATOR_PINS[motor];
        pinMode(pin, OUTPUT);
        digitalWrite(pin, level ? HIGH : LOW);
        char msg[48];
        snprintf(msg, sizeof(msg), "GPIO %d → %s", pin, level ? "HIGH (motor ON)" : "LOW (motor OFF)");
        Serial.println(msg);
        server.send(200, "text/plain", msg);
    });

    // ── Legacy endpoints (backward compat with original firmware) ────────────
    server.on("/presure", []() {
        server.send(200, "text/plain",
            "Front:" + String(sensorValueV) + " Back:" + String(sensorValueH));
    });
    server.on("/restart", []() {
        server.send(200, "text/plain", "Restarting...");
        vTaskDelay(200 / portTICK_PERIOD_MS);
        ESP.restart();
    });

    server.begin();

    for (;;) {
        server.handleClient();
        ArduinoOTA.handle();
        vTaskDelay(clientDelay / portTICK_PERIOD_MS);
    }
}
