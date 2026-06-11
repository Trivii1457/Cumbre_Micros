#include "libs/config.h"
#include "libs/gpio.h"
#include "libs/interrupt.h"
#include "libs/lcd.h"
#include "libs/timer.h"
//#include <avr/wdt.h>
#define F_CPU 16000000UL

#define SERVO_MIN   2000
#define SERVO_MID   3000
#define SERVO_MAX   4000
#define Pin_servo 5
#define Motor_adelante 1
#define Motor_Atras 2
#define PWM 7
#define Pin_buzzer 0
#define Velocidad 120

volatile int contador_personas = 8;
volatile int contador_vueltas = 0;
volatile uint8_t tick_500ms = 0;
volatile uint8_t bandera_conteo = 0;
volatile uint8_t bandera_motor = 0;


/*
Con la funcion Pabajo se controla la cantidad de personas es decir,
cuando el juego inciia, probablemente la podria renombrar pero me da pereza
@Trivi
*/
static void PaBajo(void){
	if (!bandera_conteo) return;

	if (++tick_500ms < 50) return;
	tick_500ms = 0;

	if (contador_personas > 0){
		contador_personas--;
	}
}

/*
Reiniciar el juego 
@Trivi
*/
/*
static void Reiniciar(void){
	wdt_enable(WDTO_15MS);
	while (1){}
}*/

void PWM_servo(void){
	GPIO_PIN_MODE_PORTD(Pin_servo, OUTPUT);
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

void Servo_abrir(void){
	HAL_Timer_PWM_SetRaw(HAL_TIMER1, HAL_TIMER_CH_A, SERVO_MIN);
}

void Servo_cerrar(void){
	HAL_Timer_PWM_SetRaw(HAL_TIMER1, HAL_TIMER_CH_A, SERVO_MAX);
}

void Motor_init(void){
	GPIO_PIN_MODE_PORTC(Motor_adelante, OUTPUT);
	GPIO_PIN_MODE_PORTC(Motor_Atras, OUTPUT);
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

void PWM_Motor_Adelante(void){
	if (!bandera_motor) return;
	GPIO_WRITE_PORTC(Motor_adelante, HIGH);
	GPIO_WRITE_PORTC(Motor_Atras, LOW);
	HAL_Timer_PWM_SetRaw(HAL_TIMER2, HAL_TIMER_CH_A, Velocidad);
}

void PWM_Motor_Atras(void){
	if (!bandera_motor) return;
	GPIO_WRITE_PORTC(Motor_adelante, LOW);
	GPIO_WRITE_PORTC(Motor_Atras, HIGH);
	HAL_Timer_PWM_SetRaw(HAL_TIMER2, HAL_TIMER_CH_A, Velocidad);
}

void Detener_Motor(void){
	GPIO_WRITE_PORTC(Motor_adelante, LOW);
	GPIO_WRITE_PORTC(Motor_Atras, LOW);
	HAL_Timer_PWM_SetRaw(HAL_TIMER2, HAL_TIMER_CH_A, 0);
}

void Timer_contador(void){
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
/*
void Boton_init(void){
	GPIO_PIN_MODE_PORTD(2, INPUT);
	GPIO_PULLUP_PORTD(2, HIGH);
	HAL_EXT_INT_RegisterCallback(HAL_EXT_INT0, HAL_IRQ_SENSE_FALL, Reiniciar);
	HAL_EXT_INT_CLEAR_FLAG(HAL_EXT_INT0);
	HAL_EXT_INT_ENABLE(HAL_EXT_INT0);
}
*/
void Buzzer_init(void){
	GPIO_PIN_MODE_PORTB(Pin_buzzer, OUTPUT);
	GPIO_WRITE_PORTB(Pin_buzzer, LOW);
}

void Buzzer_sonar(void){
	GPIO_WRITE_PORTB(Pin_buzzer, HIGH);
	_delay_ms(300);
	GPIO_WRITE_PORTB(Pin_buzzer, LOW);
}



void Actualizar_lcd(void){
	lcd_set_cursor(2,1);
	lcd_printf("Personas: %d ", contador_personas);
	lcd_set_cursor(3,1);
	lcd_printf("Vueltas: %d ", contador_vueltas);
}

int main(void)
{
	/*MCUSR &= ~(1 << WDRF);
	wdt_disable()*/

	PWM_servo();
	Motor_init();
	Buzzer_init();
	//Boton_init();
	Timer_contador();
	lcd_init();
	lcd_clear();
	lcd_disable_blink();
	lcd_disable_cursor();
	

	HAL_IRQ_ENABLE();

	while (1)
	{
		contador_personas = 8;
		contador_vueltas = 0;
		bandera_motor = 0;

		Servo_abrir();
		Detener_Motor();

		lcd_clear();
		lcd_set_cursor(1,1);
		lcd_puts("_MUERTE");

		bandera_conteo = 1;
		while (contador_personas > 0){
			Actualizar_lcd();
			_delay_ms(150);
		}
		bandera_conteo = 0;
		Actualizar_lcd();

		Servo_cerrar();
		Buzzer_sonar();
		bandera_motor = 1;

		while (contador_vueltas < 2){
			PWM_Motor_Adelante();
			_delay_ms(1200);
			Detener_Motor();
			_delay_ms(3000);
			PWM_Motor_Atras();
			_delay_ms(1200);
			Detener_Motor();
			contador_vueltas++;
			Actualizar_lcd();
		}

		lcd_clear();

		while (1){
			lcd_set_cursor(2,1);
			lcd_puts("Presiona REINICIO");
			_delay_ms(500);
			lcd_set_cursor(2,1);
			lcd_puts("                 ");
			_delay_ms(500);
		}
	}
}

/*
void Counter_Sensor(){
	uint8_t counter_bef = 1, counter_act;
	GPIO_PIN_MODE_PORTD(0, INPUT);
	GPIO_PULLUP(PORTD,0,HIGH);

	counter_act = GPIO_READ_PORTD(0);
	if(counter_bef == 0 && counter_act == 1){
		if(contador_personas>= 8){
			contador_personas= 0;
		}
		contador_personas++;
		_delay_ms(100);
	}
	counter_bef = counter_act;
	_delay_ms(20);
}
*/
