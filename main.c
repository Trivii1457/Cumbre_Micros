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
#define Pin_servo 0
#define Motor_adelante 1
#define Motor_Atras 2
#define PWM 5
volatile int contador = 8;



static void PaBajo(void){
	if (contador > 0) contador--;
}


void PWM_servo(void) {
	
	GPIO_PIN_MODE_PORTC(Pin_servo,OUTPUT);
	
	hal_timer_config_t cfg = {
		.mode      = HAL_TIMER_MODE_FAST_PWM,
		.prescaler = HAL_TIMER_PRESCALER_8,
		.top       = 39999   // ICR1 = 39999 -> 50 Hz
	};
	HAL_Timer_Init(HAL_TIMER1, &cfg);
	HAL_Timer_PWM_Enable(HAL_TIMER1, HAL_TIMER_CH_A);
	HAL_Timer_PWM_SetRaw(HAL_TIMER1, HAL_TIMER_CH_A, SERVO_MIN);
	HAL_Timer_Start(HAL_TIMER1);
}

void PWM_Motor(void){
	GPIO_PIN_MODE_PORTC(Motor_adelante,OUTPUT);
	GPIO_PIN_MODE_PORTB(PWM, OUTPUT);

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
	/* Timer0 CTC — genera IRQ periodica para decrementar contador */
	hal_timer_config_t cfg = {
		.mode      = HAL_TIMER_MODE_CTC,
		.prescaler = HAL_TIMER_PRESCALER_1024,
		.top       = 155   // ~10 Hz con F_CPU=16MHz: 16M/(2*1024*10)-1 = 780
		// ajusta este valor segun cada cuanto quieres decrementar
	};
	HAL_Timer_Init(HAL_TIMER0, &cfg);
	HAL_Timer_RegisterCallback(HAL_TIMER0, HAL_TIMER_IRQ_COMPARE_A, PaBajo);
	HAL_Timer_EnableIRQ(HAL_TIMER0, HAL_TIMER_IRQ_COMPARE_A);
	HAL_Timer_Start(HAL_TIMER0);
}

int main(void)
{	
	PWM_servo();
	PWM_Motor();
	Timer_contador();

	HAL_IRQ_ENABLE();
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

