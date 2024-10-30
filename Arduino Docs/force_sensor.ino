#include <Serial.h>
#include <math.h>
#include "force_sensor_class.h"

unsigned int data_pin = A0;
force_sensor sensor1(data_pin);

void setup() {
  unsigned int data_pin = A0;
  // put your setup code here, to run once:
  pinMode(data_pin, INPUT);
  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
  
  sensor1.update();
  Serial.println(sensor1.get_force());
}
