// ============================================================
// File Name	: functions.c
// Author		: Bhavesh
// Created on	: Jul 26, 2025
// ============================================================


// ---------- Header Inclusion ----------
#include "functions.h"


// ---------- Read ADC in Polling ----------
void readADC(void)
{
	uint8_t i = 0, j = 0;
	uint32_t AdcValSum = 0;

	static uint8_t AdcCnt = 0;
	static uint16_t AdcValArr[9][15] = {0};

	// Read ADC1 3 Channels
	HAL_ADC_Start(&hadc1);
	for(i = 0; i < 3; i++)
	{
		HAL_ADC_PollForConversion(&hadc1, 20);
		AdcVal[i] = HAL_ADC_GetValue(&hadc1);
	}
	HAL_ADC_Stop(&hadc1);

	// Read ADC2 1 Channel
	HAL_ADC_Start(&hadc2);
	HAL_ADC_PollForConversion(&hadc2, 20);
	AdcVal[3] = HAL_ADC_GetValue(&hadc2);
	HAL_ADC_Stop(&hadc2);


	// Read ADC3 5 Channels
	HAL_ADC_Start(&hadc3);
	for(i = 4; i < 9; i++)
	{
		HAL_ADC_PollForConversion(&hadc3, 20);
		AdcVal[i] = HAL_ADC_GetValue(&hadc3);
	}
	HAL_ADC_Stop(&hadc3);

	memcpy(AdcValArr[AdcCnt], AdcVal, 9);
	if(++AdcCnt >= 15)
	{
		AdcCnt = 0;
	}

	for(i = 0; i < 9; i++)
	{
		AdcValSum = 0;

		for(j = 0; j < 15; j++)
		{
			AdcValSum += AdcValArr[i][j];
		}

		AdcVal[i] = (uint16_t)(AdcValSum / 15);
	}

	return;
}


// ---------- Init Motor GPIOs ----------
void motorGPIOs_Init(void)
{
	// Motor SLT GPIO Ports
	Mot_SLT_Port[0][0] = M1_SLTA_GPIO_Port;
	Mot_SLT_Port[0][1] = M1_SLTB_GPIO_Port;
	Mot_SLT_Port[0][2] = M1_SLTC_GPIO_Port;

	Mot_SLT_Port[1][0] = M2_SLTA_GPIO_Port;
	Mot_SLT_Port[1][1] = M2_SLTB_GPIO_Port;
	Mot_SLT_Port[1][2] = M2_SLTC_GPIO_Port;

	Mot_SLT_Port[2][0] = M3_SLTA_GPIO_Port;
	Mot_SLT_Port[2][1] = M3_SLTB_GPIO_Port;
	Mot_SLT_Port[2][2] = M3_SLTC_GPIO_Port;

	Mot_SLT_Port[3][0] = M4_SLTA_GPIO_Port;
	Mot_SLT_Port[3][1] = M4_SLTB_GPIO_Port;
	Mot_SLT_Port[3][2] = M4_SLTC_GPIO_Port;


	// Motor RST GPIO Ports
	Mot_RST_Port[0][0] = M1_RSTA_GPIO_Port;
	Mot_RST_Port[0][1] = M1_RSTB_GPIO_Port;
	Mot_RST_Port[0][2] = M1_RSTC_GPIO_Port;

	Mot_RST_Port[1][0] = M2_RSTA_GPIO_Port;
	Mot_RST_Port[1][1] = M2_RSTB_GPIO_Port;
	Mot_RST_Port[1][2] = M2_RSTC_GPIO_Port;

	Mot_RST_Port[2][0] = M3_RSTA_GPIO_Port;
	Mot_RST_Port[2][1] = M3_RSTB_GPIO_Port;
	Mot_RST_Port[2][2] = M3_RSTC_GPIO_Port;

	Mot_RST_Port[3][0] = M4_RSTA_GPIO_Port;
	Mot_RST_Port[3][1] = M4_RSTB_GPIO_Port;
	Mot_RST_Port[3][2] = M4_RSTC_GPIO_Port;


	// Motor SLT GPIO Pins
	Mot_SLT_Pin[0][0] = M1_SLTA_Pin;
	Mot_SLT_Pin[0][1] = M1_SLTB_Pin;
	Mot_SLT_Pin[0][2] = M1_SLTB_Pin;

	Mot_SLT_Pin[1][0] = M2_SLTA_Pin;
	Mot_SLT_Pin[1][1] = M2_SLTB_Pin;
	Mot_SLT_Pin[1][2] = M2_SLTB_Pin;

	Mot_SLT_Pin[2][0] = M3_SLTA_Pin;
	Mot_SLT_Pin[2][1] = M3_SLTB_Pin;
	Mot_SLT_Pin[2][2] = M3_SLTB_Pin;

	Mot_SLT_Pin[3][0] = M4_SLTA_Pin;
	Mot_SLT_Pin[3][1] = M4_SLTB_Pin;
	Mot_SLT_Pin[3][2] = M4_SLTB_Pin;


	// Motor RST GPIO Pins
	Mot_RST_Pin[0][0] = M1_RSTA_Pin;
	Mot_RST_Pin[0][1] = M1_RSTB_Pin;
	Mot_RST_Pin[0][2] = M1_RSTB_Pin;

	Mot_RST_Pin[1][0] = M2_RSTA_Pin;
	Mot_RST_Pin[1][1] = M2_RSTB_Pin;
	Mot_RST_Pin[1][2] = M2_RSTB_Pin;

	Mot_RST_Pin[2][0] = M3_RSTA_Pin;
	Mot_RST_Pin[2][1] = M3_RSTB_Pin;
	Mot_RST_Pin[2][2] = M3_RSTB_Pin;

	Mot_RST_Pin[3][0] = M4_RSTA_Pin;
	Mot_RST_Pin[3][1] = M4_RSTB_Pin;
	Mot_RST_Pin[3][2] = M4_RSTB_Pin;


	return;
}


// ---------- Read Motor Hall Status ----------
// motor	--  0-3
uint8_t readHallStatus(uint8_t motor)
{
	uint8_t status = 0;

	if(motor == 0)
	{
		// Motor 1 Hall Status
		status |= (HAL_GPIO_ReadPin(M1_HallA_GPIO_Port, M1_HallA_Pin) << 2);
		status |= (HAL_GPIO_ReadPin(M1_HallB_GPIO_Port, M1_HallB_Pin) << 1);
		status |= (HAL_GPIO_ReadPin(M1_HallC_GPIO_Port, M1_HallC_Pin) << 0);

	}
	else if(motor == 0)
	{
		// Motor 2 Hall Status
		status |= (HAL_GPIO_ReadPin(M2_HallA_GPIO_Port, M2_HallA_Pin) << 2);
		status |= (HAL_GPIO_ReadPin(M2_HallB_GPIO_Port, M2_HallB_Pin) << 1);
		status |= (HAL_GPIO_ReadPin(M2_HallC_GPIO_Port, M2_HallC_Pin) << 0);
	}
	else if(motor == 0)
	{
		// Motor 3 Hall Status
		status |= (HAL_GPIO_ReadPin(M3_HallA_GPIO_Port, M3_HallA_Pin) << 2);
		status |= (HAL_GPIO_ReadPin(M3_HallB_GPIO_Port, M3_HallB_Pin) << 1);
		status |= (HAL_GPIO_ReadPin(M3_HallC_GPIO_Port, M3_HallC_Pin) << 0);
	}
	else if(motor == 0)
	{
		// Motor 4 Hall Status
		status |= (HAL_GPIO_ReadPin(M4_HallA_GPIO_Port, M4_HallA_Pin) << 2);
		status |= (HAL_GPIO_ReadPin(M4_HallB_GPIO_Port, M4_HallB_Pin) << 1);
		status |= (HAL_GPIO_ReadPin(M4_HallC_GPIO_Port, M4_HallC_Pin) << 0);
	}
	else
	{

	}

	if(status < 1 || 6 < status)
	{
		status = 0;
	}

	return status;
}


// ---------- Check Fault ----------
void checkMotorFault(void)
{
	uint8_t ActFault[4] = {0, 0, 0, 0};

	FaultStat = 0;

	// Actuator 1
	ActFault[0] |= (HAL_GPIO_ReadPin(M1_FAULT_GPIO_Port, M1_FAULT_Pin) << 0);
	ActFault[0] |= (HAL_GPIO_ReadPin(M1_OTW_GPIO_Port, M1_OTW_Pin) << 1);
	FaultStat ^= (ActFault[0] << 6);

	// Actuator 2
	ActFault[1] |= (HAL_GPIO_ReadPin(M2_FAULT_GPIO_Port, M2_FAULT_Pin) << 0);
	ActFault[1] |= (HAL_GPIO_ReadPin(M2_OTW_GPIO_Port, M2_OTW_Pin) << 1);
	FaultStat ^= (ActFault[1] << 4);

	// Actuator 3
	ActFault[2] |= (HAL_GPIO_ReadPin(M3_FAULT_GPIO_Port, M3_FAULT_Pin) << 0);
	ActFault[2] |= (HAL_GPIO_ReadPin(M3_OTW_GPIO_Port, M3_OTW_Pin) << 1);
	FaultStat ^= (ActFault[2] << 2);

	// Actuator 4
	ActFault[3] |= (HAL_GPIO_ReadPin(M4_FAULT_GPIO_Port, M4_FAULT_Pin) << 0);
	ActFault[3] |= (HAL_GPIO_ReadPin(M4_OTW_GPIO_Port, M4_OTW_Pin) << 1);
	FaultStat ^= (ActFault[3] << 0);

	return;
}


// ---------- Operate Motor ----------
// dir		--  0 : Backward  |  1 : Forward
// status 	--  0 : Disable   |  1 : Enable
// motor	--  0-3
void operateMotor(uint8_t motor, uint8_t status, uint8_t dir)
{
	uint8_t HallStat = 0;

	if(motor >= 4 || dir >= 2)
	{
		return;
	}

	if(status == 1)
	{
		// Read Hall Sensor Status
		HallStat = readHallStatus(motor);

		// Start PWM Pulse
		HAL_TIM_PWM_Stop(PWMTim[motor], PWMChn[motor]);

		if(HallStat == 0)
		{
			return;
		}

		// RST GPIO Pins
		HAL_GPIO_WritePin(Mot_RST_Port[motor][0], Mot_RST_Pin[motor][0], 1);
		HAL_GPIO_WritePin(Mot_RST_Port[motor][1], Mot_RST_Pin[motor][1], 1);
		HAL_GPIO_WritePin(Mot_RST_Port[motor][2], Mot_RST_Pin[motor][2], 1);

		// 50 Nanoseconds Delay
		dwtdelayNS(50);

		// SLT GPIO Pins
		HAL_GPIO_WritePin(Mot_SLT_Port[motor][0], Mot_SLT_Pin[motor][0], SLT_Stat[dir][HallStat][0]);
		HAL_GPIO_WritePin(Mot_SLT_Port[motor][1], Mot_SLT_Pin[motor][1], SLT_Stat[dir][HallStat][1]);
		HAL_GPIO_WritePin(Mot_SLT_Port[motor][2], Mot_SLT_Pin[motor][2], SLT_Stat[dir][HallStat][2]);

		// 50 Nanoseconds Delay
		dwtdelayNS(50);

		// RST GPIO Pins
		HAL_GPIO_WritePin(Mot_RST_Port[motor][0], Mot_RST_Pin[motor][0], RST_Stat[HallStat][0]);
		HAL_GPIO_WritePin(Mot_RST_Port[motor][1], Mot_RST_Pin[motor][1], RST_Stat[HallStat][1]);
		HAL_GPIO_WritePin(Mot_RST_Port[motor][2], Mot_RST_Pin[motor][2], RST_Stat[HallStat][2]);
	}
	else if(status == 0)
	{
		// Stop PWM Pulse
		HAL_TIM_PWM_Stop(PWMTim[motor], PWMChn[motor]);

		// 50 Nanoseconds Delay
		dwtdelayNS(50);

		// SLT GPIO Pins
		HAL_GPIO_WritePin(Mot_SLT_Port[motor][0], Mot_SLT_Pin[motor][0], 0);
		HAL_GPIO_WritePin(Mot_SLT_Port[motor][1], Mot_SLT_Pin[motor][1], 0);
		HAL_GPIO_WritePin(Mot_SLT_Port[motor][2], Mot_SLT_Pin[motor][2], 0);

		// 50 Nanoseconds Delay
		dwtdelayNS(50);

		// RST GPIO Pins
		HAL_GPIO_WritePin(Mot_RST_Port[motor][0], Mot_RST_Pin[motor][0], 0);
		HAL_GPIO_WritePin(Mot_RST_Port[motor][1], Mot_RST_Pin[motor][1], 0);
		HAL_GPIO_WritePin(Mot_RST_Port[motor][2], Mot_RST_Pin[motor][2], 0);
	}
	else
	{

	}

	return;
}


// ---------- CheckSum Calculation ----------
uint8_t calCheckSum(uint8_t *inpBuff, uint32_t Length)
{
	int byteCnt;
	uint8_t checksum = 0;

	for(byteCnt = 0; byteCnt < Length; byteCnt++)
	{
		checksum = checksum ^ inpBuff[byteCnt];
	}

	return checksum;
}


// ---------- Send Health Response ----------
void sendHealth(void)
{
	ObcResp.HealthResp.Header[0] = EMA_ID;
	ObcResp.HealthResp.Header[1] = OBC_ID;
	ObcResp.HealthResp.Header[2] = Health_CmdID;

	ObcResp.HealthResp.DataLen = sizeof(struct _HealthResp_) - 0x05;

	memcpy(ObcResp.HealthResp.SwVersion, EmaSwVersion, 18);
	ObcResp.HealthResp.SwCheckSum = SwCheckSum;

	ObcResp.HealthResp.CheckSum = calCheckSum(ObcResp.ObcRespBuff, ObcResp.HealthResp.DataLen + 0x04);

	HAL_UART_Transmit_DMA(ObcUart, ObcResp.ObcRespBuff, ObcResp.HealthResp.DataLen + 0x05);

	return;
}


// ---------- Send Ack/Nack ----------
void sendAckNack(uint8_t stat, uint8_t cmdID)
{
	uint8_t AckByte[3] = {0xE5, 0xB2, 0xB1};

	if(stat >= sizeof(AckByte))
	{
		return;
	}

	ObcResp.AckNack.Header[0] = EMA_ID;
	ObcResp.AckNack.Header[1] = OBC_ID;
	ObcResp.AckNack.Header[2] = cmdID;

	ObcResp.AckNack.DataLen = sizeof(struct _AckNack_) - 0x05;

	ObcResp.AckNack.AckByte = AckByte[stat];

	ObcResp.AckNack.CheckSum = calCheckSum(ObcResp.ObcRespBuff, ObcResp.AckNack.DataLen + 0x04);

	HAL_UART_Transmit_DMA(ObcUart, ObcResp.ObcRespBuff, ObcResp.AckNack.DataLen + 0x05);

	return;
}


// ---------- Send Actuator Position Response ----------
void sendActPosResp(void)
{
	ObcResp.ActPosResp.Header[0] = EMA_ID;
	ObcResp.ActPosResp.Header[1] = OBC_ID;
	ObcResp.ActPosResp.Header[2] = Health_CmdID;

	ObcResp.ActPosResp.DataLen = sizeof(struct _ActPosResp_) - 0x05;

	ObcResp.ActPosResp.ActPos[0] = ActPos[0];
	ObcResp.ActPosResp.ActPos[1] = ActPos[1];
	ObcResp.ActPosResp.ActPos[2] = ActPos[2];
	ObcResp.ActPosResp.ActPos[3] = ActPos[3];

	ObcResp.ActPosResp.ActCurr[0] = ActCurr[0];
	ObcResp.ActPosResp.ActCurr[1] = ActCurr[1];
	ObcResp.ActPosResp.ActCurr[2] = ActCurr[2];
	ObcResp.ActPosResp.ActCurr[3] = ActCurr[3];

	ObcResp.ActPosResp.FaultStat = FaultStat;

	ObcResp.ActPosResp.CheckSum = calCheckSum(ObcResp.ObcRespBuff, ObcResp.ActPosResp.DataLen + 0x04);

	HAL_UART_Transmit_DMA(ObcUart, ObcResp.ObcRespBuff, ObcResp.ActPosResp.DataLen + 0x05);

	return;
}




