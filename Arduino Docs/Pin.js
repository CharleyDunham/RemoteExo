/**
 * Pin class for managing digital/analog pins.
 */
class Pin {
    /**
     * Private method to retrieve all device pins (whether occupied or not),
     *  depending on the 'device' (nano or r4).
     * @param device
     * @param pinType
     * @returns {string[]|*[]}
     */
    static #getPins = (device, pinType) => {
        if (pinType === 'digital') {
            return Array.from({ length: 14 }, (_, i) => `D${i}`);
        }
        if (pinType === 'analog') {
            const p = Array.from({ length: 6 }, (_, i) => `A${i}`);
            if (device === 'nano') p.push('A6', 'A7');
            return p;
        }
        console.error('Invalid pin type');
        return [];
    }
    /**
     * A JSON object storing the pins for all devices.
     * Structure:
     * pins =
     * {
     *     nano: {
     *         digital: ['D0', ..., 'D13'],
     *         analog: ['A0', ..., 'A7']
     *     },
     *     r4: {
     *         digital: ['D0', ..., 'D13'],
     *         analog: ['A0, ..., 'A5']
     *     }
     * };
     * @type {{nano: {digital: string[]|*[], analog: string[]|*[]}, r4: {digital: string[]|*[], analog: string[]|*[]}}}
     */
    static #pins = {
        nano: {
            digital: Pin.#getPins('nano', 'digital'),
            analog: Pin.#getPins('nano', 'analog'),
        },
        r4: {
            digital: Pin.#getPins('r4', 'digital'),
            analog: Pin.#getPins('r4', 'analog'),
        }
    };
    /**
     * Same structure as prior JSON pins variable.
     * @type {any}
     */
    static #availablePins = JSON.parse(JSON.stringify(Pin.#pins));
    static get device() {
        const element = document.getElementById('device');
        return element ? element.value : null;
    }

    /**
     * Single-argument constructor which, by default, implies occupation (declaration of a Pin with the
     *  argument 'A0', or any other pin, assumes that the pin is being occupied).
     * @param pin
     */
    constructor(pin) {
        const device = Pin.device;
        if (!Pin.#pins[device]) {
            throw new Error(`Invalid device: ${device}`);
        }
        const available = Pin.#availablePins[device];
        const found = available.digital.includes(pin) || available.analog.includes(pin);
        if (found) {
            console.log(`Reserving pin ${pin} on ${device}`);
            Pin.occupyPin(pin);
        } else {
            throw new Error(`Pin ${pin} not available on ${device}`);
        }
    }
    /**
     * Static public method for acquiring the available pins for the device defined by the DOM.
     */
    static get availablePins() {
        const device = Pin.device;
        if (!Pin.#pins[device]) {
            console.error(`Invalid device: ${device}`);
            return null;
        }
        return Pin.#availablePins[device];
    }

    /**
     * Static public method for determining whether the parameter 'pin' is currently occupied.
     * @param pin
     * @returns {boolean}
     */
    static pinAvailable(pin) {
        const device = Pin.device;
        if (!Pin.#pins[device]) {
            console.error(`Invalid device: ${device}`);
            return false;
        }
        const available = Pin.#availablePins[device];
        return available.digital.includes(pin) || available.analog.includes(pin);
    }

    /**
     * Static public method for making the 'pin' occupied.
     * @param pin
     */
    static occupyPin(pin) {
        const device = Pin.device;
        if (!Pin.#pins[device]) {
            console.error(`Invalid device: ${device}`);
            return;
        }
        const available = Pin.#availablePins[device];
        available.digital = available.digital.filter(p => p !== pin);
        available.analog = available.analog.filter(p => p !== pin);
    }
}