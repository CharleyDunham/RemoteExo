class WSClient {
    constructor(url) {
        this.ws = new WebSocket(url);
        this.handlers = {};        // event‑type → [fn…]
        this.ws.onmessage = evt => this._onMessage(evt);
    }
    on(dev, fn) {
        (this.handlers[dev] ||= []).push(fn);
    }
    _onMessage(evt) {
        let msg;
        console.log(evt.data);
        try {
            msg = JSON.parse(evt.data);
        } catch (e) {
            console.warn("Bad JSON:", evt.data);
            return;
        }
        if (msg.dev && this.handlers[msg.dev]) {
            this.handlers[msg.dev].forEach(fn => fn(msg));
            return;
        }
        console.debug("reply:", msg);
    }
    sendCommand(dev, req, attr, val) {
        console.log(`Sending command: 
        {
            dev: "${dev}",
            req: "${req}",
            attr: "${attr}",
            val: ${val}
        };
        `);
        const out = {dev, req, attr};
        if (val !== undefined) out.val = val;
        this.ws.send(JSON.stringify(out));
    }
}

document.addEventListener("DOMContentLoaded", () => {
    const ws = new WSClient(`ws://${location.host}/ws`);
    const graph = new Graph();
    new ServoUI(ws);
    new FlexUI(ws, graph);
});
class Graph {
    #canvas = document.getElementById('flex-sensor-graph');
    #context = this.#canvas.getContext('2d');
    #timeWindowMs = 5000;
    #padding = 50;
    #graphWidth = this.#canvas.width - 2 * this.#padding;
    #graphHeight = this.#canvas.height - 2 * this.#padding;
    #sensorData = [[], [], [], []];
    #colors = ['blue', 'red', 'green', 'orange']; // Colors for each sensor line
    #sensorLabels = ['Sensor 1', 'Sensor 2', 'Sensor 3', 'Sensor 4'];
    constructor() {
        this.drawGraph();
    }
    drawGraph() {
        const ctx = this.#context;
        const canvas = this.#canvas;
        ctx.clearRect(0, 0, canvas.width, canvas.height);
        ctx.fillStyle = '#222';
        ctx.fillRect(this.#padding, this.#padding, this.#graphWidth, this.#graphHeight);
        ctx.strokeStyle = '#444';
        ctx.lineWidth = 0.5;
        for (let i = 0; i <= 5; i++) {
            const y = this.#padding + (i * this.#graphHeight / 5);
            ctx.beginPath();
            ctx.moveTo(this.#padding, y);
            ctx.lineTo(this.#padding + this.#graphWidth, y);
            ctx.stroke();
        }
        for (let i = 0; i <= 10; i++) {
            const x = this.#padding + (i * this.#graphWidth / 10);
            ctx.beginPath();
            ctx.moveTo(x, this.#padding);
            ctx.lineTo(x, this.#padding + this.#graphHeight);
            ctx.stroke();
        }
        ctx.beginPath();
        ctx.strokeStyle = '#fff';
        ctx.lineWidth = 2;
        ctx.moveTo(this.#padding, this.#padding);
        ctx.lineTo(this.#padding, canvas.height - this.#padding);
        ctx.lineTo(canvas.width - this.#padding, canvas.height - this.#padding);
        ctx.stroke();
        let earliestTime = Infinity;
        for (const sensorPoints of this.#sensorData) {
            if (sensorPoints.length > 0) {
                earliestTime = Math.min(earliestTime, sensorPoints[0].time);
            }
        }
        if (earliestTime === Infinity) return;
        const now = performance.now();
        const timeRange = now - earliestTime || 1;
        for (let sensorIndex = 0; sensorIndex < 4; sensorIndex++) {
            const points = this.#sensorData[sensorIndex];
            if (points.length < 2) continue;
            ctx.beginPath();
            ctx.strokeStyle = this.#colors[sensorIndex];
            ctx.lineWidth = 2;
            points.forEach((pt, i) => {
                const x = this.#padding + ((pt.time - earliestTime) / timeRange) * this.#graphWidth;
                const y = canvas.height - this.#padding - (pt.value * this.#graphHeight / 4096);
                if (i === 0) ctx.moveTo(x, y);
                else ctx.lineTo(x, y);
            });
            ctx.stroke();
        }
        this.drawLegend();
    }
    drawLegend() {
        const ctx = this.#context;
        const legendX = this.#padding + 10;
        const legendY = this.#padding + 20;
        const lineLength = 20;
        const lineSpacing = 20;
        ctx.font = '10px monospace';
        ctx.textBaseline = 'middle';
        for (let i = 0; i < 4; i++) {
            if (this.#sensorData[i].length > 0) {
                ctx.strokeStyle = this.#colors[i];
                ctx.lineWidth = 2;
                ctx.beginPath();
                ctx.moveTo(legendX, legendY + i * lineSpacing);
                ctx.lineTo(legendX + lineLength, legendY + i * lineSpacing);
                ctx.stroke();
                ctx.fillStyle = '#fff';
                ctx.fillText(this.#sensorLabels[i], legendX + lineLength + 5, legendY + i * lineSpacing);
            }
        }
    }
    onNewPoint(sensorIndex, adcReading) {
        sensorIndex -= 2;
        const now = performance.now();
        if (sensorIndex >= 0 && sensorIndex < 4) {
            this.#sensorData[sensorIndex].push({value: adcReading, time: now});
            // Remove points older than the time window
            this.#sensorData[sensorIndex] = this.#sensorData[sensorIndex].filter(
                pt => now - pt.time <= this.#timeWindowMs
            );
        }
    }
}
class ServoUI {
    constructor(ws) {
        this.ws = ws;
        this.el = {
            angleStep: document.getElementById("SERVO ANGLE_STEP"),
            timeDelay: document.getElementById("SERVO TIME_DELAY"),
            position: document.getElementById("SERVO POSITION"),
            startAngle: document.getElementById("SERVO START_ANGLE"),
            stopAngle: document.getElementById("SERVO STOP_ANGLE"),
            motion: document.getElementById("SERVO MOTION"),
            actuateOn: document.getElementById("SERVO ACTUATE_ON"),
            actuateOff: document.getElementById("SERVO ACTUATE_OFF"),
            pin: document.getElementById("SERVO PIN"),
            minPwm: document.getElementById("SERVO MIN_PWM"),
            maxPwm: document.getElementById("SERVO MAX_PWM"),
            maxAngle: document.getElementById("SERVO MAX_ANGLE")
        };
        this.el.actuateOn.addEventListener('click',evt => {
            ws.sendCommand("SERVO", "SET", "ACTUATE", true);
        });
        this.el.actuateOff.addEventListener('click', evt => {
            ws.sendCommand("SERVO", "SET", "ACTUATE", false);
        });
        this.el.motion.addEventListener("change", evt => {
            ws.sendCommand("SERVO", "SET", "MOTION", evt.target.value);
        });
        this.el.pin.addEventListener("change", evt => {
            ws.sendCommand("SERVO", "SET", "PIN", evt.target.value);
        });
        this.el.minPwm.addEventListener("change", evt => {
            ws.sendCommand("SERVO", "SET", "MIN_PWM", evt.target.value);
        });
        this.el.maxPwm.addEventListener("change", evt => {
            ws.sendCommand("SERVO", "SET", "MAX_PWM", evt.target.value);
        });
        this.el.maxAngle.addEventListener("change", evt => {
            ws.sendCommand("SERVO", "SET", "MAX_ANGLE", evt.target.value);
        });
        this.el.startAngle.addEventListener("change", evt => {
            ws.sendCommand("SERVO", "SET", "START_ANGLE", evt.target.value);
        });
        this.el.stopAngle.addEventListener("change", evt => {
            ws.sendCommand("SERVO", "SET", "STOP_ANGLE", evt.target.value);
        });
        this.el.timeDelay.addEventListener("change", evt => {
            ws.sendCommand("SERVO", "SET", "TIME_DELAY", evt.target.value);
        });
        this.el.angleStep.addEventListener("change", evt => {
            ws.sendCommand("SERVO", "SET", "ANGLE_STEP", evt.target.value);
        });
        this.el.position.addEventListener("change", evt => {
            ws.sendCommand("SERVO", "SET", "POSITION", evt.target.value);
        });
        this.ws.on("SERVO", msg => {
            let found = false;
            if (msg.attr == null) {
                return console.error(`Received message doesn't contain an attribute: ${msg}`);
            }
            if (msg.req == null && msg.attr === 'POSITION') {
                this.el.position.valueAsNumber = msg.val;
            }

            if (msg.req === 'GET') {
                for (const element of this.el) {
                    if (element.id === `SERVO ${msg.attr}`) {
                        found = true;
                        if (element.type === 'select-one') {
                            const opt = element.querySelector(`option[value=${msg.val}]`);
                            if (opt) {
                                opt.selected = true;
                            } else {
                                console.warn(`Unknown value for ${msg.attr}: ${msg.val}`);
                            }
                            break;
                        } else if (element.type === 'number') {
                            element.valueAsNumber = msg.val;
                            break;
                        }
                    }
                }
                if (!found) {
                    console.warn(`Unknown attribute: ${msg.attr}`);
                }
            } else if (msg.req === 'SET') {
                if (msg.stat != null) {
                    if (msg.stat === 'OK' && msg.val != null) {
                        console.log(`Server responded with OK to SET request ${msg.attr}`);
                        for (const element of this.el) {
                            if (element.id === `SERVO ${msg.attr}`) {
                                found = true;
                                if (element.type === 'select-one') {
                                    const opt = element.querySelector(`option[value=${msg.val}]`);
                                    if (opt) {
                                        opt.selected = true;
                                        break;
                                    } else {
                                        console.warn(`Unknown value for ${msg.attr}: ${msg.val}`);
                                        break;
                                    }
                                } else if (element.type === 'number') {
                                    element.valueAsNumber = msg.val;
                                    break;
                                } else {
                                    console.warn(`Unknown element type: ${element.type}`);
                                    break;
                                }
                            }
                        }
                        if (!found) {
                            console.warn(`Unknown attribute: ${msg.attr}`);
                        }
                    }
                    else if (msg.stat === 'ERROR' && msg.val != null) {
                        console.warn(`Server responded with ERROR to SET request ${msg.attr}: ${msg.val}`);
                    } else {
                        console.warn(`Server responded with invalid SET response. Missing 'val': ${msg.val}`);
                    }
                } else {
                    console.warn(`Server responded with invalid SET response. Missing 'stat': ${msg}`);
                }
            }
        });
    }
}
class FlexUI {
    constructor(ws, graph) {
        this.ws = ws;
        this.graph = graph;

        this.el = {
            pin2: document.getElementById("FLEX_2 PIN"),
            pin3: document.getElementById("FLEX_3 PIN"),
            pin4: document.getElementById("FLEX_4 PIN"),
            pin5: document.getElementById("FLEX_5 PIN"),
            reading2: document.getElementById("FLEX_2 READ"),
            reading3: document.getElementById("FLEX_3 READ"),
            reading4: document.getElementById("FLEX_4 READ"),
            reading5: document.getElementById("FLEX_5 READ"),
            fixed2: document.getElementById("FLEX_2 FIXED_RESIST"),
            fixed3: document.getElementById("FLEX_3 FIXED_RESIST"),
            fixed4: document.getElementById("FLEX_4 FIXED_RESIST"),
            fixed5: document.getElementById("FLEX_5 FIXED_RESIST"),
            volt2: document.getElementById("FLEX_2 VOLT"),
            volt3: document.getElementById("FLEX_3 VOLT"),
            volt4: document.getElementById("FLEX_4 VOLT"),
            volt5: document.getElementById("FLEX_5 VOLT"),
            resist2: document.getElementById("FLEX_2 RESIST"),
            resist3: document.getElementById("FLEX_3 RESIST"),
            resist4: document.getElementById("FLEX_4 RESIST"),
            resist5: document.getElementById("FLEX_5 RESIST"),
            start: document.getElementById('FLEX START'),
            stop: document.getElementById('FLEX STOP')
        };
        this.ws.on("FLEX_2", msg => {
            if (msg.attr === "READ") {
                this.el.reading2.textContent = msg.val;
                this.graph.onNewPoint(2, msg.val);
                this.graph.drawGraph();
            }
            else if (msg.attr === 'PIN') {
                const opt = this.el.pin2.querySelector(`option[value=${msg.val}]`);
                if (opt) opt.selected = true;
                else console.warn(`Unknown pin: ${msg.val}`);
            } else {
                console.warn(`Unknown attribute: ${msg.attr}`);
            }
        });
        this.ws.on("FLEX_3", msg => {
            if (msg.attr === "READ") {
                this.el.reading3.textContent = msg.val;
                this.graph.onNewPoint(3, msg.val);
                this.graph.drawGraph();
            } else if (msg.attr === 'PIN') {
                const opt = this.el.pin3.querySelector(`option[value=${msg.val}]`);
                if (opt) opt.selected = true;
                else console.warn(`Unknown pin: ${msg.val}`);
            } else {
                console.warn(`Unknown attribute: ${msg.attr}`);
            }
        });
        this.ws.on("FLEX_4", msg => {
            if (msg.attr === "READ") {
                this.el.reading4.textContent = msg.val;
                this.graph.onNewPoint(4, msg.val);
                this.graph.drawGraph();
            } else if (msg.attr === 'PIN') {
                const opt = this.el.pin4.querySelector(`option[value=${msg.val}]`);
                if (opt) opt.selected = true;
                else console.warn(`Unknown pin: ${msg.val}`);
            } else {
                console.warn(`Unknown attribute: ${msg.attr}`);
            }
        });
        this.ws.on("FLEX_5", msg => {
            if (msg.attr === "READ") {
                this.el.reading5.textContent = msg.val;
                this.graph.onNewPoint(5, msg.val);
                this.graph.drawGraph();
            } else if (msg.attr === 'PIN') {
                const opt = this.el.pin4.querySelector(`option[value=${msg.val}]`);
                if (opt) opt.selected = true;
                else console.warn(`Unknown pin: ${msg.val}`);
            } else {
                console.warn(`Unknown attribute: ${msg.attr}`);
            }
        });
        this.el.pin2.addEventListener('change', evt => {
            this.ws.sendCommand('FLEX_2', 'SET', "PIN", evt.target.value);
        });
        this.el.pin3.addEventListener('change', evt => {
            this.ws.sendCommand('FLEX_3', 'SET', 'PIN', evt.target.value);
        });
        this.el.pin4.addEventListener('change', evt => {
            this.ws.sendCommand('FLEX_4', 'SET', 'PIN', evt.target.value);
        });
        this.el.pin5.addEventListener('change', evt => {
            this.ws.sendCommand('FLEX_5', 'SET', 'PIN', evt.target.value);
        });
        this.el.start.addEventListener('click', evt => {
            this.ws.sendCommand('FLEX', 'SET', 'START');
        });
        this.el.stop.addEventListener('click', evt => {
            this.ws.sendCommand('FLEX', 'SET', 'STOP');
        });
    }
    pinToOption(pinValue) {
        if (pinValue.NaN) throw new Error(`Invalid pin number ${pinValue}`);
        if (pinValue <= 13) {
            return `D${pinValue}`;
        } else if (17 <= pinValue && pinValue <= 24) {
            return `A${pinValue}`;
        }
        throw new Error(`Pin ${pinValue} not an ADC pin.`);
    }
}