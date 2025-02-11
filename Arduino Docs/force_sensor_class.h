#ifndef FORCE_SENSOR_CLASS_H
#define FORCE_SENSOR_CLASS_H

#include <Arduino.h>
#include <math.h>
class force_sensor {
public:
	force_sensor(unsigned int& pin_no, const bool monitor_input = true, const unsigned long sample_delay_ms = 150);
	void update();
	void set_delay(const unsigned long delay);
	void set_on_off(const bool on = true);
	float get_force() const;
private:
	float m_resistance;
	float m_force;
	unsigned int& m_pin_no;
 	bool m_monitor_input;
	unsigned long m_sample_delay;
	unsigned long m_current_time;
	unsigned long m_prev_time;
	//constants
	const float m_k_r = static_cast<float>(50.0 / (10 * 5.0 / 1023));
	const float m_k_f = static_cast<float>(pow(10, -0.669) / 194.46);
};

#endif