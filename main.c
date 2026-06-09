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
#include "libs/timer.h"



#define SERVO_MIN   2000  // ~1 ms pulso (0 grados)
#define SERVO_MID   3000  // ~1.5 ms pulso (90 grados)
#define SERVO_MAX   4000  // ~2 ms pulso (180 grados)
#define Pin_servo 5
#define Motor_adelante 1
#define Motor_Atras 2
#define PWM 7
volatile int contador_personas = 8;
volatile int Contador_subidas = 2;
volatile int bandera_servo = 0;
volatile int bandera_motor = 0;



static void PaBajo(void){
	if (contador_personas > 0) {
		contador_personas--;
	} else {
		bandera_servo = 1;
		contador_personas = 8;
	}
}

static void Caida(void){
	if (Contador_subidas >= 2){
		Contador_subidas--;
	}
	else{
		bandera_motor = 1;
		Contador_subidas = 2; 
	}
}


void PWM_servo(void) {
	
	GPIO_PIN_MODE_PORTD(Pin_servo,OUTPUT);
	
	hal_timer_config_t cfg = {
		.mode      = HAL_TIMER_MODE_FAST_PWM,
		.prescaler = HAL_TIMER_PRESCALER_8,
		.top       = 39999   
	};
	HAL_Timer_Init(HAL_TIMER1, &cfg);
	HAL_Timer_PWM_Enable(HAL_TIMER1, HAL_TIMER_CH_A);
	HAL_Timer_PWM_SetRaw(HAL_TIMER1, HAL_TIMER_CH_A, SERVO_MIN);
	HAL_Timer_Start(HAL_TIMER1);
}

void PWM_Motor_Adelante(void){
	GPIO_PIN_MODE_PORTC(Motor_adelante,OUTPUT);
	GPIO_PIN_MODE_PORTD(PWM, OUTPUT);
	hal_timer_config_t cfg = {
		.mode      = HAL_TIMER_MODE_FAST_PWM,
		.prescaler = HAL_TIMER_PRESCALER_8,
		.top       = 255
	};
	HAL_Timer_Init(HAL_TIMER2, &cfg);
	HAL_Timer_PWM_Enable(HAL_TIMER2, HAL_TIMER_CH_A);
	HAL_Timer_PWM_SetRaw(HAL_TIMER2, HAL_TIMER_CH_A, 0);
	HAL_Timer_Start(HAL_TIMER2);
	
}

void PWM_Motor_Atras(void){
	GPIO_PIN_MODE_PORTC(Motor_Atras,OUTPUT);
	GPIO_PIN_MODE_PORTD(PWM, OUTPUT);
	hal_timer_config_t cfg = {
		.mode      = HAL_TIMER_MODE_FAST_PWM,
		.prescaler = HAL_TIMER_PRESCALER_8,
		.top       = 255
	};
	HAL_Timer_Init(HAL_TIMER2, &cfg);
	HAL_Timer_PWM_Enable(HAL_TIMER2, HAL_TIMER_CH_A);
	HAL_Timer_PWM_SetRaw(HAL_TIMER2, HAL_TIMER_CH_A, 0);
	HAL_Timer_Start(HAL_TIMER2);
}

void Timer_contador(void) {
	hal_timer_config_t cfg = {
		.mode      = HAL_TIMER_MODE_CTC,
		.prescaler = HAL_TIMER_PRESCALER_1024,
		.top       = 155  
	};
	HAL_Timer_Init(HAL_TIMER0, &cfg);
	HAL_Timer_RegisterCallback(HAL_TIMER0, HAL_TIMER_IRQ_COMPARE_A, PaBajo);
	HAL_Timer_EnableIRQ(HAL_TIMER0, HAL_TIMER_IRQ_COMPARE_A);
	HAL_Timer_Start(HAL_TIMER0);
}


void Detener_Motor(void){
	GPIO_PIN_MODE_PORTC(Motor_adelante,INPUT);
	GPIO_PIN_MODE_PORTC(Motor_Atras,INPUT);
}

void Menu_lcd(void){
	lcd_init();
	lcd_clear();
	lcd_disable_blink();
	lcd_disable_cursor();
	lcd_set_cursor(0,0);
	
}

int main(void)
{	
	PWM_servo();
	PWM_Motor_Adelante();
	Timer_contador();

	HAL_IRQ_ENABLE();
	
    
	uint8_t Posicion = 0;
    while (1) 
    {	
		if(bandera_servo){
			bandera_servo = 0;
			
			if(Posicion == 0){
				HAL_Timer_PWM_SetRaw(HAL_TIMER1, HAL_TIMER_CH_A, SERVO_MAX);
				Posicion = 1;
			}
			else{
				HAL_Timer_PWM_SetRaw(HAL_TIMER1, HAL_TIMER_CH_A, SERVO_MIN);
				Posicion = 0;
			}
			}
    }
}

