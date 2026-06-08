/*
 * Torre.c
 *
 * Created: 6/8/2026 3:06:38 PM
 * Author : USUARIO
 */ 

#include "libs/config.h"
#include "libs/gpio.h"
#include "libs/interrupt.h"
#include "libs/lcd.h"



#define SERVO_MIN   2000  // ~1 ms pulso (0 grados)
#define SERVO_MID   3000  // ~1.5 ms pulso (90 grados)
#define SERVO_MAX   4000  // ~2 ms pulso (180 grados)
#define Pin_servo 0
#define Motor_adelante 1
#define Motor_Atras 2
#define PWM 5
int contador = 8;



ISR(INT1_vect){
	if(contador>0) contador--;
}


void PWM_servo(void) {
	
	GPIO_PIN_MODE_PORTC(Pin_servo,OUTPUT);
	
	TCCR1A = (1 << COM1A1) | (1 << WGM11);
	TCCR1B = (1 << WGM13) | (1 << WGM12);

	
	ICR1 = 39999;

	TCCR1B |= (1 << CS11);
	
	OCR1A = SERVO_MIN;
}

void PWM_Motor(void){
	GPIO_PIN_MODE_PORTC(Motor_adelante,OUTPUT);
	GPIO_PIN_MODE_PORTB(PWM, OUTPUT);

	TCCR2A = (1 << COM2A1) | (1 << WGM21) | (1 << WGM20);
	TCCR2B = (1 << CS21);
	OCR2A = 0;
	
}
int main(void)
{	
	PWM_servo();
	PWM_Motor();
	GPIO_PIN_MODE_PORTD(3, INPUT);
	GPIO_PULLUP_PORTD(3, HIGH);
	EICRA = (EICRA & ~(0x03 << 2)) | (0x02 << 2);
	sei();
	lcd_init();
	lcd_disable_cursor();
	lcd_disable_blink();
	lcd_set_cursor(0,0);
	lcd_puts("Bienvenido a la torre de la muerte");
    /* Replace with your application code */
    while (1) 
    {
    }
}

