#include <WebSocketsClient.h>
#include <WiFiS3.h>
#include <LiquidCrystal_74HC595.h>

//PW range: 0.5 - 2.5 ms
//

// WL_NO_SHIELD = 255,
//        WL_NO_MODULE = WL_NO_SHIELD,
//        WL_IDLE_STATUS = 0,
//        WL_NO_SSID_AVAIL,
//        WL_SCAN_COMPLETED,
//        WL_CONNECTED,
//        WL_CONNECT_FAILED,
//        WL_CONNECTION_LOST,
//        WL_DISCONNECTED,
//        WL_AP_LISTENING,
//        WL_AP_CONNECTED,
//        WL_AP_FAILED

//------------------------------------------------------------------ LCD: 
//shift register pins
byte DS = 11; //SER
byte SHCP = 13; //SRCLK
byte STCP = 12; //RCLK 
byte RS = 1;
byte E = 3;
byte _D4 = 4;
byte _D5 = 5;
byte _D6 = 6;
byte _D7 = 7;
//lcd w/ shift register object instantiation with specified pins
LiquidCrystal_74HC595 lcd(DS, SHCP, STCP, RS, E, _D4, _D5, _D6, _D7);
//custom character maps
byte full_wifi[8] = {0x0E, 0x1F, 0x11, 0x04, 0x0E, 0x00, 0x04, 0x00};
byte med_wifi[8] = {0x00, 0x00, 0x00, 0x04, 0x0E, 0x00, 0x04, 0x00};
byte low_wifi[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00};
byte no_shield_wifi[8] = {0x14, 0x1D, 0x1D, 0x09, 0x00, 0x01, 0x00, 0x00};
byte error_wifi[8] = {0x04, 0x04, 0x04, 0x04, 0x00, 0x04, 0x00, 0x00};
byte not_connected_wifi[8] = {0x11, 0x0A, 0x04, 0x0A, 0x11, 0x00, 0x00, 0x00};

//------------------------------------------------------------------ Wi-Fi Client:
WiFiClient client;
WebSocketsClient client_ws;

CWifi *const w = &WiFi; //easier typing
char server[] = "calm-citadel-25518-ec2bfb31abdd.herokuapp.com";
char ssid[] = "UI-DeviceNet";
char pass[] = "UI-DeviceNet";

//------------------------------------------------------------------ Serial Debugging:
//make serial function similar to std::cout <<
template <typename T>
inline Print & operator << (Print &stream, T value) {
  stream.print(value);
  return stream;
}
inline void endl() {
  Serial.println();
}
inline Print &operator << (Print &stream, void (*func)()) {
  func();
  return stream;
}

//------------------------------------------------------------------ State management/event handling:
//device state types
typedef enum device_state_type {
  D_empty,        // 0 - won't use
  D_LCD_INIT,     // 1 - LCD initialization
  D_IDLE,         // 3 - idle - listen for commands via server/button
  D_FLEX,         // 4 - active flexion
  D_HOLD         // 5 - active hold
};
//server state types
typedef enum server_state_type {
  S_INIT,         // 0 - initialization
  S_LISTENING,    // 1 - listening for commands
  S_SLEEP         // 2 - not listening
};
//------------------------------------------------------------------ Commands
// For now, just 1 command: rotate. This has 2 parameters, hold-time and angle

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  while (!Serial);
}

void loop() {
  // put your main code here, to run repeatedly:
  //exoskeleton control variables
  
  static int exo_state = D_LCD_INIT;
  static int flex_delay = 50; 
  static int w_state = -1;
  static int motor_pin = 9;
  static unsigned long goal_angle_required_time = static_cast<long>(2000);
                              //goal angle, goes where 200 is
  static long goal_angle_pwm = map(200, 0.5, 1.5, 0, 255);
  static long current_angle_pwm = map(0, 0.5, 1.5, 0, 255);
  static long angle_step_pwm = static_cast<long>((goal_angle_pwm - current_angle_pwm) / flex_delay); //angle step required for destined angle constrained by delay 
  static unsigned long t_servo = static_cast<long>(0);
  //static unsigned long min_w_hold = 1000 / 50.0; //50 hz min frequency

  static unsigned long ts;
  static unsigned int it_counter = 1;
  static unsigned int failed_attempts_wifi = 0;
  static bool wifi_sleep = false;
  static int server_state = 0;
  auto webSocketEvent = [] (WStype_t in_type, uint8_t *payload, size_t length) {
    switch (in_type) {
      case WStype_DISCONNECTED:
        Serial << "Disconnected from server" << endl;
        server_state = S_SLEEP;
        break;  
      case WStype_CONNECTED:
        Serial << "Connected to server" << endl;
        break;
      case WStype_TEXT:
        Serial << *payload << endl;
        break;
      default:
        break;
    }
  };

  switch (exo_state) {
    case D_empty:
    //do nothing for now
    break;
    case D_LCD_INIT: {
      init_lcd();
      ts = millis();
      exo_state = D_IDLE;
      break;
    }
    case D_IDLE:
      //control power to servo - allow for idling 
      break;
    case D_FLEX:
      if (millis() - ts > flex_delay) {
        if (current_angle_pwm < goal_angle_pwm) {
          current_angle_pwm += angle_step_pwm;
        }
        ts = millis();
        analogWrite(motor_pin, current_angle_pwm);
      }
      break;
    default:
      break;
  }
  //handling wifi status states, where -1 represents initialization state
  if (!wifi_sleep) {
    switch (w_state) {
      case -1:
        w->begin(ssid, pass);
        w_state = w->status();
        ts = millis();
        break;
      case WL_IDLE_STATUS:
        //update animation (custom characters in CGRAM 0 - 3)
        if (millis() - ts > 500) {
          lcd.setCursor(15, 0);
          if (it_counter > 3) {
            it_counter = 0;
            lcd.write(byte(0));
          } else {
            it_counter++;
            lcd.write(it_counter);
          }
          lcd.home();
        }
        w_state = w->status(); //update w_state as wifi status
        break;
      case WL_NO_SHIELD:
        lcd.setCursor(15, 0);
        lcd.write(3); //write no_shield character stored in CGRAM
        lcd.home();
        wifi_sleep = true;
        break;
      case WL_CONNECTED: {
        if (millis() - ts > 10000) {
          w_state = w->status();
          long rssi = w->RSSI();
          lcd.setCursor(15, 0);
          if (rssi > -100) {
            if (rssi > -80) {
              if (rssi > -70) {
                lcd.write(2);
                lcd.home();
              } else {
                lcd.write(1);
                lcd.home();
              }
            } else {
              lcd.write(1);
              lcd.home();
            }
          } else {
            lcd.write(byte(0));
          }
        }
        break;
      }

      case WL_DISCONNECTED:
        w_state = w->status();
        ts = millis();
        lcd.write(5);
        wifi_sleep = true;
        break;
      case WL_CONNECTION_LOST:
        if (millis() - ts < 1000) {
          w_state = -1;
        } 
        break;
      case WL_CONNECT_FAILED: // 3 allowed failed attempts
        if (failed_attempts_wifi < 3) {
          failed_attempts_wifi++;
          w_state = -1;
        } else {
          failed_attempts_wifi = 0;
          wifi_sleep = true;
        }
        break;
      default:
        w_state = WL_DISCONNECTED;
        break;
    }
    switch (server_state) {
      case S_INIT:
        client_ws.begin(server, 80);
        client_ws.onEvent(webSocketEvent);
        break;
      default:
        break;
    }
  }
}
void init_lcd() {
  Serial << "Initializing lcd custom characters" << endl;
  lcd.begin(16, 2);
  lcd.createChar(0, low_wifi);
  lcd.createChar(1, med_wifi);
  lcd.createChar(2, full_wifi);
  lcd.createChar(3, no_shield_wifi);
  lcd.createChar(4, error_wifi);
  lcd.createChar(5, not_connected_wifi);
  lcd.noBlink();
  lcd.display();
}
