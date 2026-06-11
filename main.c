#define F_CPU 16000000UL
#include "libs/config.h"
#include "libs/gpio.h"
#include "libs/interrupt.h"
#include "libs/lcd.h"
#include "libs/timer.h"


#define SERVO_MIN      2000
#define SERVO_MID      3000
#define SERVO_MAX      4000
#define Pin_servo      5        /* PD5 = OC1A (Timer1 CH_A) */
#define Motor_adelante 1        /* PC1 -> base Q1 -> IN1 */
#define Motor_Atras    2        /* PC2 -> base Q2 -> IN2 */
#define Pin_PWM_Motor  4        /* PD4 = OC1B (Timer1 CH_B) -> ENA */
#define Velocidad      120

volatile int     contador_personas = 8;
volatile int     contador_vueltas  = 0;
volatile uint8_t tick_500ms        = 0;
volatile uint8_t bandera_conteo    = 0;
volatile uint8_t bandera_motor     = 0;


static void PaBajo(void){
	if (!bandera_conteo) return;

	if (++tick_500ms < 50) return;
	tick_500ms = 0;

	if (contador_personas > 0){
		contador_personas--;
	}
}


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

/*
 Etapa 2N2222 (emisor comun = inversor):
   PC HIGH -> transistor conduce -> IN del L298N cae a GND  (IN = LOW)
   PC LOW  -> transistor en corte -> pull-up 10k lleva IN a +5V (IN = HIGH)
*/
void Motor_init(void){
	GPIO_PIN_MODE_PORTC(Motor_adelante, OUTPUT);
	GPIO_PIN_MODE_PORTC(Motor_Atras,    OUTPUT);
	GPIO_PIN_MODE_PORTD(Pin_PWM_Motor,  OUTPUT);
	HAL_Timer_PWM_Enable(HAL_TIMER1, HAL_TIMER_CH_B);
	HAL_Timer_PWM_SetRaw(HAL_TIMER1, HAL_TIMER_CH_B, 0);

	/* Arranque seguro: ambos en corte -> IN1=IN2=HIGH -> motor detenido */
	GPIO_WRITE_PORTC(Motor_adelante, LOW);
	GPIO_WRITE_PORTC(Motor_Atras,    LOW);
}

void PWM_Motor_Adelante(void){
	if (!bandera_motor) return;
	/* Objetivo L298N: IN1=HIGH, IN2=LOW */
	GPIO_WRITE_PORTC(Motor_adelante, LOW);   /* PC1 LOW  -> Q1 corte   -> IN1=HIGH */
	GPIO_WRITE_PORTC(Motor_Atras,    HIGH);  /* PC2 HIGH -> Q2 conduce -> IN2=LOW  */
	HAL_Timer_PWM_SetRaw(HAL_TIMER1, HAL_TIMER_CH_B, Velocidad);
}

void PWM_Motor_Atras(void){
	if (!bandera_motor) return;
	/* Objetivo L298N: IN1=LOW, IN2=HIGH */
	GPIO_WRITE_PORTC(Motor_adelante, HIGH);  /* PC1 HIGH -> Q1 conduce -> IN1=LOW  */
	GPIO_WRITE_PORTC(Motor_Atras,    LOW);   /* PC2 LOW  -> Q2 corte   -> IN2=HIGH */
	HAL_Timer_PWM_SetRaw(HAL_TIMER1, HAL_TIMER_CH_B, Velocidad);
}

void Detener_Motor(void){
	/* Ambos en corte -> IN1=IN2=HIGH, ENA=0 -> motor completamente detenido */
	GPIO_WRITE_PORTC(Motor_adelante, LOW);
	GPIO_WRITE_PORTC(Motor_Atras,    LOW);
	HAL_Timer_PWM_SetRaw(HAL_TIMER1, HAL_TIMER_CH_B, 0);
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

void Actualizar_lcd(void){
	lcd_set_cursor(2, 1);
	lcd_printf("Personas: %-2d", contador_personas);
	lcd_set_cursor(3, 1);
	lcd_printf("Vueltas:  %-2d", contador_vueltas);
}

int main(void)
{
	MCUSR &= ~(1 << WDRF);

	PWM_servo();
	Motor_init();
	Timer_contador();
	lcd_init();
	_delay_ms(10);
	lcd_clear();
	_delay_ms(5);
	lcd_disable_blink();
	lcd_disable_cursor();

	HAL_IRQ_ENABLE();

	while (1)
	{
		/* --- Reset de estado --- */
		bandera_motor     = 0;   /* motor bloqueado antes de tocar nada fisico */
		bandera_conteo    = 0;
		contador_personas = 8;
		contador_vueltas  = 0;
		tick_500ms        = 0;

		Detener_Motor();         /* garantia hardware: pines y ENA a cero */
		Servo_abrir();

		lcd_clear();
		lcd_set_cursor(1, 1);
		lcd_printf("_MUERTE");
		Actualizar_lcd();

		/* --- Cuenta regresiva --- */
		bandera_conteo = 1;
		while (contador_personas > 0){
			Actualizar_lcd();
			_delay_ms(100);
		}
		bandera_conteo = 0;
		Actualizar_lcd();

		/* --- Cierre y arranque del motor --- */
		Servo_cerrar();
		bandera_motor = 1;

		/* --- 2 vueltas: adelante -> pausa -> atras --- */
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

		/* --- Motor completamente apagado antes de reiniciar --- */
		bandera_motor = 0;
		Detener_Motor();
		_delay_ms(500);   
		return 0;       /* pausa minima antes del siguiente ciclo */
	}
}