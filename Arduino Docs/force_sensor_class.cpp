#include "force_sensor_class.h"

force_sensor::force_sensor(unsigned int& pin_no, const bool monitor_input, const unsigned long delay) : m_pin_no(pin_no), m_monitor_input(monitor_input), m_sample_delay(delay) {
	m_prev_time = 0;
}


//must be continuously called to work
void force_sensor::update() {
	m_current_time = millis();
	
	if (!m_monitor_input) {
		return;
	}
	if (m_current_time - m_prev_time < m_sample_delay) {
		return;
	}
	//since both conditions fail, update previous time, resistance, and force.
	m_prev_time = m_current_time;
	m_resistance = static_cast<float>((m_k_r / analogRead(m_pin_no)) - 1);
	m_force = static_cast<float>((m_k_f * m_resistance));
}

void force_sensor::set_delay(const unsigned long delay) {
	if (delay > 10000) {
	//delay limit
		Serial.println("Delay is too long");
		return;
	}

	m_sample_delay = delay;
	return;
}

void force_sensor::set_on_off(const bool on) {
	m_monitor_input = on;
}

float force_sensor::get_force() const {
	return m_force;
}
