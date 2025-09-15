// ============================================================
// File Name	: variables.h
// Author		: Bhavesh
// Created on	: Jul 26, 2025
// ============================================================


#ifndef INC_VARIABLES_H_
#define INC_VARIABLES_H_

// ---------- Header Inclusion ----------
#include "main.h"
#include "struct.h"


// ---------- UART Variable -----------
UART_HandleTypeDef *ObcUart	= &huart8;


// ---------- Timer & PWM Variable -----------
TIM_HandleTypeDef *(PWMTim[4]) = {&htim23, &htim13, &htim4, &htim1};
long unsigned int PWMChn[4] = {TIM_CHANNEL_1, TIM_CHANNEL_1, TIM_CHANNEL_1, TIM_CHANNEL_4};


// ---------- GPIOs Variable -----------
GPIO_TypeDef *(Mot_SLT_Port[4][3]);
GPIO_TypeDef *(Mot_RST_Port[4][3]);
short unsigned int Mot_SLT_Pin[4][3];
short unsigned int Mot_RST_Pin[4][3];


// ---------- EMA Variable -----------
uint16_t SwCheckSum = 0;
volatile uint32_t EmaTime = 0;
volatile uint8_t TimIntFlag = 0, ReadOffsetFlag = 0;


// ---------- ADC & Data Buffers -----------
uint16_t AdcVal[9];				// ADC Buffer
union _ObcCmd_ ObcCmd;
union _ObcResp_ ObcResp;
uint8_t ObcRxCnt = 0, ObcTxCnt = 0, ObcErrCnt = 0;


// ---------- Actuators Variable -----------
volatile float EmaActVolt, ActCurr[4];
volatile float CmdActpos[4], ActPos[4];
volatile uint8_t FaultStat, ActStatflag[4];


// ---------- Potentiometer Variable -----------
const float Vd0 = -0.19875f, Vd150 = 1.5590625f, Vd160 = 1.67625f, Vd170 = 1.7934375f, Vd320 = 3.55125f;
float Delta_Vd160[4] = {1.67625f, 1.67625f, 1.67625f, 1.67625f};
float Delta_VN[4] = {0.01171875f, 0.01171875f, 0.01171875f, 0.01171875f};
float Delta_VP[4] = {0.01171875f, 0.01171875f, 0.01171875f, 0.01171875f};


// ---------- Error Calculation Variable -----------
const float Kpfb[4] = {1.0f, 1.0f, 1.0f, 1.0f};
const float MaxTorque[4] = {24.0f, 24.0f, 24.0f, 24.0f};
const float Kpos[4] = {(20.0f * 3.0f * 16.44f), (20.0f * 3.0f * 16.44f), (20.0f * 3.0f * 16.44f), (20.0f * 3.0f * 16.44f)};
const float Krfb[4] = {(0.45f * 0.0103f * 1.7f), (0.45f * 0.0103f * 1.7f), (0.45f * 0.0103f * 1.7f), (0.45f * 0.0103f * 1.7f)};

volatile float Delta_FB_Com[4], Delta_FB_Com_t[4][2];
volatile float Delta_Error[4], PWM_DC[4], Delta_Dot_FF[4];
volatile float Delta_Cmd[4], Delta_Pos[4], Delta_Pos_Last[4];
volatile float Delta_FB_Com_Fil[4], Delta_FB_Com_Fil_t[4][2];


// ---------- Motor Rotation Variable -----------
// SLT Pin Status
uint8_t SLT_Stat[2][7][3] =
{
	// Backward Rotation
	{
		{0, 0, 0},	// Dummy
		{0, 1, 0},	// Hall = 1
		{1, 0, 0},	// Hall = 2
		{1, 0, 0},	// Hall = 3
		{0, 0, 1},	// Hall = 4
		{0, 1, 0},	// Hall = 5
		{0, 0, 1}	// Hall = 6
	},

	// Forward Rotation
	{
		{0, 0, 0},	// Dummy
		{0, 0, 1},	// Hall = 1
		{0, 1, 0},	// Hall = 2
		{0, 0, 1},	// Hall = 3
		{1, 0, 0},	// Hall = 4
		{1, 0, 0},	// Hall = 5
		{0, 1, 0}	// Hall = 6
	}
};

// RST Pin Status
uint8_t RST_Stat[7][3] =
{
	{0, 0, 0},	// Dummy
	{0, 1, 1},	// Hall = 1
	{1, 1, 0},	// Hall = 2
	{1, 0, 1},	// Hall = 3
	{1, 0, 1},	// Hall = 4
	{1, 1, 0},	// Hall = 5
	{0, 1, 1}	// Hall = 6
};


#endif /* INC_VARIABLES_H_ */




