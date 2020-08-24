/*
* This file is part of the hoverboard-firmware-hack project.
*
* Copyright (C) 2017-2018 Rene Hopf <renehopf@mac.com>
* Copyright (C) 2017-2018 Nico Stute <crinq@crinq.de>
* Copyright (C) 2017-2018 Niklas Fauth <niklas.fauth@kit.fail>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "stm32f1xx_hal.h"
#include "defines.h"
#include "setup.h"
#include "config.h"
//#include "hd44780.h"

// ###############################################################################
#include "BLDC_controller.h"            /* Model's header file */
#include "rtwtypes.h"

RT_MODEL rtM_Left_;    /* Real-time model */
RT_MODEL rtM_Right_;   /* Real-time model */
RT_MODEL *const rtM_Left = &rtM_Left_;
RT_MODEL *const rtM_Right = &rtM_Right_;

P rtP;                           /* Block parameters (auto storage) */

DW rtDW_Left;                    /* Observable states */
ExtU rtU_Left;                   /* External inputs */
ExtY rtY_Left;                   /* External outputs */

DW rtDW_Right;                   /* Observable states */
ExtU rtU_Right;                  /* External inputs */
ExtY rtY_Right;                  /* External outputs */
// ###############################################################################


void SystemClock_Config(void);

extern TIM_HandleTypeDef htim_left;
extern TIM_HandleTypeDef htim_right;
extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc2;
extern volatile adc_buf_t adc_buffer;
//LCD_PCF8574_HandleTypeDef lcd;
extern I2C_HandleTypeDef hi2c2;
extern UART_HandleTypeDef huart2;

#ifdef DEBUG_I2C_LCD
extern I2C_HandleTypeDef hi2c2;
#endif

#ifdef CONTROL_SERIAL_USART2
extern UART_HandleTypeDef huart2;
#endif

int cmd1;  // normalized input values. -1000 to 1000
int cmd2;
int cmd3;

typedef struct{
   int16_t steer;
   int16_t speed;
   //uint32_t crc;
} Serialcommand;

volatile Serialcommand command;

uint8_t button1, button2;

int steer; // global variable for steering. -1000 to 1000
int speed; // global variable for speed. -1000 to 1000

float local_speed_coefficent;
float local_steer_coefficent;

extern volatile int pwml;  // global variable for pwm left. -1000 to 1000
extern volatile int pwmr;  // global variable for pwm right. -1000 to 1000
//extern volatile int weakl; // global variable for field weakening left. -1000 to 1000
//extern volatile int weakr; // global variable for field weakening right. -1000 to 1000

extern uint8_t buzzerFreq;    // global variable for the buzzer pitch. can be 1, 2, 3, 4, 5, 6, 7...
extern uint8_t buzzerPattern; // global variable for the buzzer pattern. can be 1, 2, 3, 4, 5, 6, 7...

extern uint8_t enable; // global variable for motor enable

extern volatile uint32_t timeout; // global variable for timeout
extern float batteryVoltage; // global variable for battery voltage

uint32_t inactivity_timeout_counter;

#ifdef CONTROL_NUNCHUCK
extern uint8_t nunchuck_data[6];
#endif

#ifdef CONTROL_PPM
extern volatile uint16_t ppm_captured_value[PPM_NUM_CHANNELS+1];
#endif

#ifdef CONTROL_DETECT_HALL
typedef enum
{
	wait = 0,
	beep = 1,
	spin = 2
} motor_test_state;

typedef struct
{
	motor_test_state state;
	uint16_t time;
	uint16_t timeout;
	uint16_t hall_cfg;
	uint16_t beep_cnt;
	uint16_t beep_on;
} motor_test_admin;

motor_test_admin tst_admin = {0,0,1000/DELAY_IN_MAIN_LOOP,5,0,0};
extern volatile uint8_t hall_idx_left;
extern volatile uint8_t hall_idx_right;
#endif

int milli_vel_error_sum = 0;


void poweroff() {
    if (ABS(speed) < 20) {
        buzzerPattern = 0;
        enable = 0;
        for (int i = 0; i < 8; i++) {
            buzzerFreq = i;
            HAL_Delay(100);
        }
        HAL_GPIO_WritePin(OFF_PORT, OFF_PIN, 0);
        while(1) {}
    }
}


int main(void) {
  HAL_Init();

  __HAL_RCC_AFIO_CLK_ENABLE();
  HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
  /* System interrupt init*/
  /* MemoryManagement_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(MemoryManagement_IRQn, 0, 0);
  /* BusFault_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(BusFault_IRQn, 0, 0);
  /* UsageFault_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(UsageFault_IRQn, 0, 0);
  /* SVCall_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SVCall_IRQn, 0, 0);
  /* DebugMonitor_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DebugMonitor_IRQn, 0, 0);
  /* PendSV_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(PendSV_IRQn, 0, 0);
  /* SysTick_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);

  SystemClock_Config();
  __HAL_RCC_DMA1_CLK_DISABLE();

  MX_GPIO_Init();
  MX_TIM_Init();
  MX_ADC1_Init();
  MX_ADC2_Init();

  #if defined(DEBUG_SERIAL_USART2) || defined(DEBUG_SERIAL_USART3)
    UART_Init();
  #endif

  HAL_GPIO_WritePin(OFF_PORT, OFF_PIN, 1);

  HAL_ADC_Start(&hadc1);
  HAL_ADC_Start(&hadc2);

// Matlab Init
// ###############################################################################
  
  /* Set BLDC controller parameters */  
  rtP.z_ctrlTypSel        = CTRL_TYP_SEL;
  rtP.b_phaAdvEna         = PHASE_ADV_ENA;  
  
  /* Pack LEFT motor data into RTM */
  rtM_Left->defaultParam  = &rtP;
  rtM_Left->dwork         = &rtDW_Left;
  rtM_Left->inputs        = &rtU_Left;
  rtM_Left->outputs       = &rtY_Left;

  /* Pack RIGHT motor data into RTM */
  rtM_Right->defaultParam = &rtP;
  rtM_Right->dwork        = &rtDW_Right;
  rtM_Right->inputs       = &rtU_Right;
  rtM_Right->outputs      = &rtY_Right;

  /* Initialize BLDC controllers */
  BLDC_controller_initialize(rtM_Left);
  BLDC_controller_initialize(rtM_Right);


  for (int i = 8; i >= 0; i--) {
    buzzerFreq = i;
    HAL_Delay(100);
  }
  buzzerFreq = 0;

  HAL_GPIO_WritePin(LED_PORT, LED_PIN, 1);

  #if defined (CONTROL_MOTOR_TEST) || defined (CONTROL_DETECT_HALL)
  #else
  int lastSpeedL = 0, lastSpeedR = 0;
  #endif

  int speedL = 0, speedR = 0;
  float direction = 1;
  local_speed_coefficent = SPEED_COEFFICIENT;
  local_steer_coefficent = STEER_COEFFICIENT;

  #ifdef CONTROL_PPM
    PPM_Init();
  #endif

  #ifdef CONTROL_NUNCHUCK
    I2C_Init();
    Nunchuck_Init();
  #endif

  #ifdef CONTROL_SERIAL_USART2
    UART_Control_Init();
    HAL_UART_Receive_DMA(&huart2, (uint8_t *)&command, 4);
  #endif

  #ifdef DEBUG_I2C_LCD
    I2C_Init();
    HAL_Delay(50);
    lcd.pcf8574.PCF_I2C_ADDRESS = 0x27;
      lcd.pcf8574.PCF_I2C_TIMEOUT = 5;
      lcd.pcf8574.i2c = hi2c2;
      lcd.NUMBER_OF_LINES = NUMBER_OF_LINES_2;
      lcd.type = TYPE0;

      if(LCD_Init(&lcd)!=LCD_OK){
          // error occured
          //TODO while(1);
      }

    LCD_ClearDisplay(&lcd);
    HAL_Delay(5);
    LCD_SetLocation(&lcd, 0, 0);
    LCD_WriteString(&lcd, "Hover V2.0");
    LCD_SetLocation(&lcd, 0, 1);
    LCD_WriteString(&lcd, "Initializing...");
  #endif

  float board_temp_adc_filtered = (float)adc_buffer.temp;
  float board_temp_deg_c;

  //wait for other RC to startup HW and make a noise when ready
  //20 samples 5ms * 20 = 100ms of "neutral position"
  int RCOKcounter = 0;
  while(RCOKcounter < 20)
  {
    int adcstartup1 = CLAMP(adc_buffer.l_tx2 - ADC1_MIN, 0, ADC1_MAX);  // ADC1
    int adcstartup2 = CLAMP(adc_buffer.l_rx2 - ADC2_MIN, 0, ADC2_MAX);  // ADC2
    if(ABS(adcstartup1 - ADC1_MID) < ADC_DEAD_BAND &&
       ABS(adcstartup2 - ADC2_MID) < ADC_DEAD_BAND)
    {
      RCOKcounter++;
    }

    // ####### POWEROFF BY POWER-BUTTON #######
    if (HAL_GPIO_ReadPin(BUTTON_PORT, BUTTON_PIN)) {
      enable = 0;
      while (HAL_GPIO_ReadPin(BUTTON_PORT, BUTTON_PIN)) {}
      poweroff();
    }
    HAL_Delay(DELAY_IN_MAIN_LOOP);
  }
  
  for (int i = 4; i >= 0; i--) {
    buzzerFreq = i;
    HAL_Delay(100);
  }
  buzzerFreq = 0;
  //end RC rdy

  enable = 1;  // enable motors

  while(1) {
    HAL_Delay(DELAY_IN_MAIN_LOOP); //delay in ms

    #ifdef CONTROL_NUNCHUCK
      Nunchuck_Read();
      cmd1 = CLAMP((nunchuck_data[0] - 127) * 8, -1000, 1000); // x - axis. Nunchuck joystick readings range 30 - 230
      cmd2 = CLAMP((nunchuck_data[1] - 128) * 8, -1000, 1000); // y - axis

      button1 = (uint8_t)nunchuck_data[5] & 1;
      button2 = (uint8_t)(nunchuck_data[5] >> 1) & 1;
    #endif

    #ifdef CONTROL_PPM
		if(ABS(ppm_captured_value[0] - 500) > PPM_DEAD_BAND)
			cmd1 = CLAMP((ppm_captured_value[0] - 500) * 2, -1000, 1000);
		else 
			cmd1 = 0;
		if(ABS(ppm_captured_value[1] - 500) > PPM_DEAD_BAND)
			cmd2 = CLAMP((ppm_captured_value[1] - 500) * 2, -1000, 1000);
		else
			cmd2 = 0;
      button1 = ppm_captured_value[5] > 500;
      //float scale = ppm_captured_value[2] / 1000.0f;
	  //PPM input goes from 0 to 1000, map this to 0 to 2
	  local_speed_coefficent = ppm_captured_value[3] / 500.0f;
	  local_steer_coefficent = ppm_captured_value[4] / 1000.0f;
    #endif

    #ifdef CONTROL_ADC
      // ADC values range: 0-4095, see ADC-calibration in config.h
      cmd1 = CLAMP(adc_buffer.l_tx2 - ADC1_MIN, 0, ADC1_MAX);  // ADC1
      cmd2 = CLAMP(adc_buffer.l_rx2 - ADC2_MIN, 0, ADC2_MAX);  // ADC2
	    if(ABS(cmd1 - ADC1_MID) > ADC_DEAD_BAND)
			  cmd1 = CLAMP((cmd1 - ADC1_MID) * 2, -ADC1_MAX, ADC1_MAX) / (ADC1_MAX / 1000.0f); //  / (ADC2_MAX / 1000.0f
		  else 
			  cmd1 = 0;
	    if(ABS(cmd2 - ADC2_MID) > ADC_DEAD_BAND)
			  cmd2 = CLAMP((cmd2 - ADC2_MID) * 2, -ADC2_MAX, ADC2_MAX) / (ADC2_MAX / 1000.0f); //  / (ADC2_MAX / 1000.0f
		  else 
			  cmd2 = 0;
	
      // use ADCs as button inputs:
      button1 = (uint8_t)(adc_buffer.l_tx2 > 2000);  // ADC1
      button2 = (uint8_t)(adc_buffer.l_rx2 > 2000);  // ADC2

      timeout = 0;
    #endif

    #ifdef CONTROL_SERIAL_USART2
      cmd1 = CLAMP((int16_t)command.steer, -1000, 1000);
      cmd2 = CLAMP((int16_t)command.speed, -1000, 1000);

      timeout = 0;
    #endif


    // ####### LOW-PASS FILTER #######
    steer = steer * (1.0 - FILTER) + cmd1 * FILTER;
    speed = speed * (1.0 - FILTER) + cmd2 * FILTER;

#ifdef CONTROL_MOTOR_TEST
    //keep activity timeout happy
    speedR = 100;
    speedL = 100;

	#ifdef INVERT_R_DIRECTION
	  pwmr = 60;
	#else
	  pwmr = -60;
	#endif
	#ifdef INVERT_L_DIRECTION
	  pwml = -60;
	#else
	  pwml = 60;
	#endif

#elif defined CONTROL_DETECT_HALL

    //keep activity timeout happy
    speedR = 100;
    speedL = 100;

    //update timer
    tst_admin.time++;

    //continue test
    switch(tst_admin.state)
    {
		//wait until timeout, then set configuration and go to beep
		case wait:
			if(tst_admin.time == tst_admin.timeout)
			{
				tst_admin.hall_cfg = (tst_admin.hall_cfg + 1) % 6;
				hall_idx_left  = tst_admin.hall_cfg;
				hall_idx_right = tst_admin.hall_cfg;
				tst_admin.state = beep;
				tst_admin.beep_cnt = 0;
				tst_admin.beep_on  = 0;
				tst_admin.timeout = tst_admin.time + 500/DELAY_IN_MAIN_LOOP;
			}
			break;
		//wait until timeout to turn buzzer on or off, when turning buzzer off, increment nr_beeps
		//if number of beeps has been reached, go to spin
		case beep:
			if(tst_admin.time == tst_admin.timeout)
			{
				if(tst_admin.beep_on)
				{
					//turn buzzer off, increment beep_cnt, and set new timeout
					buzzerFreq = 0;
					tst_admin.beep_on = 0;
					tst_admin.beep_cnt++;
					tst_admin.timeout = tst_admin.time + 300/DELAY_IN_MAIN_LOOP;
				}
				else
				{
					//check beep_cnt. If count has been reached, spin 5s, else beep buzzer 0.5s
					if(tst_admin.beep_cnt == tst_admin.hall_cfg + 1)
					{
						#ifdef INVERT_R_DIRECTION
						  pwmr = 60;
						#else
						  pwmr = -60;
						#endif
						#ifdef INVERT_L_DIRECTION
						  pwml = -60;
						#else
						  pwml = 60;
						#endif
						tst_admin.state = spin;
						tst_admin.timeout = tst_admin.time + 3000/DELAY_IN_MAIN_LOOP;
					}
					else
					{
						buzzerFreq = 16;
						tst_admin.beep_on = 1;
						tst_admin.timeout = tst_admin.time + 100/DELAY_IN_MAIN_LOOP;
					}
				}
			}
			break;
		//wait until timeout to stop spinning
		case spin:
			if(tst_admin.time == tst_admin.timeout)
			{
				pwmr = 0;
				pwml = 0;
				tst_admin.state = wait;
				tst_admin.timeout = tst_admin.time + 750/DELAY_IN_MAIN_LOOP;
			}
			break;
		default:
			//do nothing
			break;
    }

#else

    // ####### MIXER #######
    speedR = CLAMP(speedR * (1.0 - FILTER) + cmd1 * FILTER,-1000,1000);
    speedL = CLAMP(speedL * (1.0 - FILTER) + cmd2 * FILTER,-1000,1000);

//    speedR = CLAMP(speed * local_speed_coefficent -  steer * local_steer_coefficent, -1000, 1000);
//    speedL = CLAMP(speed * local_speed_coefficent +  steer * local_steer_coefficent, -1000, 1000);


    // ####### SET OUTPUTS #######
    if ((speedL < lastSpeedL + 50 && speedL > lastSpeedL - 50) && (speedR < lastSpeedR + 50 && speedR > lastSpeedR - 50) && timeout < TIMEOUT) {
    #ifdef INVERT_R_DIRECTION
      pwmr = speedR;
    #else
      pwmr = -speedR;
    #endif
    #ifdef INVERT_L_DIRECTION
      pwml = -speedL;
    #else
      pwml = speedL;
    #endif
    }

    lastSpeedL = speedL;
    lastSpeedR = speedR;
#endif

    if (inactivity_timeout_counter % 25 == 0) {
      // ####### CALC BOARD TEMPERATURE #######
      board_temp_adc_filtered = board_temp_adc_filtered * 0.99 + (float)adc_buffer.temp * 0.01;
      board_temp_deg_c = ((float)TEMP_CAL_HIGH_DEG_C - (float)TEMP_CAL_LOW_DEG_C) / ((float)TEMP_CAL_HIGH_ADC - (float)TEMP_CAL_LOW_ADC) * (board_temp_adc_filtered - (float)TEMP_CAL_LOW_ADC) + (float)TEMP_CAL_LOW_DEG_C;
      
      // ####### DEBUG SERIAL OUT #######
      #ifdef CONTROL_ADC
        setScopeChannel(0, (int)adc_buffer.l_tx2);  // 1: ADC1
        setScopeChannel(1, (int)adc_buffer.l_rx2);  // 2: ADC2
      #endif
      setScopeChannel(2, (int)speedR);  // 3: output speed: 0-1000
      setScopeChannel(3, (int)speedL);  // 4: output speed: 0-1000
      setScopeChannel(4, (int)adc_buffer.batt1);  // 5: for battery voltage calibration
      setScopeChannel(5, (int)(batteryVoltage * 100.0f));  // 6: for verifying battery voltage calibration
      setScopeChannel(6, (int)board_temp_adc_filtered);  // 7: for board temperature calibration
      setScopeChannel(7, (int)board_temp_deg_c);  // 8: for verifying board temperature calibration
      consoleScope();
    }


    // ####### POWEROFF BY POWER-BUTTON #######
    if (HAL_GPIO_ReadPin(BUTTON_PORT, BUTTON_PIN)) {
      enable = 0;
      while (HAL_GPIO_ReadPin(BUTTON_PORT, BUTTON_PIN)) {}
      poweroff();
    }


    // ####### BEEP AND EMERGENCY POWEROFF #######
    if ((TEMP_POWEROFF_ENABLE && board_temp_deg_c >= TEMP_POWEROFF && ABS(speed) < 20) || (batteryVoltage < ((float)BAT_LOW_DEAD * (float)BAT_NUMBER_OF_CELLS) && ABS(speed) < 20)) {  // poweroff before mainboard burns OR low bat 3
      poweroff();
    } else if (TEMP_WARNING_ENABLE && board_temp_deg_c >= TEMP_WARNING) {  // beep if mainboard gets hot
      buzzerFreq = 4;
      buzzerPattern = 1;
    } else if (batteryVoltage < ((float)BAT_LOW_LVL1 * (float)BAT_NUMBER_OF_CELLS) && batteryVoltage > ((float)BAT_LOW_LVL2 * (float)BAT_NUMBER_OF_CELLS) && BAT_LOW_LVL1_ENABLE) {  // low bat 1: slow beep
      buzzerFreq = 5;
      buzzerPattern = 42;
    } else if (batteryVoltage < ((float)BAT_LOW_LVL2 * (float)BAT_NUMBER_OF_CELLS) && batteryVoltage > ((float)BAT_LOW_DEAD * (float)BAT_NUMBER_OF_CELLS) && BAT_LOW_LVL2_ENABLE) {  // low bat 2: fast beep
      buzzerFreq = 5;
      buzzerPattern = 6;
    } else if (BEEPS_BACKWARD && speed < -50) {  // backward beep
      buzzerFreq = 5;
      buzzerPattern = 1;
    } else {  // do not beep
      buzzerFreq = 0;
      buzzerPattern = 0;
    }


    // ####### INACTIVITY TIMEOUT #######
    if (ABS(speedL) > 50 || ABS(speedR) > 50) {
      inactivity_timeout_counter = 0;
    } else {
      inactivity_timeout_counter ++;
    }
    if (inactivity_timeout_counter > (INACTIVITY_TIMEOUT * 60 * 1000) / (DELAY_IN_MAIN_LOOP + 1)) {  // rest of main loop needs maybe 1ms
      poweroff();
    }
  }
}

/** System Clock Configuration
*/
void SystemClock_Config(void) {
#ifndef AT32F403Rx_HD
  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_PeriphCLKInitTypeDef PeriphClkInit;

  /**Initializes the CPU, AHB and APB busses clocks
    */
// #ifndef AT32F403Rx_HD
//Is automatically set at startup in SystemConfig 
  RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState            = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = 16;
  RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource       = RCC_PLLSOURCE_HSI_DIV2;
  RCC_OscInitStruct.PLL.PLLMUL          = RCC_PLL_MUL16;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);
  /**Initializes the CPU, AHB and APB busses clocks
    */
  RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
	

	// HAL_RCC_ClockConfig(&RCC_ClkInitStruct, 0);
// #else
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);

  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection    = RCC_ADCPCLK2_DIV8;  // 8 MHz
  HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit);
#endif
  /**Configure the Systick interrupt time
    */
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq() / 1000);

  /**Configure the Systick
    */
// #ifndef AT32F403Rx_HD
	//seems to not be available on AT32
  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);
// #endif
  /* SysTick_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}
