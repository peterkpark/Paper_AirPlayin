#include "driver/mcpwm.h"
#include "soc/mcpwm_reg.h"
#include "soc/mcpwm_struct.h"
#define GPIO_PWM0A_OUT 19   //HV
#define GPIO_PWM1A_OUT 18   //HV

int freq0 = 50000;
float duty0 = 100;
float duty1 = 50;

void setuppwm(){
   mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, GPIO_PWM0A_OUT);
   mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM1A, GPIO_PWM1A_OUT);
 mcpwm_config_t pwm_config0;
     pwm_config0.frequency = freq0;  //frequency 
     pwm_config0.cmpr_a = 0;      //duty cycle of PWMxA = 50.0%
     pwm_config0.cmpr_b = 0;      //duty cycle of PWMxB = 50.0%
     pwm_config0.counter_mode = MCPWM_UP_COUNTER; // Up-down counter (triangle wave)
     pwm_config0.duty_mode = MCPWM_DUTY_MODE_0; // Active HIGH
   mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config0);    //Configure PWM0A & PWM0B with above settings
   mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_1, &pwm_config0);    //Configure PWM0A & PWM0B with above settings
   delay(20);
   mcpwm_set_frequency(MCPWM_UNIT_1, MCPWM_TIMER_1,freq0);
   mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_GEN_A, duty0);
   mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_1, MCPWM_GEN_A, duty1);
   
 }
void setup() {
 setuppwm();
}

void loop(){
  
}
