//
//
// class WSClient {
//     constructor(url) {
//         //this.ws = new WSClient(url);
//         this.ws = new WebSocket(url);
//         this.handlers = {};        // event‑type → [fn…]
//         this.ws.onmessage = evt => this._onMessage(evt);
//     }
//     on(type, fn) {
//         (this.handlers[type] ||= []).push(fn);
//
//     }
//     _onMessage(evt) {
//         let msg;
//         console.log(evt.data);
//         try {
//             msg = JSON.parse(evt.data);
//         } catch (e) {
//             console.warn("Bad JSON:", evt.data);
//             return;
//         }
//         if (msg.type && this.handlers[msg.type]) {
//             this.handlers[msg.type].forEach(fn => fn(msg));
//             return;
//         }
//         if (!msg.type && msg.dev) {
//             const dev = msg.dev.toLowerCase();
//             if (this.handlers[dev]) {
//                 return this.handlers[dev].forEach(fn => fn(msg));
//             }
//         }
//         console.debug("reply:", msg);
//     }
//     sendCommand(dev, req, attr, val) {
//         const out = {dev, req, attr};
//         if (val !== undefined) out.val = val;
//         this.ws.send(JSON.stringify(out));
//     }
//     getServoPosition() {
//         this.sendCommand("SERVO", "GET", "POSITION");
//     }
// }
// class Graph {
//     #canvas = document.getElementById('flex-sensor-graph');
//     #context = this.#canvas.getContext('2d');
//     #timeWindowMs = 5000;
//     #padding = 50;
//     #graphWidth = this.#canvas.width - 2 * this.#padding;
//     #graphHeight = this.#canvas.height - 2 * this.#padding;
//     #sensorData = [[], [], [], []];
//     #colors = ['blue', 'red', 'green', 'orange']; // Colors for each sensor line
//     #sensorLabels = ['Sensor 1', 'Sensor 2', 'Sensor 3', 'Sensor 4'];
//     constructor() {
//         this.drawGraph();
//     }
//     drawGraph() {
//         const ctx = this.#context;
//         const canvas = this.#canvas;
//         ctx.clearRect(0, 0, canvas.width, canvas.height);
//         ctx.fillStyle = '#222';
//         ctx.fillRect(this.#padding, this.#padding, this.#graphWidth, this.#graphHeight);
//         ctx.strokeStyle = '#444';
//         ctx.lineWidth = 0.5;
//         for (let i = 0; i <= 5; i++) {
//             const y = this.#padding + (i * this.#graphHeight / 5);
//             ctx.beginPath();
//             ctx.moveTo(this.#padding, y);
//             ctx.lineTo(this.#padding + this.#graphWidth, y);
//             ctx.stroke();
//         }
//         for (let i = 0; i <= 10; i++) {
//             const x = this.#padding + (i * this.#graphWidth / 10);
//             ctx.beginPath();
//             ctx.moveTo(x, this.#padding);
//             ctx.lineTo(x, this.#padding + this.#graphHeight);
//             ctx.stroke();
//         }
//         ctx.beginPath();
//         ctx.strokeStyle = '#fff';
//         ctx.lineWidth = 2;
//         ctx.moveTo(this.#padding, this.#padding);
//         ctx.lineTo(this.#padding, canvas.height - this.#padding);
//         ctx.lineTo(canvas.width - this.#padding, canvas.height - this.#padding);
//         ctx.stroke();
//         let earliestTime = Infinity;
//         for (const sensorPoints of this.#sensorData) {
//             if (sensorPoints.length > 0) {
//                 earliestTime = Math.min(earliestTime, sensorPoints[0].time);
//             }
//         }
//         if (earliestTime === Infinity) return;
//         const now = performance.now();
//         const timeRange = now - earliestTime || 1;
//         for (let sensorIndex = 0; sensorIndex < 4; sensorIndex++) {
//             const points = this.#sensorData[sensorIndex];
//             if (points.length < 2) continue;
//             ctx.beginPath();
//             ctx.strokeStyle = this.#colors[sensorIndex];
//             ctx.lineWidth = 2;
//             points.forEach((pt, i) => {
//                 const x = this.#padding + ((pt.time - earliestTime) / timeRange) * this.#graphWidth;
//                 const y = canvas.height - this.#padding - (pt.value * this.#graphHeight / 4096);
//                 if (i === 0) ctx.moveTo(x, y);
//                 else ctx.lineTo(x, y);
//             });
//             ctx.stroke();
//         }
//         this.drawLegend();
//     }
//     drawLegend() {
//         const ctx = this.#context;
//         const legendX = this.#padding + 10;
//         const legendY = this.#padding + 20;
//         const lineLength = 20;
//         const lineSpacing = 20;
//         ctx.font = '10px monospace';
//         ctx.textBaseline = 'middle';
//         for (let i = 0; i < 4; i++) {
//             if (this.#sensorData[i].length > 0) {
//                 ctx.strokeStyle = this.#colors[i];
//                 ctx.lineWidth = 2;
//                 ctx.beginPath();
//                 ctx.moveTo(legendX, legendY + i * lineSpacing);
//                 ctx.lineTo(legendX + lineLength, legendY + i * lineSpacing);
//                 ctx.stroke();
//                 ctx.fillStyle = '#fff';
//                 ctx.fillText(this.#sensorLabels[i], legendX + lineLength + 5, legendY + i * lineSpacing);
//             }
//         }
//     }
//     onNewPoint(sensorIndex, adcReading) {
//         const now = performance.now();
//         if (sensorIndex >= 0 && sensorIndex < 4) {
//             this.#sensorData[sensorIndex].push({value: adcReading, time: now});
//             // Remove points older than the time window
//             this.#sensorData[sensorIndex] = this.#sensorData[sensorIndex].filter(
//                 pt => now - pt.time <= this.#timeWindowMs
//             );
//         }
//     }
// }
// class ServoUI {
//     constructor(ws) {
//         this.ws = ws;
//         this.commands = [
//             { id: 'servo-start',       req: 'SET', attr: 'START',       event: 'click'  },
//             { id: 'servo-stop',        req: 'SET', attr: 'STOP',        event: 'click'  },
//             { id: 'servo-set-position',req: 'SET', attr: 'POSITION',    event: 'change' },
//             { id: 'servo-start-angle', req: 'SET', attr: 'START_ANGLE', event: 'change' },
//             { id: 'servo-stop-angle',  req: 'SET', attr: 'STOP_ANGLE',  event: 'change' },
//             { id: 'servo-motion',      req: 'SET', attr: 'MOTION',      event: 'change' },
//             { id: 'servo-frequency',   req: 'SET', attr: 'FREQUENCY',   event: 'change' },
//             { id: 'servo-resolution',  req: 'SET', attr: 'RESOLUTION',  event: 'change' },
//             { id: 'servo-pwm-min',     req: 'SET', attr: 'MIN_PWM',      event: 'change' },
//             { id: 'servo-pwm-max',     req: 'SET', attr: 'MAX_PWM',      event: 'change' },
//             { id: 'servo-time-delay',  req: 'SET', attr: 'TIME_DELAY',   event: 'change' },
//             { id: 'servo-angle-step',  req: 'SET', attr: 'ANGLE_STEP',   event: 'change' },
//             { id: 'servo-pin',         req: 'SET', attr: 'PIN',          event: 'change' }
//         ];
//
//         this.updaters = {
//             POSITION:    msg => this._update('servo-set-position', 'servo-current-position', msg.val),
//             START_ANGLE: msg => this._update('servo-start-angle',  'servo-current-start-angle', msg.val),
//             STOP_ANGLE:  msg => this._update('servo-stop-angle',   'servo-current-stop-angle',  msg.val),
//             MOTION:      msg => this._update('servo-motion',       'servo-current-motion',      msg.val),
//             FREQUENCY:   msg => this._update('servo-frequency',    'servo-current-freq',        msg.val),
//             RESOLUTION:  msg => this._update('servo-resolution',   'servo-current-res',         msg.val),
//             MIN_PWM:     msg => this._update('servo-min-pwm',      'servo-current-pwm-min',     msg.val),
//             MAX_PWM:     msg => this._update('servo-max-pwm',      'servo-current-pwm-max',     msg.val),
//             TIME_DELAY:  msg => this._update('servo-time-delay',   'servo-current-time-delay',  msg.val),
//             ANGLE_STEP:  msg => this._update('servo-angle-step',   'servo-current-angle-step',  msg.val),
//             PIN:         msg => this._update('servo-pin',          'servo-current-pin',         msg.val)
//         };
//         this._bindCommands();
//         Object.keys(this.updaters).forEach(attr => {
//             this.ws.sendCommand('SERVO', "GET", attr);
//         });
//         this.ws.on('servo', msg => this._handleServo(msg));
//     }
//
//     _bindCommands() {
//         this.commands.forEach(({ id, event, req, attr }) => {
//             const el = document.getElementById(id);
//             if (!el) {
//                 console.warn(`ServoUI: element #${id} not found`);
//                 return;
//             }
//             el.addEventListener(event, () => {
//                 let val;
//                 if (event === 'change') {
//                     val = Number(el.value);
//                     if (Number.isNaN(val)) return;
//                 }
//                 this.ws.sendCommand('SERVO', req, attr, val);
//             });
//         });
//     }
//     _handleServo(msg) {
//         if (typeof msg.angle === 'number') {
//             // broadcast angle → POSITION updater
//             this.updaters.POSITION({ val: msg.angle });
//             return;
//         }
//         if (msg.stat && msg.attr && msg.val != null) {
//             if (msg.stat !== 'OK') {
//                 console.warn(`SET ${msg.attr} failed; re-GET`);
//                 this.ws.sendCommand('SERVO', 'GET', msg.attr);
//             } else {
//                 const fn = this.updaters[msg.attr];
//                 if (fn) fn(msg);
//             }
//         }
//     }
//     _update(inputId, displayId, value) {
//         const inp = document.getElementById(inputId);
//         const disp = document.getElementById(displayId);
//         if (inp)  inp.value = value;
//         if (disp) disp.textContent = value;
//     }
// }
// class FlexUI {
//     constructor(ws, graph) {
//         this.ws = ws;
//         this.graph = graph;
//         this.readEls = [0, 1, 2, 3].map(i => document.getElementById(`reading-${i}`));
//         this.isPaused = false;
//         document.getElementById("start-graph")
//             .addEventListener("click", () => {
//                 this.isPaused = false;
//                 this.ws.sendCommand("FLEX", "SET", "STREAM", true);
//             });
//         document.getElementById("stop-graph")
//             .addEventListener("click", () => {
//                 this.isPaused = true;
//                 this.ws.sendCommand("FLEX", "SET", "STREAM", false);
//             });
//         document.getElementById("pin-0")
//             .addEventListener('change', e => {
//                 const v = Number(e.target.value);
//                 if (!isNaN(v)) {
//                     ws.sendCommand("FLEX-1", "SET", "PIN", v);
//                 }
//             });
//         document.getElementById("pin-1")
//             .addEventListener('change', e => {
//                 const v = Number(e.target.value);
//                 if (!isNaN(v)) {
//                     ws.sendCommand("FLEX-2", "SET", "PIN", v);
//                 }
//             });
//         document.getElementById("pin-2")
//             .addEventListener('change', e => {
//                 const v = Number(e.target.value);
//                 if (!isNaN(v)) {
//                     ws.sendCommand("FLEX-3", "SET", "PIN", v);
//                 }
//             });
//         document.getElementById("pin-3")
//             .addEventListener('change', e => {
//                 const v = Number(e.target.value);
//                 if (!isNaN(v)) {
//                     ws.sendCommand("FLEX-4", "SET", "PIN", v);
//                 }
//             });
//         ws.on("flex", msg => {
//             if (this.isPaused) return;
//             // server sends { type:"flex", values:[…] }
//             if (Array.isArray(msg.values)) {
//                 msg.values.forEach((v, i) => {
//                     this.readEls[i].textContent = v;
//                     this.graph.onNewPoint(i, v);
//                 });
//                 this.graph.drawGraph();
//             }
//         });
//     }
// }
// document.addEventListener("DOMContentLoaded", () => {
//     const ws = new WSClient(`ws://${window.location.host}/ws`);
//     const graph = new Graph();
//     new ServoUI(ws);
//     new FlexUI(ws, graph);
//     ws.ws.addEventListener("open", () => ws.getServoPosition());
// });
class WSClient {
    constructor(url) {
        this.ws = new WebSocket(url);
        this.handlers = {};
        this.ws.addEventListener('message', evt => this._onMessage(evt));
    }
    on(type, fn) {
        if (!this.handlers[type]) this.handlers[type] = [];
        this.handlers[type].push(fn);
    }
    _onMessage(evt) {
        console.log('Received:', evt.data);
        let msg;
        try {
            msg = JSON.parse(evt.data);
        } catch (e) {
            console.warn('Bad JSON:', evt.data);
            return;
        }
        const h = this.handlers[msg.type];
        if (h) {
            h.forEach(fn => fn(msg));
        } else {
            console.debug('Unhandled:', msg);
        }
    }
    sendCommand(dev, req, attr, val) {
        const out = { dev, req, attr };
        if (val !== undefined) out.val = val;
        this.ws.send(JSON.stringify(out));
    }
    getServoPosition() {
        this.sendCommand('SERVO', 'GET', 'POSITION');
    }
}

class Graph {
    constructor(canvasId = 'flex-sensor-graph', timeWindowMs = 5000, padding = 50) {
        this.canvas = document.getElementById(canvasId);
        this.ctx = this.canvas.getContext('2d');
        this.timeWindow = timeWindowMs;
        this.padding = padding;
        this.width = this.canvas.width - 2 * this.padding;
        this.height = this.canvas.height - 2 * this.padding;
        this.sensorData = [[], [], [], []];
        this.colors = ['blue', 'red', 'green', 'orange'];
        this.labels = ['Sensor 1', 'Sensor 2', 'Sensor 3', 'Sensor 4'];
        this.drawAxes();
    }
    drawAxes() {
        const ctx = this.ctx;
        const { canvas, padding, width, height } = this;
        ctx.clearRect(0, 0, canvas.width, canvas.height);
        // background
        ctx.fillStyle = '#222';
        ctx.fillRect(padding, padding, width, height);
        // grid lines
        ctx.strokeStyle = '#444';
        ctx.lineWidth = 0.5;
        for (let i = 0; i <= 5; i++) {
            const y = padding + (i * height / 5);
            ctx.beginPath(); ctx.moveTo(padding, y); ctx.lineTo(padding + width, y); ctx.stroke();
        }
        for (let i = 0; i <= 10; i++) {
            const x = padding + (i * width / 10);
            ctx.beginPath(); ctx.moveTo(x, padding); ctx.lineTo(x, padding + height); ctx.stroke();
        }
        // axes
        ctx.beginPath();
        ctx.strokeStyle = '#fff';
        ctx.lineWidth = 2;
        ctx.moveTo(padding, padding);
        ctx.lineTo(padding, canvas.height - padding);
        ctx.lineTo(canvas.width - padding, canvas.height - padding);
        ctx.stroke();
    }
    drawGraph() {
        this.drawAxes();
        const ctx = this.ctx;
        const now = performance.now();
        let earliest = Infinity;
        this.sensorData.forEach(points => { if (points.length && points[0].time < earliest) earliest = points[0].time; });
        if (earliest === Infinity) return;
        const range = now - earliest;
        this.sensorData.forEach((points, idx) => {
            if (points.length < 2) return;
            ctx.beginPath();
            ctx.strokeStyle = this.colors[idx];
            ctx.lineWidth = 2;
            points.forEach((pt, i) => {
                const x = this.padding + ((pt.time - earliest) / range) * this.width;
                const y = this.canvas.height - this.padding - (pt.value * this.height / 4096);
                i === 0 ? ctx.moveTo(x, y) : ctx.lineTo(x, y);
            });
            ctx.stroke();
        });
        this.drawLegend();
    }
    drawLegend() {
        const ctx = this.ctx;
        const { padding } = this;
        ctx.font = '10px monospace';
        ctx.textBaseline = 'middle';
        this.labels.forEach((lbl, i) => {
            if (this.sensorData[i].length) {
                const y = padding + 20 + i * 20;
                ctx.beginPath();
                ctx.strokeStyle = this.colors[i]; ctx.lineWidth = 2;
                ctx.moveTo(padding + 10, y);
                ctx.lineTo(padding + 30, y);
                ctx.stroke();
                ctx.fillStyle = '#fff';
                ctx.fillText(lbl, padding + 35, y);
            }
        });
    }
    onNewPoint(idx, val) {
        const t = performance.now();
        this.sensorData[idx].push({ value: val, time: t });
        this.sensorData[idx] = this.sensorData[idx].filter(pt => t - pt.time <= this.timeWindow);
    }
}

class ServoUI {
    constructor(ws) {
        this.ws = ws;
        // buttons
        document.getElementById('servo-start')?.addEventListener('click', () => this.ws.sendCommand('SERVO','SET','START'));
        document.getElementById('servo-stop')?.addEventListener('click',  () => this.ws.sendCommand('SERVO','SET','STOP'));
        // inputs mapping
        [
            { id: 'servo-motion',     attr: 'MOTION',     parser: v => v },
            { id: 'servo-time-delay', attr: 'TIME_DELAY', parser: v => Number(v) },
            { id: 'servo-angle-step', attr: 'ANGLE_STEP', parser: v => Number(v) },
            { id: 'servo-start-angle',attr: 'START_ANGLE',parser: v => Number(v) },
            { id: 'servo-stop-angle', attr: 'STOP_ANGLE', parser: v => Number(v) },
            { id: 'servo-pin',        attr: 'PIN',        parser: v => Number(v) },
            { id: 'servo-set-position',attr: 'POSITION',  parser: v => Number(v) }
        ].forEach(({id, attr, parser}) => {
            const el = document.getElementById(id);
            if (!el) return console.warn('ServoUI missing element', id);
            el.addEventListener('change', e => {
                const val = parser(e.target.value);
                if (!Number.isNaN(val)) this.ws.sendCommand('SERVO','SET',attr,val);
            });
        });
        // listen for servo messages
        this.ws.on('servo', msg => this._handleServo(msg));
    }
    _handleServo(msg) {
        if (typeof msg.angle === 'number') {
            document.getElementById('servo-current-position').textContent = msg.angle;
            document.getElementById('servo-set-position').value = msg.angle;
        }
        if (msg.stat === 'OK' && msg.attr && msg.val != null) {
            const map = {
                'MOTION': '#servo-current-motion',
                'TIME_DELAY': '#servo-current-time-delay',
                'ANGLE_STEP': '#servo-current-angle-step',
                'START_ANGLE':'#servo-current-start-angle',
                'STOP_ANGLE': '#servo-current-stop-angle',
                'PIN': '#servo-current-pin',
                'POSITION':'#servo-current-position'
            };
            const sel = map[msg.attr];
            if (sel) document.querySelector(sel).textContent = msg.val;
        }
    }
}

class FlexUI {
    constructor(ws, graph) {
        this.ws = ws;
        this.graph = graph;
        this.isPaused = false;
        this.readEls = [0,1,2,3].map(i => document.getElementById(`reading-${i}`));
        document.getElementById('start-graph')?.addEventListener('click', () => {
            this.isPaused = false;
            this.ws.sendCommand('FLEX','SET','STREAM', true);
        });
        document.getElementById('stop-graph')?.addEventListener('click', () => {
            this.isPaused = true;
            this.ws.sendCommand('FLEX','SET','STREAM', false);
        });
        [0,1,2,3].forEach(i => {
            const sel = document.getElementById(`pin-${i}`);
            sel?.addEventListener('change', e => {
                const v = e.target.value === 'NC' ? false : Number(e.target.value);
                this.ws.sendCommand(`FLEX-${i+1}`,'SET','PIN', v);
            });
        });
        this.ws.on('flex', msg => {
            if (this.isPaused || !Array.isArray(msg.values)) return;
            msg.values.forEach((v,i) => {
                this.readEls[i] && (this.readEls[i].textContent = v);
                this.graph.onNewPoint(i, v);
            });
            this.graph.drawGraph();
        });
    }
}

document.addEventListener('DOMContentLoaded', () => {
    const wsClient = new WSClient(`ws://${location.host}/ws`);
    const graph = new Graph();
    new ServoUI(wsClient);
    new FlexUI(wsClient, graph);
    wsClient.ws.addEventListener('open', () => wsClient.getServoPosition());
});
