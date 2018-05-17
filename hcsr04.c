#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>

#define TRIG 29
#define ECHO 28

void setup(){
	wiringPiSetup();
	pinMode(TRIG,OUTPUT);
	pinMode(ECHO,INPUT);
	digitalWrite(TRIG, LOW);
	delay(30);

}

void getCM(){
	// enviar pulso 
	digitalWrite(TRIG,HIGH);
	delayMicroseconds(20);
	digitalWrite(TRIG,LOW);

	//espera o ECHO começar
	while(digitalRead(ECHO)==LOW);

	//espera pelo fim do eco
	long startTime = micros();
	while(digitalRead(ECHO) == HIGH);
	long travelTime = micros() = startTime;

	// obtendo a distancia em cm
	int distance = travelTime / 58;
	return distance;

}
int main()
{
	setup();
	printf("Distancia:%dcm\n",getCM());
	return 0;
}