 /*
    Project: Landstalker
    Code for: Controller
    Author: Michał "maximer mane" Brdys
    Copyright (c) 2022 - 2023
*/


#include <LiquidCrystal_I2C.h>
#include <nRF24L01.h>
#include <printf.h>
#include <RF24.h>
#include <RF24_config.h>
#include <SPI.h>

// global variables
int loading = true;
bool connection = false;
String notification = "";

// buttons
#define led_PIN 6
#define obstacle_PIN 5
bool led_state = false;
bool led_clicked = false;
String led_text = " ON";

// obstacle detector
bool obstacle_state = false;
bool obstacle_clicked = false;
bool obstacle_detected = false;
String obstacle_text = " ON ";

// buzzer
#define buzzer_PIN 4

// potentiometers
#define potentiometer_1 A2
#define potentiometer_2 A3
#define potentiometer_3 A6
#define potentiometer_4 A8
#define potentiometer_5 A9
int arm_1;
int arm_2;
int arm_3;
int arm_4;
int arm_5;

// battery level
#define battery_PIN A7
float battery_level;
int battery_percent;
int battery_percent_text;
int battery_rover;
int battery_rover_text;

// joystick
#define joystick_x A0
#define joystick_y A1

// cover switch
bool cover_state = false;

// lcd and custom chars
LiquidCrystal_I2C lcd(0x27,20,4);

// communication
#define CE_PIN 7
#define CSN_PIN 8
RF24 radio(CE_PIN, CSN_PIN);
int send_datas[9]; // {int joystick_x, int joystick_y, bool led, bool obstacle_detection, int servo_1 (cam), int servo_2 (cam), int servo_3 (arm), int servo_4 (arm), int servo_5 (arm)}
int recive_datas[4]; // {int battery level, bool obstacle detected, bool cover}
const byte addresses[][6] = {"00001", "00002"};
int long timeoutStartMillis;
int long timeoutCurrentMillis;

byte logo_piece_1[] = {
  0x1F,
  0x1F,
  0x1F,
  0x00,
  0x1F,
  0x11,
  0x1F,
  0x00
};

byte logo_piece_2[] = {
  0x18,
  0x06,
  0x1A,
  0x02,
  0x02,
  0x1F,
  0x1F,
  0x1F
};

byte logo_piece_3[] = {
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x1F,
  0x1F,
  0x1F
};

byte logo_piece_4[] = {
  0x0D,
  0x1E,
  0x0C,
  0x04,
  0x04,
  0x1F,
  0x1F,
  0x1F
};

byte battery_icon[] = {
  0x0E,
  0x1B,
  0x11,
  0x11,
  0x1F,
  0x1F,
  0x1F,
  0x1F
};

byte bell_icon[] = {
  0x00,
  0x04,
  0x0E,
  0x0E,
  0x0E,
  0x1F,
  0x04,
  0x00
};

byte light_icon[] = {
  0x0E,
  0x11,
  0x15,
  0x15,
  0x0A,
  0x0E,
  0x0E,
  0x04
};

byte obstacle_icon[] = {
  0x00,
  0x03,
  0x0B,
  0x0B,
  0x0B,
  0x03,
  0x0B,
  0x03
};


void setup()
{
  Serial.begin(9600);

  // led and obstacle detection buttons
  pinMode(led_PIN, INPUT_PULLUP);
  pinMode(obstacle_PIN, INPUT_PULLUP);

  // joystick
  pinMode(joystick_x, INPUT);
  pinMode(joystick_y, INPUT);

  // potentiometers
  pinMode(potentiometer_1, INPUT);
  pinMode(potentiometer_2, INPUT);
  pinMode(potentiometer_3, INPUT);
  pinMode(potentiometer_4, INPUT);
  pinMode(potentiometer_5, INPUT);

  // communication
  radio.begin();
  radio.openReadingPipe(1, addresses[1]);
  radio.openWritingPipe(addresses[0]);
  radio.setDataRate(RF24_2MBPS);
  radio.setPALevel(RF24_PA_LOW);
  timeoutStartMillis = millis();

  // lcd
  lcd.init();
  lcd.backlight();
  lcd.begin(20,4);
  lcd.createChar(0, logo_piece_1);
  lcd.createChar(1, logo_piece_2);
  lcd.createChar(2, logo_piece_3);
  lcd.createChar(3, logo_piece_4);
  lcd.createChar(4, battery_icon);
  lcd.createChar(5, bell_icon);
  lcd.createChar(6, light_icon);
  lcd.createChar(7, obstacle_icon);
  lcd.clear();

  // buzzer
  pinMode(buzzer_PIN, OUTPUT);
}


void loop() {
  // loading screen
  if(loading==true){
    lcd.setCursor(2,2);
    lcd.write(0);
    lcd.write(0);
    lcd.write(0);
  
    lcd.setCursor(2,1);
    lcd.write(1);
    lcd.write(2);
    lcd.write(3);
    
    lcd.setCursor(7,0);
    lcd.print("LANDSTALKER");
  
    lcd.setCursor(9,1);
    lcd.print("PROJECT");

    lcd.setCursor(7,3);
    lcd.print("Loading...");
    
    delay(2500);

    loading = false;
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.write(4);
    lcd.setCursor(2,0);
    battery_level = analogRead(battery_PIN);
    battery_level = battery_level * 0.0048;
    battery_percent = ((battery_level - 3)/(4,2 - 3))*100;
    if(battery_percent<0) {
      battery_percent = battery_percent*-1;
    }
  
    lcd.setCursor(0,1);
    lcd.write(4);
    lcd.print(" -");

    lcd.setCursor(6,0);
    lcd.print("|");
    
    lcd.setCursor(6,1);
    lcd.print("|");

    lcd.setCursor(14,0);
    lcd.print("|");

    lcd.setCursor(14,1);
    lcd.print("|");
  
    lcd.setCursor(0,2);
    lcd.print("--------------------");
  
    lcd.setCursor(0,3);
    lcd.write(5);
    lcd.print(notification);
    
    tone(buzzer_PIN, 1000);
    delay(50);
    noTone(buzzer_PIN);
    delay(50);
    
    tone(buzzer_PIN, 1000);
    delay(50);
    noTone(buzzer_PIN);
    delay(50);
    
    tone(buzzer_PIN, 1000);
    delay(50);
    noTone(buzzer_PIN);
  } else {
    radio.startListening();
    
    if(radio.available()) {
      radio.read(recive_datas, sizeof(recive_datas));

      if(recive_datas[0] and recive_datas[0]!=battery_rover) {
        battery_rover = recive_datas[0];
      }
 
      if(recive_datas[1] != obstacle_detected) {
        obstacle_detected = recive_datas[1];
      }

      if(recive_datas[2] != cover_state) {
        cover_state = recive_datas[2];
      }

      timeoutStartMillis = millis();
      connection=true; 
    } else {
      timeoutCurrentMillis = millis();
      
      if(timeoutCurrentMillis - timeoutStartMillis >= 1000) {
        if(connection==true) {
          connection=false;
          timeoutStartMillis = millis();
        }
      }
    }

    delay(5);

    radio.stopListening();
    screenUpdate();

    if(digitalRead(led_PIN)==LOW) {
      if(led_clicked==false) {
        led_state = !led_state;
        led_clicked = true;

        tone(buzzer_PIN, 1000);
        delay(50);
        noTone(buzzer_PIN);
        delay(10);
      }
    } else {
      if(led_clicked==true) {
        led_clicked = false;
      }
    }

    if(digitalRead(obstacle_PIN)==LOW) {
      if(obstacle_clicked==false) {
        obstacle_state = !obstacle_state;
        obstacle_clicked = true;
        
        tone(buzzer_PIN, 1000);
        delay(50);
        noTone(buzzer_PIN);
        delay(10);
      }
    } else {
      if(obstacle_clicked==true) {
        obstacle_clicked = false;
      }
    }

 
    if(connection==true) {
      arm_1 = map(analogRead(potentiometer_1), 100, 904, 115, 180);
      arm_2 = map(analogRead(potentiometer_2), 0, 1024, 0, 115);
      arm_3 = map(analogRead(potentiometer_3), 0, 1024, 15, 110);
      arm_4 = map(analogRead(potentiometer_4), 0, 1024, 135, 0);
      arm_5 = map(analogRead(potentiometer_5), 0, 1024, 90, 0);
      
      send_datas[0] = analogRead(joystick_x);
      send_datas[1] = analogRead(joystick_y);
      send_datas[2] = led_state;
      send_datas[3] = obstacle_state;
      send_datas[4] = arm_1;
      send_datas[5] = arm_2;
      send_datas[6] = arm_3;
      send_datas[7] = arm_4;
      send_datas[8] = arm_5;
      Serial.print(arm_1);
      Serial.print(" ");
      Serial.print(arm_2);
      Serial.print(" ");
      Serial.print(arm_3);
      Serial.print(" ");
      Serial.print(arm_4);
      Serial.print(" ");
      Serial.print(arm_5);
      Serial.println(" ");

      
      radio.write(send_datas, sizeof(send_datas));
    }
  }
}

void screenUpdate() {
  if(connection==false) {
    if(battery_percent<=30) {
      if(notification != " PILOT BATTERY LOW"){
          lcd.setCursor(0,3);
          lcd.write(5);
          notification = " PILOT BATTERY LOW";
          lcd.print(notification);
  
          lcd.setCursor(2,1);
          lcd.print("   ");
          lcd.setCursor(2,1);
          lcd.print("-");
  
          battery_rover=0;
          battery_rover_text=0;
      }
    } else {
      if(notification != "   NO CONNECTION  "){
          lcd.setCursor(0,3);
          lcd.write(5);
          notification = "   NO CONNECTION  ";
          lcd.print(notification);
  
          lcd.setCursor(2,1);
          lcd.print("   ");
          lcd.setCursor(2,1);
          lcd.print("-");

          battery_rover=0;
          battery_rover_text=0;
      }
    }
  } else {
      if(obstacle_detected==true) {
        if(notification != " OBSTACLE DETECTED"){
          lcd.setCursor(0,3);
          lcd.write(5);
          notification = " OBSTACLE DETECTED";
          lcd.print(notification);
  
          tone(buzzer_PIN, 500, 100);
        }
      } else {
        if(cover_state==false) {
          if(notification != "     CONNECTED    "){
            lcd.setCursor(0,3);
            lcd.write(5);
            notification = "     CONNECTED    ";
            lcd.print(notification);
          }
          
        } else if(cover_state==true) {
          if(notification != "    COVER OPEN    "){
            lcd.setCursor(0,3);
            lcd.write(5);
            notification = "    COVER OPEN    ";
            lcd.print(notification);
    
            tone(buzzer_PIN, 500, 100);
          }
          
        } else if(cover_state==false and battery_rover<=30 and battery_percent>30) {
          if(notification != " ROBOT BATTERY LOW"){
            lcd.setCursor(0,3);
            lcd.write(5);
            notification = " ROBOT BATTERY LOW";
            lcd.print(notification);
    
            tone(buzzer_PIN, 500, 100);
          }
          
        } else if(cover_state==false and battery_rover>30 and battery_percent<=30) {
          if(notification != " PILOT BATTERY LOW"){
            lcd.setCursor(0,3);
            lcd.write(5);
            notification = " PILOT BATTERY LOW";
            lcd.print(notification);
    
            tone(buzzer_PIN, 500, 100);
          }
          
        } else if(cover_state==false and battery_rover<=30 and battery_percent<=30) {
          if(notification != "  BATTERIES LOW  "){
            lcd.setCursor(0,3);
            lcd.write(5);
            notification = "  BATTERIES LOW  ";
            lcd.print(notification);
    
            tone(buzzer_PIN, 500, 100);
          }
        }
      }
    }

  
  if(led_state==true) {
    if(led_text != " ON "){
      led_text = " ON ";
      lcd.setCursor(15,0);
      lcd.write(6);
      lcd.print(" ON ");
    }
  } else {
    if(led_text != " OFF"){
      led_text = " OFF";
      lcd.setCursor(15,0);
      lcd.write(6);
      lcd.print(" OFF");
    }
  }

  if(obstacle_state==true) {
    if(obstacle_text != " ON "){
      obstacle_text = " ON ";
      lcd.setCursor(15,1);
      lcd.write(7);
      lcd.print(" ON ");
    }
  } else {
    if(obstacle_text != " OFF"){
      obstacle_text = " OFF";
      lcd.setCursor(15,1);
      lcd.write(7);
      lcd.print(" OFF");
    }
  }
  

  if(connection==true and battery_rover!=battery_rover_text) {
    battery_rover_text = battery_rover;
    lcd.setCursor(2,1);
    lcd.print("   ");
    lcd.setCursor(2,1);
    lcd.print(battery_rover);
    lcd.print("%");
  }

  if(battery_percent!=battery_percent_text) {
    battery_percent_text = battery_percent;
    lcd.setCursor(2,0);
    lcd.print("   ");
    lcd.setCursor(2,0);
    lcd.print(battery_percent);
    lcd.print("%");
  }
}
