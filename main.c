#include <Servo.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16,2);

int distancia=0;
long readUltrassonicDistance( int triggerPin, int echoPin)
{
  pinMode(triggerPin,OUTPUT);
  digitalWrite(triggerPin,LOW);
  delayMicroseconds(2);
  
  digitalWrite(triggerPin,HIGH);
  delayMicroseconds(10);
  digitalWrite(triggerPin,LOW);
  
  pinMode(echoPin,INPUT);
  return pulseIn(echoPin, HIGH);
  
}
Servo servo_9;
void setup(){
  	lcd.init();
  	lcd.backlight();
  	lcd.setCursor(0,0);
	servo_9.attach(9);
}

void loop(){
  	lcd.setCursor(0,0);
  	lcd.print("Hi, I am WALL-E");
	distancia=0.01723*readUltrassonicDistance(7,6);
  	servo_9.write(0);
  	delay(300);
  	
  while(distancia<=20){
    servo_9.write(180);
  	delay(2000);
    distancia=0.01723*readUltrassonicDistance(7,6);
  }

}
