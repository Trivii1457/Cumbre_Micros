# Torre — Réplica del juego "La Cumbre"

Réplica a baja escala de un juego mecánico de torre, sobre **ATmega1284P**.
El juego cuenta pasajeros, cierra la compuerta (servo), suena un buzzer y mueve
la torre arriba/abajo dos veces; toda la información se muestra en una LCD y se
reinicia con un botón.

---

## 1. Plataforma

| Parámetro | Valor |
|---|---|
| Microcontrolador | ATmega1284P |
| Frecuencia (F_CPU) | 16 MHz |
| Lenguaje | C (avr-gcc) |
| IDE | Microchip Studio |

---

## 2. Periféricos y uso de timers

| Timer | Modo | Uso |
|---|---|---|
| Timer0 | CTC + IRQ Compare A | Conteo de pasajeros (tick ~10 ms, 50 ticks = 500 ms) |
| Timer1 | Fast PWM 50 Hz (top 39999, presc. 8) | Servo (OC1A = PD5) |
| Timer2 | Fast PWM (top 255, presc. 8) | Velocidad del motor (OC2A = PD7) |
| INT0 | Interrupción externa (flanco de bajada) | Botón de reinicio → reset por watchdog |

---

## 3. Mapa de pines

| Señal | Pin MCU | Detalle |
|---|---|---|
| Servo (señal) | **PD5** | OC1A, PWM 50 Hz |
| Motor IN1 (adelante) | **PC1** | dirección |
| Motor IN2 (atrás) | **PC2** | dirección |
| Motor ENA (velocidad) | **PD7** | OC2A, PWM (duty `Velocidad = 120`) |
| Buzzer (+) | **PB0** | buzzer activo, ON/OFF |
| Botón reinicio | **PD2** | INT0, pull-up interno, a GND |
| LCD RS | **PA0** | |
| LCD EN | **PA1** | |
| LCD D4 | **PA4** | |
| LCD D5 | **PA5** | |
| LCD D6 | **PA6** | |
| LCD D7 | **PA7** | |

---

## 4. Lógica del juego

| Fase | Servo | Motor | LCD |
|---|---|---|---|
| **Abordaje** | abierto | apagado (`bandera_motor = 0`) | Personas 8→0 (resta 1 cada 500 ms), Vueltas 0 |
| **Cierre** | cerrado | apagado + buzzer 300 ms | — |
| **Habilitación** | cerrado | `bandera_motor = 1` | — |
| **Movimiento** | cerrado | 2× [adelante 1200 ms → para 3 s → atrás 1200 ms → para] | Vueltas 1, 2 |
| **Fin** | cerrado | parado por completo | LCD limpia, solo "Presiona REINICIO" parpadeando |

- El **motor está apagado hasta que se cierra el servo**, garantizado por orden de
  ejecución y por el flag `bandera_motor` (las funciones de motor retornan si el
  flag está en 0).
- El **botón de reinicio** (INT0) provoca un **reset real del micro** vía watchdog,
  por lo que reinicia el juego completo desde cualquier punto.

---

## 5. Diagrama de conexión

### 5.1 Motor — Módulo L298N

```
   L298N                         ATmega1284P
 ┌──────────────┐
 │ IN1 ─────────┼────────────────  PC1
 │ IN2 ─────────┼────────────────  PC2
 │ ENA ─────────┼────────────────  PD7 (OC2A)   ← quitar jumper de ENA
 │ OUT1 ────────┼────────────────  Motor (+)
 │ OUT2 ────────┼────────────────  Motor (-)
 │ +12V ────────┼────────────────  Fuente del motor (+)
 │ GND ─────────┼────────────────  GND común (micro + fuente)
 │ +5V ─────────┼────────────────  ver nota
 └──────────────┘
```

**Notas L298N**
- Jumper del regulador interno ("5V-EN"): motor **≤ 12 V** → déjalo puesto (genera 5 V
  en el pin `+5V`); motor **> 12 V** → quítalo y alimenta `+5V` aparte.
- **GND siempre común** entre micro, L298N y fuente del motor.
- Si gira al revés: intercambia `OUT1/OUT2` o invierte `HIGH`/`LOW` en las funciones.

### 5.2 Servo

```
 Servo            ATmega1284P / Fuente
 ┌─────┐
 │ SIG ┼──────────  PD5
 │ VCC ┼──────────  +5V
 │ GND ┼──────────  GND común
 └─────┘
```
Si el servo consume mucho, aliméntalo de una fuente de 5 V aparte (GND común).

### 5.3 Buzzer (activo)

```
 Buzzer
 ┌─────┐
 │  +  ┼──────────  PB0
 │  -  ┼──────────  GND
 └─────┘
```

### 5.4 Botón de reinicio

```
 PD2 ───────[ pulsador ]─────── GND
```
Pull-up interno activado en el firmware; al presionar lleva PD2 a LOW → INT0
(flanco de bajada) → reset por watchdog.

### 5.5 LCD 20x4 (4 bits, PORTA)

```
 LCD            ATmega1284P
 RS ───────────  PA0
 EN ───────────  PA1
 D4 ───────────  PA4
 D5 ───────────  PA5
 D6 ───────────  PA6
 D7 ───────────  PA7
 VSS ──────────  GND
 VDD ──────────  +5V
 V0  ──────────  potenciómetro de contraste (entre +5V y GND)
 RW  ──────────  GND
 A (LED+) ─────  +5V (con resistencia)
 K (LED-) ─────  GND
```

---

## 6. Estructura del código (`main.c`)

| Función | Rol |
|---|---|
| `PaBajo()` | ISR Timer0: resta un pasajero cada 500 ms (solo si `bandera_conteo`) |
| `Reiniciar()` | ISR INT0: activa watchdog → reset del micro |
| `PWM_servo()` | Inicializa el servo (Timer1) |
| `Servo_abrir()` / `Servo_cerrar()` | Mueve el servo a posición abierta/cerrada |
| `Motor_init()` | Inicializa pines y PWM del motor (Timer2) |
| `PWM_Motor_Adelante()` / `PWM_Motor_Atras()` | Gira el motor (gated por `bandera_motor`) |
| `Detener_Motor()` | Frena el motor (dirección LOW + duty 0) |
| `Timer_contador()` | Configura Timer0 y su IRQ |
| `Boton_init()` | Configura INT0 (botón de reinicio) |
| `Buzzer_init()` / `Buzzer_sonar()` | Buzzer en PB0, suena 300 ms |
| `Menu_lcd()` / `Actualizar_lcd()` | Inicializa y refresca la LCD |
| `main()` | Inicializa todo y corre las fases del juego |

### Variables globales

| Variable | Significado |
|---|---|
| `contador_personas` | Pasajeros restantes (8 → 0) |
| `contador_vueltas` | Vueltas completadas (0 → 2) |
| `tick_500ms` | Contador de ticks del Timer0 |
| `bandera_conteo` | 1 = fase de abordaje (la ISR resta pasajeros) |
| `bandera_motor` | 1 = motor habilitado (solo tras cerrar el servo) |

### Constantes ajustables

| Define | Valor | Qué hace |
|---|---|---|
| `Velocidad` | 120 | Duty del PWM del motor (0–255) |
| `SERVO_MIN` / `SERVO_MAX` | 2000 / 4000 | Posición abierta / cerrada del servo |

---

## 7. Cómo probar

1. Cablear según el diagrama (recordar **GND común** en todas las fuentes).
2. Compilar y cargar en el ATmega1284P (16 MHz).
3. Al iniciar: servo abierto, LCD muestra los pasajeros bajando 8 → 0.
4. En 0: servo cierra, suena el buzzer, el motor hace 2 vueltas.
5. Al terminar: LCD limpia con "Presiona REINICIO" parpadeando.
6. Presionar el botón (PD2) para reiniciar el juego.
