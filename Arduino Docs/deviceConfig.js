/**
 * Acquire a command string.
 * @param commandType is the command type (see below)
 * @param params is the command's parameters.
 *
 * Command Types:
 *  'enableServo'      enables servo motor
 *  'disableServo'     disables servo motor
 *  'enableServoStream'     enables servo position stream
 *  'disableServoStream'    disables servo position streaming
 *  'getWiFiStatus'     gets the Wi-Fi status of arduino
 *  'getNetworkStatus'  gets network status of arduino
 *  'setServoPin'       sets the pin of servo on arduino
 *  ---------- below will throw errors for invalid args ----------------
 *  'setServoTimeDelay'      sets the time-delay of the servo (0 - 10,000)
 *  'setServoAngleStep'      sets angle step of servo         (0 - 360)
 *  'setServoMaxRange'       sets max range of servo          (0 - 360)
 *  ---------- will throw an error for invalid command types too --------
 */
const getCommand = (commandType, params = null) =>
{
    switch (commandType)
    {
        case 'enableServo':
            if (params === null) {
                params = true;
            }
            return JSON.stringify({
                "command":
                    {
                        "servo-enable":params
                    }
            });
        case 'disableServo': {
            if (params === null) {
                params = false;
            }
            return JSON.stringify({
                "command":
                    {
                        "servo-enable":params
                    }
            });
        }
        case 'getWiFiStatus':
            return JSON.stringify({
                "request": "wifiStatus"
            });
        case 'getNetworkStatus':
            return JSON.stringify({
                "request": "networkStatus"
            });
        case 'setServoTimeDelay':
            if (0 <= params && params <= 10000) {
                return JSON.stringify({
                    "command":
                        {
                            "servo-time-delay":params
                        }
                });
            } else {
                throw new Error(`invalid time delay parameters: '${params}`);
            }
        case 'setServoAngleStep':
            if (0 <= params && params <= 360) {
                return JSON.stringify({
                    "command":
                        {
                            "servo-angle-step":params
                        }
                });
            } else {
                throw new Error(`invalid angle parameters: '${params}`);
            }
        case 'setServoMaxRange':
            if (0 <= params && params <= 360) {
                return JSON.stringify({
                    "command":
                        {
                            "servo-max-range":params
                        }
                });
            } else {
                throw new Error(`invalid max range parameters: '${params}`);
            }
        case 'enableServoPositionStream':
            if (params === null) {
                params = true;
            }
            return JSON.stringify({
                "command":
                    {
                        "servo-position-stream":params
                    }
            });
        case 'disableServoPositionStream':
            if (params === null) {
                params = false;
            }
            return JSON.stringify({
                "command":
                    {
                        "servo-position-stream":params
                    }
            });
        case 'setServoPin':
            return JSON.stringify({
                "command":
                    {
                        'servo-pin':params
                    }
            });
        default:
            throw new Error(`invalid command requested: ${commandType}`);
    }
}

class DeviceConfig {
    #ws;
    #servoCB;
    #servoStream;
    #servoPinSelector;
    #angleStepDSB;
    #timeDelayDSB;
    #maxRangeDSB;
    #sendTextButton;
    #console;

    /**
     * create a new ws if one wasn't passed.
     */
    constructor(ws = null) {
        this.#ws = ws || new WebSocket(`ws://${window.location.hostname}:${window.location.port}`);
        this.#servoCB = document.getElementById('enableServo');
        this.#console = document.getElementById('consoleLog');
        this.#servoStream = document.getElementById('enableServoStream');
        this.#servoPinSelector = document.getElementById('servoPin');
        this.#angleStepDSB = document.getElementById('servo-angle-step-value');
        this.#timeDelayDSB = document.getElementById('servo-time-delay-value');
        this.#maxRangeDSB = document.getElementById('servo-max-range-value');
        this.#sendTextButton = document.getElementById('sendTextButton');
        this.#populatePins();
        this.#setupEventListeners();
    }
    /**
     * populate servo pin selector
     */
    #populatePins() {
        let pins = [];
        for (let i = 0; i <= 13; i++) {
            pins.push(`D${i}`);
        }
        for (let i = 0; i <= 7; i++) {
            pins.push(`A${i}`);
        }
        this.#servoPinSelector.innerHTML = "";
        pins.forEach(pin => {
            const pinItem = document.createElement('option');
            pinItem.value = pin;
            pinItem.textContent = pin;
            this.#servoPinSelector.appendChild(pinItem);
        });
    }

    /**
     * add anonymous callbacks for DOM elements
     */
    #setupEventListeners() {
        /**
         * TODO: Add event listener for ws message
         *  this.#ws.onMessage('' propagate only if valid header
         */
        this.#ws.addEventListener('message', (event) => {
            let header = JSON.stringify(event.target.value);
            if (header === 'ConsoleMessage') {
                this.#console.innerHTML += `\n${header}`;
            }
        });
        this.#servoCB.addEventListener('change', (event) => this.#sendCommand('enableServo', event.target.checked));
        this.#servoStream.addEventListener('change', (event) => this.#sendCommand('enableStream', event.target.checked));
        this.#servoPinSelector.addEventListener('change', (event) => this.#sendCommand('setServoPin', event.target.value));
        this.#angleStepDSB.addEventListener('change', (event) => {
            let value = parseInt(event.target.value);
            if (value >= 0 && value <= 360) {
                this.#sendCommand('setServoAngleStep', value);
            } else {
                event.target.value = Math.max(0, Math.min(value, 360));
            }
        });

        this.#timeDelayDSB.addEventListener('change', (event) => {
            let value = parseInt(event.target.value);
            if (value >= 0 && value <= 10000) {
                this.#sendCommand('timeDelay', value);
            }
        });

        this.#maxRangeDSB.addEventListener('change', (event) => {
            let value = parseInt(event.target.value);
            if (value >= 0 && value <= 360) {
                this.#sendCommand('maxRange', value);
            }
        });
        this.#sendTextButton.addEventListener('click', (event) => {
            let val = event.target.value;
            this.#ws.send(JSON.stringify({val}));
        });
    }

    #sendCommand(command, value) {
        try {
            let cmd = getCommand(command, value);
            this.#ws.send(cmd);
        } catch (error) {
            console.error(error);
        }
    }

    /**
     * clashes w/ command names, so added underscore to
     * each public getter.
     * @returns {*}
     * @private
     */
    get _servoCB() { return this.#servoCB; }
    get _servoStream() { return this.#servoStream; }
    get _servoPin() { return this.#servoPinSelector; }
    get _angleStep() { return this.#angleStepDSB.value; }
    get _timeDelay() { return this.#timeDelayDSB.value; }
    get _maxRange() { return this.#maxRangeDSB.value; }
}

let device = new DeviceConfig();




