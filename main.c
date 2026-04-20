/* ─────────────────────────────
 * INCLUDES
 * ───────────────────────────── */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "driver/ledc.h"
#include "esp_rom_sys.h"
#include <stdio.h>

/* ─────────────────────────────
 * DEFINES
 * ───────────────────────────── */
#define CENTENAS  GPIO_NUM_21
#define DECENAS   GPIO_NUM_22
#define UNIDADES  GPIO_NUM_23

#define POT_CANAL ADC1_CHANNEL_6  

#define LED1 GPIO_NUM_18
#define LED2 GPIO_NUM_19

#define BTN1 GPIO_NUM_4
#define BTN2 GPIO_NUM_5

#define PWM_GPIO 17

/* PWM */
#define PWM_FREQ 1000
#define PWM_RES  LEDC_TIMER_10_BIT
#define PWM_MAX  1023

/* ── PUENTE H ─── */
#define HB_IN1  GPIO_NUM_15
#define HB_IN2  GPIO_NUM_16
#define HB_IN3  GPIO_NUM_32
#define HB_IN4  GPIO_NUM_2

/* ─────────────────────────────
 * CONSTANTES
 * ───────────────────────────── */
static const gpio_num_t SEG_PINS[7] = {
    GPIO_NUM_13, GPIO_NUM_12, GPIO_NUM_14,
    GPIO_NUM_27, GPIO_NUM_26, GPIO_NUM_25,
    GPIO_NUM_33
};

static const gpio_num_t DIG_PINS[3] = {
    CENTENAS, DECENAS, UNIDADES
};

static const uint8_t TABLA[10][7] = {
    {0,0,0,0,0,0,1},  // 0
    {1,0,0,1,1,1,1},  // 1
    {0,0,1,0,0,1,0},  // 2
    {0,0,0,0,1,1,0},  // 3
    {1,0,0,1,1,0,0},  // 4
    {0,1,0,0,1,0,0},  // 5
    {0,1,0,0,0,0,0},  // 6
    {0,0,0,1,1,1,1},  // 7
    {0,0,0,0,0,0,0},  // 8
    {0,0,0,0,1,0,0}   // 9
};

/* ─────────────────────────────
 * VARIABLES
 * ───────────────────────────── */
uint8_t digitos[3] = {0, 0, 0};

int estado_led1 = 0;
int estado_led2 = 0;

int btn1_last = 1;
int btn2_last = 1;

/* ─────────────────────────────
 * PWM INIT
 * ───────────────────────────── */
void pwm_init(void)
{
    ledc_timer_config_t timer = {
        .speed_mode      = LEDC_LOW_SPEED_MODE,
        .timer_num       = LEDC_TIMER_0,
        .duty_resolution = PWM_RES,
        .freq_hz         = PWM_FREQ,
        .clk_cfg         = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer);

    ledc_channel_config_t ch = {
        .gpio_num   = PWM_GPIO,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = LEDC_CHANNEL_0,
        .timer_sel  = LEDC_TIMER_0,
        .duty       = 0,
        .hpoint     = 0
    };
    ledc_channel_config(&ch);
}

/* ─────────────────────────────
 * SET PWM
 * ───────────────────────────── */
void set_pwm(uint32_t duty)
{
    if(duty > PWM_MAX) duty = PWM_MAX;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

/* ─────────────────────────────
 * PUENTE H – INIT
 * ───────────────────────────── */
void hbridge_init(void)
{
    ledc_timer_config_t timer = {
        .speed_mode      = LEDC_LOW_SPEED_MODE,
        .timer_num       = LEDC_TIMER_1,
        .duty_resolution = PWM_RES,
        .freq_hz         = PWM_FREQ,
        .clk_cfg         = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer);

   
    ledc_channel_config_t ch1 = {
        .gpio_num   = HB_IN1,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = LEDC_CHANNEL_1,
        .timer_sel  = LEDC_TIMER_1,
        .duty       = PWM_MAX,
        .hpoint     = 0
    };
    ledc_channel_config(&ch1);

   
    ledc_channel_config_t ch2 = {
        .gpio_num   = HB_IN2,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = LEDC_CHANNEL_2,
        .timer_sel  = LEDC_TIMER_1,
        .duty       = 0,
        .hpoint     = 0
    };
    ledc_channel_config(&ch2);

  
    gpio_reset_pin(HB_IN3);
    gpio_set_direction(HB_IN3, GPIO_MODE_OUTPUT);
    gpio_set_level(HB_IN3, 1);

   
    gpio_reset_pin(HB_IN4);
    gpio_set_direction(HB_IN4, GPIO_MODE_OUTPUT);
    gpio_set_level(HB_IN4, 0);
}

/* ─────────────────────────────
 * PUENTE H – CONTROL
 * ───────────────────────────── */
void hbridge_set(int dir, uint32_t duty)
{
    if(duty > PWM_MAX) duty = PWM_MAX;

    uint32_t duty_p = PWM_MAX - duty;   // invertido para P-channel

    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, PWM_MAX);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2);
    gpio_set_level(HB_IN3, 1);
    gpio_set_level(HB_IN4, 0);
    esp_rom_delay_us(10);

    switch(dir)
    {
        case 1:  /* ── Giro HORARIO ── */
            gpio_set_level(HB_IN4, 1);
            ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, duty_p);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
            break;

        case 2:  /* ── Giro ANTIHORARIO ── */
            gpio_set_level(HB_IN3, 0);
            ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, duty);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2);
            break;

        default: /* ── FRENO ── ya todo apagado arriba */
            break;
    }
}

/* ─────────────────────────────
 * MAIN
 * ───────────────────────────── */
void app_main(void)
{
    for(int i = 0; i < 7; i++){
        gpio_reset_pin(SEG_PINS[i]);
        gpio_set_direction(SEG_PINS[i], GPIO_MODE_OUTPUT);
        gpio_set_level(SEG_PINS[i], 1);   // CA: apagado = HIGH
    }

    for(int i = 0; i < 3; i++){
        gpio_reset_pin(DIG_PINS[i]);
        gpio_set_direction(DIG_PINS[i], GPIO_MODE_OUTPUT);
        gpio_set_level(DIG_PINS[i], 0);   // CA: inactivo = LOW
    }

    /* LEDS */
    gpio_set_direction(LED1, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED2, GPIO_MODE_OUTPUT);
    gpio_set_level(LED1, 0);
    gpio_set_level(LED2, 0);

    /* BOTONES */
    gpio_set_direction(BTN1, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BTN1, GPIO_PULLUP_ONLY);
    gpio_set_direction(BTN2, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BTN2, GPIO_PULLUP_ONLY);


    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(POT_CANAL, ADC_ATTEN_DB_12);

    pwm_init();

    hbridge_init();

    int i = 0;

    while(1)
    {
        uint32_t adc_val = adc1_get_raw(POT_CANAL);

        
        uint32_t duty = (adc_val * PWM_MAX) / 4095;
        set_pwm(duty);

        uint32_t valor = (adc_val * 100 + 2047) / 4095;
        if(valor > 100) valor = 100;

        digitos[0] = valor / 100;          // centenas: 0 o 1
        digitos[1] = (valor % 100) / 10;   // decenas:  0–9
        digitos[2] = valor % 10;           // unidades: 0–9

        int btn1 = gpio_get_level(BTN1);
        int btn2 = gpio_get_level(BTN2);

        if(btn1 == 0 && btn1_last == 1){
            /* Freno 1 segundo antes de cambiar dirección */
            hbridge_set(0, 0);
            vTaskDelay(pdMS_TO_TICKS(1000));
            estado_led1 = 1;
            estado_led2 = 0;
        }
        if(btn2 == 0 && btn2_last == 1){
            /* Freno 1 segundo antes de cambiar dirección */
            hbridge_set(0, 0);
            vTaskDelay(pdMS_TO_TICKS(1000));
            estado_led2 = 1;
            estado_led1 = 0;
        }

        btn1_last = btn1;
        btn2_last = btn2;

        gpio_set_level(LED1, estado_led1);
        gpio_set_level(LED2, estado_led2);

        /* ───── PUENTE H ───── */
        int dir = estado_led1 ? 1 :
                  estado_led2 ? 2 : 0;
        hbridge_set(dir, duty);

        /* ───── MULTIPLEX ───── */
        gpio_set_level(CENTENAS, 0);
        gpio_set_level(DECENAS,  0);
        gpio_set_level(UNIDADES, 0);

        for(int s = 0; s < 7; s++)
            gpio_set_level(SEG_PINS[s], 1);

        esp_rom_delay_us(50); 

        for(int s = 0; s < 7; s++)
            gpio_set_level(SEG_PINS[s], TABLA[digitos[i]][s]);

        gpio_set_level(DIG_PINS[i], 1);

        vTaskDelay(pdMS_TO_TICKS(2));

        i = (i + 1) % 3;
    }
}