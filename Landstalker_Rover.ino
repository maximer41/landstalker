/*
    Project: Landstalker
    Code for: Rover
    Author: Michał "maximer mane" Brdys
    Copyright (c) 2022 - 2023
*/


#include <NewPing.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <printf.h>
#include <RF24.h>
#include <RF24_config.h>
#include <Servo.h>

// engines
int mapped_speed;
int a_pwm = 11;
int a1_out = 6;
int a2_out = 10;

int b_pwm = 9;
int b1_out = 4;
int b2_out = 5;

// communication
RF24 radio(7, 8);
int recive_datas[9]; // {int joystick_x, int joystick_y, bool led, bool obstacle_detection, int servo_1 (cam), int servo_2 (cam), int servo_3 (arm), int servo_4 (arm), int servo_5 (arm)}}
int send_datas[4]; // {int battery level, bool obstacle detected, bool cover}
const byte addresses[][6] = {"00001", "00002"};

// leds bar
#define led_pin 23
bool led_state = false;

// obstacle detection
#define echo_pin 2
#define trig_pin 3
bool obstacle_state = true;
unsigned int distance;
NewPing sonar(trig_pin, echo_pin, 200);

// servos
#define servo1_PIN 24
#define servo2_PIN 25
#define servo3_PIN 28
#define servo4_PIN 27
#define servo5_PIN 26
Servo servo_1;
Servo servo_2;
Servo servo_3;
Servo servo_4;
Servo servo_5;

// cover switch
#define cover_PIN 22

// battery status
#define battery_PIN A7
float battery_level;
int battery_percent;
int long batteryStartMillis;
int long batteryCurrentMillis;

void setup(){
  Serial.begin(9600);

  // obstacle detection
  pinMode(trig_pin, OUTPUT);
  pinMode(echo_pin, INPUT);

  // leds bar
  pinMode(led_pin, OUTPUT);
  
  // pins for engines
  pinMode(a1_out, OUTPUT);
  pinMode(a2_out, OUTPUT);
  pinMode(a_pwm, OUTPUT);

  pinMode(b1_out, OUTPUT);
  pinMode(b2_out, OUTPUT);
  pinMode(b_pwm, OUTPUT);

  // communication
  radio.begin();     
  radio.openReadingPipe(1, addresses[0]);
  radio.openWritingPipe(addresses[1]);
  radio.setDataRate(RF24_2MBPS);
  radio.setPALevel(RF24_PA_LOW);

  // cover switch
  pinMode(cover_PIN, INPUT_PULLUP);

  // battery check
  batteryStartMillis = millis();
}

void loop(){
  radio.startListening();
  
  if (radio.available()){
  radio.read(recive_datas, sizeof(recive_datas));
    
  if(recive_datas[2] != led_state) {
    led_state = recive_datas[2];
    digitalWrite(led_pin, recive_datas[2]); 
  }
                
  if(recive_datas[3] != obstacle_state) {
    obstacle_state = recive_datas[3];
  }

  batteryCurrentMillis = millis();
  if(batteryCurrentMillis - batteryStartMillis >= 300000 or battery_percent==false) {
    battery_level = analogRead(battery_PIN);
    battery_level = battery_level * 0.0048;
    battery_percent = ((battery_level - 3)/(4.2 - 3))*100;
    batteryStartMillis = millis();
  }
  
  if(not servo_1.attached() and not servo_2.attached() and not servo_3.attached() and not servo_4.attached() and not servo_5.attached()) {
    servo_1.attach(servo1_PIN);
    servo_2.attach(servo2_PIN);
    servo_3.attach(servo3_PIN);
    servo_4.attach(servo4_PIN);
    servo_5.attach(servo5_PIN);
  }

  servo_1.write(recive_datas[4]);
  servo_2.write(recive_datas[5]);
  servo_3.write(recive_datas[6]);
  servo_4.write(recive_datas[7]);
  servo_5.write(recive_datas[8]);
  
  if(recive_datas[0] > 450 and recive_datas[0] < 550 and recive_datas[1] > 450 and recive_datas[1] < 550){ // neutral
    analogWrite(a_pwm, 0);
    analogWrite(b_pwm, 0);
    digitalWrite(a1_out, LOW);
    digitalWrite(a2_out, LOW);
    digitalWrite(b1_out, LOW);
    digitalWrite(b2_out, LOW);
    
  } else if(recive_datas[0]>=550 and recive_datas[1]>450 and recive_datas[1]<550) { // back
    mapped_speed = map(recive_datas[0], 550, 1023, 0, 255);
    analogWrite(a_pwm, mapped_speed);
    analogWrite(b_pwm, mapped_speed); 
  
    send_datas[1]=false;
  
    digitalWrite(a1_out, HIGH);
    digitalWrite(a2_out, HIGH);
    digitalWrite(b1_out, LOW);
    digitalWrite(b2_out, LOW);
    
  } else if(recive_datas[0]<=450 and recive_datas[1]>450 and recive_datas[1]<550) { //forward
    if(obstacle_state==true) {
    distance = sonar.ping_cm();   
     
      if(distance>25) {
        mapped_speed = map(recive_datas[0], 450, 0, 0, 255);
        analogWrite(a_pwm, mapped_speed);
        analogWrite(b_pwm, mapped_speed); 
    
        send_datas[1]=false;
        
        digitalWrite(a1_out, LOW);
        digitalWrite(a2_out, LOW);
        digitalWrite(b1_out, HIGH);
        digitalWrite(b2_out, HIGH);
      } else {
        analogWrite(a_pwm, 0);
        analogWrite(b_pwm, 0);
    
        send_datas[1]=true;
    
        digitalWrite(a1_out, LOW);
        digitalWrite(a2_out, LOW);
        digitalWrite(b1_out, LOW);
        digitalWrite(b2_out, LOW);
      }
    } else {
      mapped_speed = map(recive_datas[0], 450, 0, 0, 255);
      analogWrite(a_pwm, mapped_speed);
      analogWrite(b_pwm, mapped_speed); 
    
      send_datas[1]=false;
      
      digitalWrite(a1_out, LOW);
      digitalWrite(a2_out, LOW);
      digitalWrite(b1_out, HIGH);
      digitalWrite(b2_out, HIGH);
    } 
  
  } else if(recive_datas[1]>=550 and recive_datas[0]>450 and recive_datas[0]<550) { //right
    mapped_speed = map(recive_datas[1], 550, 1023, 0, 255);
    analogWrite(a_pwm, mapped_speed);
    analogWrite(b_pwm, mapped_speed); 
    
    send_datas[1]=false;
    
    digitalWrite(a1_out, LOW);
    digitalWrite(a2_out, HIGH);
    digitalWrite(b1_out, LOW);
    digitalWrite(b2_out, HIGH);
    
  } else if(recive_datas[1]<=450 and recive_datas[0]>450 and recive_datas[0]<550) { // left
    mapped_speed = map(recive_datas[1], 450, 0, 0, 255);
    analogWrite(a_pwm, mapped_speed);
    analogWrite(b_pwm, mapped_speed);
    
    send_datas[1]=false;
    
    digitalWrite(a1_out, HIGH);
    digitalWrite(a2_out, LOW);
    digitalWrite(b1_out, HIGH);
    digitalWrite(b2_out, LOW);
    }
  }
  
  delay(5);
  
  radio.stopListening();
  send_datas[0] = battery_percent;
  send_datas[2] = digitalRead(cover_PIN);
  
  radio.write(send_datas, sizeof(send_datas));
}
