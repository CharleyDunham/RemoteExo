//
// Created by Sullivan Bryant on 3/31/25.
//

#ifndef UNTITLED_HTML_PAGE_H
#define UNTITLED_HTML_PAGE_H

#include <pgmspace.h>
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Device Panel</title>
</head>
<body>
<h1>Device Panel</h1>
<h2>Servo Actuation</h2>
<div class="servo-actuation">
    <button id="startServo" onclick="startServo()">Start Servo</button>
    <button id="stopServo" disabled onclick="stopServo()">Stop Servo</button>
    <br/>
    <div>
        <label for="servo-motion">Servo Motion</label>
        <select name="setMotion" id="servo-motion">
            <option value="ONE_SHOT_CW">One-shot CW</option>
            <option value="ONE_SHOT_CCW">One-shot CCW</option>
            <option value="ONE_WAY_CW">One-way CW</option>
            <option value="ONE_WAY_CCW">One-way CCW</option>
            <option value="SWEEP">Sweep</option>
        </select>
        <br/>

        <label for="servo-time-delay">Time-delay</label>
        <input type="number" name="setTimeDelayUS" id="servo-time-delay"> (µs) <br/>
        <label for="servo-angle-step">Angle-step </label>
        <input type="number" name="setAngleStep" id="servo-angle-step"> (º) <br/>
        <label for="servo-start-angle">Start angle: </label>
        <input type="number" name="setStartAngle" id="servo-start-angle"> (º) <br/>
        <label for="servo-stop-angle">Stop angle: </label>
        <input type="number" name="setStopAngle" id="servo-stop-angle"> (ª) <br/>
        <label for="servo-pin">Servo pin</label>
        <select name="setPin" id="servo-pin">
            <option value="0">D0</option>
            <option value="1">D1</option>
            <option value="2">D2</option>
            <option value="3">D3</option>
            <option value="4">D4</option>
            <option value="5">D5</option>
            <option value="6">D6</option>
            <option value="7">D7</option>
            <option value="8">D8</option>
            <option value="9">D9</option>
            <option value="10">D10</option>
            <option value="11">D11</option>
            <option value="12">D12</option>
            <option value="13">D13</option>
        </select><br/>
        <label for="servo-pwm-min">PWM Min</label>
        <input type="number" name="setMinPWM" id="servo-pwm-min"> (µs) <br/>
        <label for="servo-pwm-max">PWM Max</label>
        <input type="number" name="setMaxPWM" id="servo-pwm-max"> (µs) <br/>
        <label for="servo-max-range">Max range: </label>
        <input type="number" name="setMaxAngle" id="servo-max-range"> (º) <br/>
        <label for="servo-frequency">Frequency: </label>
        <input type="number" name="setFrequency" id="servo-frequency"> (Hz) <br/>
        <label for="servo-resolution">Resolution</label>
        <select id="servo-resolution" name="setResolution">
            <option value="8">8-bit</option>
            <option value="10">10-bit</option>
            <option value="12">12-bit</option>
            <option value="16">16-bit</option>
        </select> <br/><br/>
        <button id='submit-servo' onclick="submitServo()">Submit</button>
    </div>
    <div>
        <!-- Current values for display. -->
        <h3>Current Configuration</h3>
        <div class="config-label">Voltage <span id="voltage-a1"></span>V</div>
        <div class="config-label">Position: <span id="servo-current-position"> </span>º</div>
        <div class="config-label">Motion status: <span id="servo-current-motion-status"></span></div>
        <div class="config-label">Motion: <span id="servo-current-motion">__</span></div>
        <div class="config-label">Time-delay: <span id="servo-current-time-delay"></span> ms</div>
        <div class="config-label">Angle-step: <span id="servo-current-angle-step"></span>º</div>
        <div class="config-label">Start angle: <span id="servo-current-start-angle"></span>º</div>
        <div class="config-label">Stop angle: <span id="servo-current-stop-angle"></span>º</div>
        <div class="config-label">Pin: <span id="servo-current-pin"></span></div>
        <div class="config-label">PWM min: <span id="servo-current-pwm-min"></span> µs</div>
        <div class="config-label">PWM max: <span id="servo-current-pwm-max"></span> µs</div>
        <div class="config-label">Max range: <span id="servo-current-max-range"></span>º</div>
        <div class="config-label">Frequency: <span id="servo-current-freq"></span> Hz</div>
        <div class="config-label">Resolution: <span id="servo-current-res"></span> bits</div>
        <div class="config-label">Max ticks: <span id="servo-current-max-ticks"></span></div>
    </div>
</div>
<div id="developer">
    <h2>Developer</h2>
    <button id="startWS" onclick="startWS()">Start WebSocket</button>
</div>
</body>
<style>
    body {
        background-color: #4a4a4a;
    }

    h1 {
        color: white;
        font-family: monospace;
    }

    label {
        color: white;
        font-family: monospace;
    }

    .servo-actuation {
        color: white;
        font-family: monospace;
    }

    button {
        font-family: monospace;
    }

    input {
        font-family: monospace;
    }

    h2 {
        color: white;
        font-family: monospace;
    }
</style>
<script>


    let socket;

    const elements = {
        user: {},
        current: {},
        buttons: {}
    };

    function refreshDomElements() {
        /* User-set inputs. */
        elements.user.angleStep   = document.getElementById('servo-angle-step');
        elements.user.minPWM      = document.getElementById('servo-pwm-min');
        elements.user.maxPWM      = document.getElementById('servo-pwm-max');
        elements.user.freq        = document.getElementById('servo-frequency');
        elements.user.res         = document.getElementById('servo-resolution');
        elements.user.startAngle  = document.getElementById('servo-start-angle');
        elements.user.stopAngle   = document.getElementById('servo-stop-angle');
        elements.user.timeDelay   = document.getElementById('servo-time-delay');
        elements.user.motion      = document.getElementById('servo-motion');
        elements.user.pin         = document.getElementById('servo-pin');

        /* Current values for display. */
        elements.current.angleStep   = document.getElementById('servo-current-angle-step');
        elements.current.minPWM      = document.getElementById('servo-current-pwm-min');
        elements.current.maxPWM      = document.getElementById('servo-current-pwm-max');
        elements.current.freq        = document.getElementById('servo-current-freq');
        elements.current.res         = document.getElementById('servo-current-res');
        elements.current.startAngle  = document.getElementById('servo-current-start-angle');
        elements.current.stopAngle   = document.getElementById('servo-current-stop-angle');
        elements.current.timeDelay   = document.getElementById('servo-current-time-delay');
        elements.current.motion      = document.getElementById('servo-current-motion');
        elements.current.motionStatus= document.getElementById('servo-current-motion-status');
        elements.current.pin         = document.getElementById('servo-current-pin');
    }

    document.addEventListener('DOMContentLoaded', () => {
        refreshDomElements();
        elements.buttons.submit = document.getElementById('submit-servo');
        elements.buttons.start  = document.getElementById('startServo');
        elements.buttons.stop   = document.getElementById('stopServo');

        elements.buttons.submit.disabled = true;
    });

    /* Servo control functions. */
    function startServo() {
        fetch('/currentConfig'
        ).then(response => response.ok ? response.json() : Promise.reject(response.status
            )).then(data => {
                if ('request' in data) {
                    if ('response' in data) {
                        if ('angleStep' in data.response) currentAS.textContent = data.angleStep;
                        if ('timeDelayUS' in data.response) currentTD.textContent = data.timeDelay;
                        if ('startAngle' in data.response) currentStartAngle.textContent = data.startAngle;
                        if ('stopAngle' in data.response) currentStopAngle.textContent = data.stopAngle;
                        if ('motion' in data.response) currentMotion.textContent = data.motion;
                        if ('motionStatus' in data.response) currentMotionStatus.textContent = data.motionStatus;
                        if ('pin' in data.response) currentPin.textContent = data.pin;
                        if ('frequency' in data.response) currentFreq.textContent = data.frequency;
                        if ('resolution' in data.response) currentRes.textContent = data.resolution;
                        if ('position' in data.response) currentPosition.textContent = data.position;
                        if ('maxPWM' in data.response) currentMaxPWM.textContent = data.maxPWM;
                        if ('minPWM' in data.response) currentMinPWM.textContent = data.minPWM;
                    }
                }
            }).catch(error => {
                console.error(error);
        });
        fetch("/startServo"
        ).then(response => {
                if (response.ok) {
                    console.log(response.body);
                    elements.buttons.start.disabled = true;
                    elements.buttons.stop.disabled  = false;
                } else {
                    console.error("Failed to retrieve response from server.");
                    if (socket) socket.close();
                    elements.buttons.start.disabled = false;
                    elements.buttons.stop.disabled  = true;
                }
            }).catch(error => console.error(error));
    }

    function stopServo() {
        fetch("/stopServo"
        ).then(response => {
                if (response.ok) {
                    console.log(response.body);
                    elements.buttons.start.disabled = false;
                    elements.buttons.stop.disabled  = true;
                } else {
                    console.error("Failed to retrieve response from server.");
                    if (socket) socket.close();
                }
            }).catch(error => console.error(error));
    }

    const configRequestJSON = {
        device: "SERVO",
        request: { sendConfig: "" }
    };

    /* WebSocket functions. */
    function startWS() {
        socket = new WebSocket('ws://' + location.host + "/ws");
        elements.buttons.submit.disabled = false;

        socket.onopen = (e) => {
            console.log(`WS opened on port ${socket.port}`);
            socket.send(`Client connected to port ${socket.port}`);
            socket.send(JSON.stringify(configRequestJSON));
            elements.buttons.submit.disabled = false;
            socket.keepalive = true;
        };

        socket.onclose = (e) => {
            console.log(`Client closed: ${e.toString()}`);
        };

        socket.onmessage = (message) => {
            // Handle voltage message separately
            if (message.data.startsWith("voltage: ")) {
                document.getElementById('voltage-a1').innerHTML = message.data.substring(9);
                return;
            }

            let data;
            try {
                data = JSON.parse(message.data);
            } catch (err) {
                console.error("Error parsing message data", message.data);
                return;
            }

            refreshDomElements();

            /* Update current configuration (if available). */
            if ('currentConfiguration' in data) {
                if ('currentAngleStep' in data) {
                    elements.current.angleStep.textContent = data.currentAngleStep;
                }
                if ('currentTimeDelay' in data) {
                    elements.current.timeDelay.textContent = data.currentTimeDelay;
                }
                if ('currentFrequency' in data) {
                    elements.current.freq.textContent = data.currentFrequency;
                }
                if ('currentResolution' in data) {
                    elements.current.res.textContent = data.currentResolution;
                }
                if ('currentStartAngle' in data) {
                    elements.current.startAngle.textContent = data.currentStartAngle;
                }
                if ('currentStopAngle' in data) {
                    elements.current.stopAngle.textContent = data.currentStopAngle;
                }
                if ('currentMinPWM' in data) {
                    elements.current.minPWM.textContent = data.currentMinPWM;
                }
                if ('currentMaxPWM' in data) {
                    elements.current.maxPWM.textContent = data.currentMaxPWM;
                }
                if ('currentPin' in data) {
                    elements.current.pin.textContent = data.currentPin;
                }
                if ('currentMotion' in data) {
                    elements.current.motion.textContent = data.currentMotion;
                }
                if ('currentMotionStatus' in data) {
                    elements.current.motionStatus.textContent = data.currentMotionStatus;
                    if (data.currentMotionStatus === "Active") {
                        elements.buttons.start.disabled = true;
                        elements.buttons.stop.disabled  = false;
                    } else if (data.currentMotionStatus === "Inactive") {
                        elements.buttons.start.disabled = false;
                        elements.buttons.stop.disabled  = true;
                    }
                }
            }
        };
    }

    /* Submission to servo configurations. */
    function submitServo() {
        refreshDomElements();

        /* Avoid stacking by overwriting previous callback. */
        socket.onmessage = (event) => {
            if (event.data.startsWith("Invalid")) {
                console.log(`Arduino error:\n${event.data}`);
                return;
            }
            let msg;
            try {
                msg = JSON.parse(event.data);
            } catch (err) {
                console.error("Error parsing response", event.data);
                return;
            }
            if (msg.device === "SERVO") {
                if (msg.status && msg.request_made) {
                    console.log(`Request made: ${msg.request_made}, Status: ${msg.status}`);
                } else {
                    console.log(`Invalid format received: ${event.data}`);
                }
            } else {
                console.log("Invalid format received: " + event.data);
            }
        };

        const settings = { device: "SERVO", request: {} };

        /* Helper for changing settings. */
        const addSetting = (userEl, currentEl, key) => {
            const userVal    = userEl.value.trim();
            const currentVal = currentEl.textContent.trim();
            if (userVal !== "" && userVal !== currentVal) {
                if (key === 'setPin') {
                    userVal.concat("D", userEl.value.trim());
                }
                settings.request[key] = userVal;
            }
        };

        addSetting(elements.user.angleStep,  elements.current.angleStep,  'setAngleStep');
        addSetting(elements.user.minPWM,     elements.current.minPWM,     'setMinPWM');
        addSetting(elements.user.maxPWM,     elements.current.maxPWM,     'setMaxPWM');
        addSetting(elements.user.freq,       elements.current.freq,       'setFreq');
        addSetting(elements.user.startAngle, elements.current.startAngle, 'setStartAngle');
        addSetting(elements.user.stopAngle,  elements.current.stopAngle,  'setStopAngle');
        addSetting(elements.user.timeDelay,  elements.current.timeDelay,  'setTimeDelayUS');
        addSetting(elements.user.motion,     elements.current.motion,     'setMotion');
        addSetting(elements.user.pin,        elements.current.pin,        'setPin');

        socket.send(JSON.stringify(settings));
    }

</script>
</html>
)rawliteral";

#endif //UNTITLED_HTML_PAGE_H
