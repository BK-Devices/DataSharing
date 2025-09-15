/* USER CODE BEGIN Header */
// ============================================================
// File Name	: main.c
// Author		: Bhavesh
// Version		: V2.0.1
// Created on	: Jul 26, 2025
// ============================================================
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "crc.h"
#include "dma.h"
#include "memorymap.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include <math.h>
#include <string.h>
#include "struct.h"
#include "timDelay.h"
#include "variables.h"
#include "functions.h"
#include "flashData.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

	/* USER CODE BEGIN 1 */

	/* USER CODE END 1 */

	/* MCU Configuration--------------------------------------------------------*/

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* USER CODE BEGIN Init */

	/* USER CODE END Init */

	/* Configure the system clock */
	SystemClock_Config();

	/* Configure the peripherals common clocks */
	PeriphCommonClock_Config();

	/* USER CODE BEGIN SysInit */

	/* USER CODE END SysInit */

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_DMA_Init();
	MX_TIM1_Init();
	MX_TIM4_Init();
	MX_TIM13_Init();
	MX_ADC3_Init();
	MX_UART8_Init();
	MX_ADC1_Init();
	MX_ADC2_Init();
	MX_TIM23_Init();
	MX_CRC_Init();
	MX_TIM2_Init();
	MX_TIM3_Init();
	/* USER CODE BEGIN 2 */

	// Local Variables
	uint8_t i = 0;
	union _Offset_ Offset;

	// Init Motor GPIOs
	motorGPIOs_Init();

	// Enable DWT for nanoseconds delay
	DWT_Init();

	// Calibrate ADC for better accuracy
	HAL_ADCEx_Calibration_Start(&hadc1, ADC_CALIB_OFFSET_LINEARITY, ADC_SINGLE_ENDED);
	HAL_ADCEx_Calibration_Start(&hadc2, ADC_CALIB_OFFSET_LINEARITY, ADC_SINGLE_ENDED);
	HAL_ADCEx_Calibration_Start(&hadc3, ADC_CALIB_OFFSET_LINEARITY, ADC_SINGLE_ENDED);

	// Calculate Software Checksum
	SwCheckSum = (uint16_t)HAL_CRC_Calculate(&hcrc, (uint32_t *)StartAddress, AppSize);

	// Read Offset from flash & validate it
	if(readFlashData(SEC_ADDR, Offset.Buffer, 17) == 17)
	{
		if(Offset.OffsetStruct.CheckSum == calCheckSum(Offset.Buffer, 16))
		{
			if( (Vd150 <= Offset.OffsetStruct.Offset[0] && Offset.OffsetStruct.Offset[0] <= Vd170) &&
				(Vd150 <= Offset.OffsetStruct.Offset[1] && Offset.OffsetStruct.Offset[1] <= Vd170) &&
				(Vd150 <= Offset.OffsetStruct.Offset[2] && Offset.OffsetStruct.Offset[2] <= Vd170) &&
				(Vd150 <= Offset.OffsetStruct.Offset[3] && Offset.OffsetStruct.Offset[3] <= Vd170) )
			{

				Delta_Vd160[0] = Offset.OffsetStruct.Offset[0];
				Delta_Vd160[1] = Offset.OffsetStruct.Offset[1];
				Delta_Vd160[2] = Offset.OffsetStruct.Offset[2];
				Delta_Vd160[3] = Offset.OffsetStruct.Offset[3];
			}
		}
	}

	// Start OBC UART Dummy Receive Polling
	HAL_UART_Receive(ObcUart, ObcCmd.ObcCmdBuff, sizeof(ObcCmd.ObcCmdBuff), 2);

	// Start OBC UART Idle DMA for Data Receive
	HAL_UARTEx_ReceiveToIdle_DMA(ObcUart, ObcCmd.ObcCmdBuff, sizeof(ObcCmd.ObcCmdBuff));
	__HAL_DMA_DISABLE_IT(ObcUart->hdmarx, DMA_IT_HT);

	// Start 200us Timer Interrupt
	HAL_TIM_Base_Start_IT(&htim3);

	// Start ADC DMA Conversions
	HAL_ADC_Start_DMA(&hadc3, (uint32_t *)(AdcVal+4), 5);
	HAL_ADC_Start_DMA(&hadc1, (uint32_t *)(AdcVal+1), 3);
	HAL_ADC_Start_DMA(&hadc2, (uint32_t *)(AdcVal+0), 1);


	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while(1)
	{
		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */

		// Read Offsets of Potentiometer
		if(ReadOffsetFlag == 1)
		{
			ReadOffsetFlag = 0;

			// Stop ADC DMA Conversion
			HAL_ADC_Stop_DMA(&hadc1);
			HAL_ADC_Stop_DMA(&hadc2);
			HAL_ADC_Stop_DMA(&hadc3);

			// Read ADC Values in Polling for averaging
			for(i = 1; i <= 25; i++)
			{
				readADC();
				timDelayMS(1);

				if(i % 5 == 0)
				{
					timDelayS(1);
				}
			}

			Delta_Vd160[0] = (AdcVal[4] * 3.3f) / 4096.0f;
			Delta_Vd160[1] = (AdcVal[6] * 3.3f) / 4096.0f;
			Delta_Vd160[2] = (AdcVal[8] * 3.3f) / 4096.0f;
			Delta_Vd160[3] = (AdcVal[2] * 3.3f) / 4096.0f;

			// Check Offset are Valid or Not
			if( (Vd150 <= Delta_Vd160[0] && Delta_Vd160[0] <= Vd170) &&
				(Vd150 <= Delta_Vd160[1] && Delta_Vd160[1] <= Vd170) &&
				(Vd150 <= Delta_Vd160[2] && Delta_Vd160[2] <= Vd170) &&
				(Vd150 <= Delta_Vd160[3] && Delta_Vd160[3] <= Vd170) )
			{

				Offset.OffsetStruct.Offset[0] = Delta_Vd160[0];
				Offset.OffsetStruct.Offset[1] = Delta_Vd160[1];
				Offset.OffsetStruct.Offset[2] = Delta_Vd160[2];
				Offset.OffsetStruct.Offset[3] = Delta_Vd160[3];

				Offset.OffsetStruct.CheckSum = calCheckSum(Offset.Buffer, 16);

				if(eraseFlash_LastSector() ==  HAL_OK && writeFlashData(Offset.Buffer, 17) == 17)
				{
					sendAckNack(0, ReadOffset_CmdID);
				}
				else
				{
					sendAckNack(2, ReadOffset_CmdID);
				}
			}
			else
			{
				sendAckNack(2, ReadOffset_CmdID);
			}

			// Reset Controller
			timDelayS(1);
			NVIC_SystemReset();
		}


		// Wait for 200 us Timer Interrupt
		while(TimIntFlag == 0);
		TimIntFlag = 0;

		// Check Actuator Fault
		checkMotorFault();

		// Do Calculations & Operate Motors
		for(i = 0; i < 4; i++)
		{
			// Convert Angles from Degree to Radian's
			Delta_Pos[i] = DegToRad(ActPos[i]);
			Delta_Cmd[i] = DegToRad(CmdActpos[i]);

			// Angular Velocity
			Delta_Dot_FF[i] = (Delta_Pos[i] - Delta_Pos_Last[i]) / (0.0002f);
			Delta_Pos_Last[i] = Delta_Pos[i];

			// Summation of Rate and Position
			Delta_FB_Com[i] = (Delta_Pos[i] * Kpfb[i]) + (Delta_Dot_FF[i] * Krfb[i]);

			// LPF (Low Pass Filter)
			Delta_FB_Com_Fil[i] = (0.025154003643226f * Delta_FB_Com[i]) + (0.050308007286452f * Delta_FB_Com_t[i][0]) + (0.025154003643226f * Delta_FB_Com_t[i][1]) - ((-1.365599449644297f) * Delta_FB_Com_Fil_t[i][0]) - (0.466215464217202f * Delta_FB_Com_Fil_t[i][1]);

			// Calculate Position Error
			Delta_Error[i] = (Delta_Cmd[i] - Delta_FB_Com_Fil[i]) * Kpos[i];

			// Storing Last Calculated Values for LPF
			Delta_FB_Com_t[i][1] = Delta_FB_Com_t[i][0];
			Delta_FB_Com_t[i][0] = Delta_FB_Com[i];

			Delta_FB_Com_Fil_t[i][1] = Delta_FB_Com_Fil_t[i][0];
			Delta_FB_Com_Fil_t[i][0] = Delta_FB_Com_Fil[i];


			// Calculate PWM Duty Cycle
			PWM_DC[i] = Delta_Error[i] / MaxTorque[i];

			if(PWM_DC[i] < 0)
			{
				PWM_DC[i] = (Delta_Error[i] / MaxTorque[i]) * (-1.0f);
			}

			if(PWM_DC[i] > 0.98)
			{
				PWM_DC[i] = 0.98;
			}

			// Update PWM Duty Cycle in Timer
			PWMTim[i]->Instance->CCR1 = (uint32_t)((float)((uint32_t)__HAL_TIM_GET_AUTORELOAD(PWMTim[i])) * PWM_DC[i]);


			// Operate Motor
			if(Delta_Error[i] >= 0)
			{
				// Operate Motor in Backward Direction
				operateMotor(i, ActStatflag[i], 0);
			}
			else
			{
				// Operate Motor in Forward Direction
				operateMotor(i, ActStatflag[i], 1);
			}
		}
	}

	return 0;
	/* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 68;
  RCC_OscInitStruct.PLL.PLLP = 1;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_3;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 6144;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief Peripherals Common Clock Configuration
  * @retval None
  */
void PeriphCommonClock_Config(void)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Initializes the peripherals clock
  */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInitStruct.PLL2.PLL2M = 1;
  PeriphClkInitStruct.PLL2.PLL2N = 24;
  PeriphClkInitStruct.PLL2.PLL2P = 2;
  PeriphClkInitStruct.PLL2.PLL2Q = 2;
  PeriphClkInitStruct.PLL2.PLL2R = 2;
  PeriphClkInitStruct.PLL2.PLL2RGE = RCC_PLL2VCIRANGE_3;
  PeriphClkInitStruct.PLL2.PLL2VCOSEL = RCC_PLL2VCOWIDE;
  PeriphClkInitStruct.PLL2.PLL2FRACN = 0;
  PeriphClkInitStruct.AdcClockSelection = RCC_ADCCLKSOURCE_PLL2;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */


// ---------- ADC DMA Conversion Callback ----------
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
	static uint8_t AdcFlag[3] = {0, 0, 0};
	static float AdcVolt[9][5] = {0};
	static uint8_t AdcCnt = 0;

	float AdcVoltAvg[9] = {0};
	uint8_t i = 0, j = 0;

	if(hadc->Instance == ADC1)
	{
		AdcFlag[0] = 1;
	}
	else if(hadc->Instance == ADC2)
	{
		AdcFlag[1] = 1;
	}
	else if(hadc->Instance == ADC3)
	{
		AdcFlag[2] = 1;
	}
	else
	{

	}

	// Calculate Act Position, Motor Current & EMA Voltage
	if(AdcFlag[0] == 1 && AdcFlag[1] == 1 && AdcFlag[3] == 1)
	{
		AdcFlag[0] = 0;
		AdcFlag[1] = 0;
		AdcFlag[2] = 0;

		for(i = 0; i < 9; i++)
		{
			// Calculate ADC Voltage
			AdcVolt[i][AdcCnt] = (AdcVal[i] * 3.3f) / 4096.0f;

			// Calculate Average of ADC Voltage
			AdcVoltAvg[i] = 0.0f;
			for(j = 0; j < 5; j++)
			{
				AdcVoltAvg[i] += AdcVolt[i][j];
			}
			AdcVoltAvg[i] /= 5.0f;
		}

		if(++AdcCnt >= 5)
		{
			AdcCnt = 0;
		}


		// EMA Actuator Voltage
		EmaActVolt	= (AdcVoltAvg[3] - 0.0001f) * 11.0f / 1.0f;

		// Actuator 1
		ActCurr[0]	= AdcVoltAvg[0] * 160.0f;
		ActPos[0]	= (AdcVoltAvg[4] - Delta_Vd160[0]) / ((AdcVoltAvg[4] <= Delta_Vd160[0]) ? Delta_VN[0] : Delta_VP[0]);

		// Actuator 2
		ActCurr[1]	= AdcVoltAvg[5] * 160.0f;
		ActPos[1]	= (AdcVoltAvg[6] - Delta_Vd160[1]) / ((AdcVoltAvg[6] <= Delta_Vd160[1]) ? Delta_VN[1] : Delta_VP[1]);

		// Actuator 3
		ActCurr[2]	= AdcVoltAvg[7] * 160.0f;
		ActPos[2]	= (AdcVoltAvg[8] - Delta_Vd160[2]) / ((AdcVoltAvg[8] <= Delta_Vd160[2]) ? Delta_VN[2] : Delta_VP[2]);

		// Actuator 4
		ActCurr[3]	= AdcVoltAvg[1] * 160.0f;
		ActPos[3]	= (AdcVoltAvg[2] - Delta_Vd160[3]) / ((AdcVoltAvg[2] <= Delta_Vd160[3]) ? Delta_VN[3] : Delta_VP[3]);


		HAL_ADC_Start_DMA(&hadc3, (uint32_t *)(AdcVal+4), 5);
		HAL_ADC_Start_DMA(&hadc1, (uint32_t *)(AdcVal+1), 3);
		HAL_ADC_Start_DMA(&hadc2, (uint32_t *)(AdcVal+0), 1);
	}

	return;
}


// ---------- TIM Interrupt Callback ----------
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	static uint8_t taskCnt = 0;

	if(htim->Instance == TIM3)
	{
		taskCnt++;
		if(taskCnt % 5 == 0)
		{
			EmaTime++;
			if(EmaTime >= 86400000)
			{
				EmaTime = 0;
			}

			taskCnt = 0;
		}

		// Disable all actuators if voltage is less
		if(EmaActVolt < 18.0f)
		{
			ActStatflag[0] = 0;
			ActStatflag[1] = 0;
			ActStatflag[2] = 0;
			ActStatflag[3] = 0;
		}

		TimIntFlag = 1;
	}

	else
	{

	}

	return;
}




// ---------- UART Receive DMA Idle Callback ----------
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	uint8_t ActID;

	if(huart->Instance == ObcUart->Instance)
	{
		// Check CheckSum && Header
		if((ObcCmd.ObcCmdBuff[ObcCmd.ObcCmdBuff[3]+4] != calCheckSum(ObcCmd.ObcCmdBuff, ObcCmd.ObcCmdBuff[3]+4)) ||
		   (ObcCmd.ObcCmdBuff[0] != OBC_ID) || (ObcCmd.ObcCmdBuff[1] != EMA_ID))
		{
			sendAckNack(ObcCmd.ObcCmdBuff[2], 1);
			ObcErrCnt++;
			goto OBCEXIT;
		}

		// Check Commands
		switch(ObcCmd.ObcCmdBuff[2])
		{
			case Health_CmdID:
				sendHealth();
				break;

			case ActPos_CmdID:
				ActID = ObcCmd.ActPosCmd.ActID;

				if((ActID == Act1_ID || ActID == Act13_ID || ActID == ActAll_ID) && (-30.0f <= ObcCmd.ActPosCmd.ActPos[0] && ObcCmd.ActPosCmd.ActPos[0] <= 30.0f))
				{
					CmdActpos[0] = ObcCmd.ActPosCmd.ActPos[0];
				}

				if((ActID == Act2_ID || ActID == Act24_ID || ActID == ActAll_ID) && (-30.0f <= ObcCmd.ActPosCmd.ActPos[1] && ObcCmd.ActPosCmd.ActPos[1] <= 30.0f))
				{
					CmdActpos[1] = ObcCmd.ActPosCmd.ActPos[1];
				}

				if((ActID == Act3_ID || ActID == Act13_ID || ActID == ActAll_ID) && (-30.0f <= ObcCmd.ActPosCmd.ActPos[2] && ObcCmd.ActPosCmd.ActPos[2] <= 30.0f))
				{
					CmdActpos[2] = ObcCmd.ActPosCmd.ActPos[2];
				}

				if((ActID == Act4_ID || ActID == Act24_ID || ActID == ActAll_ID) && (-30.0f <= ObcCmd.ActPosCmd.ActPos[3] && ObcCmd.ActPosCmd.ActPos[3] <= 30.0f))
				{
					CmdActpos[3] = ObcCmd.ActPosCmd.ActPos[3];
				}

				sendActPosResp();
				break;

			case ActStat_CmdID:
				ActID = ObcCmd.ActStatCmd.ActID;

				if(EmaActVolt >= 18.0f)
				{
					if(ActID == Act1_ID || ActID == Act13_ID || ActID == ActAll_ID)
					{
						ActStatflag[0] = !!ObcCmd.ActStatCmd.Status;
					}

					if(ActID == Act2_ID || ActID == Act24_ID || ActID == ActAll_ID)
					{
						ActStatflag[1] = !!ObcCmd.ActStatCmd.Status;
					}

					if(ActID == Act3_ID || ActID == Act13_ID || ActID == ActAll_ID)
					{
						ActStatflag[2] = !!ObcCmd.ActStatCmd.Status;
					}

					if(ActID == Act4_ID || ActID == Act24_ID || ActID == ActAll_ID)
					{
						ActStatflag[3] = !!ObcCmd.ActStatCmd.Status;
					}

					sendAckNack(ObcCmd.ObcCmdBuff[2], 0);
				}
				else
				{
					sendAckNack(ObcCmd.ObcCmdBuff[2], 2);
				}
				break;

			case ReadOffset_CmdID:
				if(EmaActVolt >= 18.0f)
				{
					ReadOffsetFlag = 1;
				}
				else
				{
					sendAckNack(ObcCmd.ObcCmdBuff[2], 2);
				}
				break;

			default:
				sendAckNack(ObcCmd.ObcCmdBuff[2], 1);
				ObcErrCnt++;
				goto OBCEXIT;
		}

		ObcRxCnt++;

		OBCEXIT:
		// Again Start OBC UART Idle DMA for Data Receive
		HAL_UARTEx_ReceiveToIdle_DMA(ObcUart, ObcCmd.ObcCmdBuff, sizeof(ObcCmd.ObcCmdBuff));
		__HAL_DMA_DISABLE_IT(ObcUart->hdmarx, DMA_IT_HT);
	}

	else
	{

	}

	return;
}


// ---------- UART Transmit DMA/interrupt Callback ----------
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	if(huart->Instance == ObcUart->Instance)
	{
		ObcTxCnt++;
	}

	else
	{

	}

	return;
}


// ---------- UART Receive DMA/interrupt Callback ----------
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if(huart->Instance == ObcUart->Instance)
	{

	}

	else
	{

	}

	return;
}


/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
