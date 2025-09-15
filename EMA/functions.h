// ============================================================
// File Name	: functions.h
// Author		: Bhavesh
// Created on	: Jul 26, 2025
// ============================================================


#ifndef INC_FUNCTIONS_H_
#define INC_FUNCTIONS_H_

// ---------- Header Inclusion ----------
#include "adc.h"
#include "main.h"
#include "struct.h"
#include <string.h>
#include "timDelay.h"


// ---------- UART Variable -----------
extern UART_HandleTypeDef *ObcUart;

// ---------- Timer & PWM Variable -----------
extern TIM_HandleTypeDef *(PWMTim[4]);
extern long unsigned int PWMChn[4];

// ---------- GPIOs Variable -----------
extern GPIO_TypeDef *(Mot_SLT_Port[4][3]);
extern GPIO_TypeDef *(Mot_RST_Port[4][3]);
extern short unsigned int Mot_SLT_Pin[4][3];
extern short unsigned int Mot_RST_Pin[4][3];

// ---------- EMA Variable -----------
extern uint16_t SwCheckSum;

// ---------- ADC & Data Buffers -----------
extern uint16_t AdcVal[9];
extern union _ObcResp_ ObcResp;

// ---------- Actuators Variable -----------
extern volatile float EmaActVolt, ActCurr[4];
extern volatile float CmdActpos[4], ActPos[4];
extern volatile uint8_t FaultStat, ActStatflag[4];

// ---------- Motor Rotation Variable -----------
extern uint8_t SLT_Stat[2][7][3];	// SLT Pin Status
extern uint8_t RST_Stat[7][3];		// RST Pin Status



// ---------- Read ADC in Polling ----------
void readADC(void);

// ---------- Init Motor GPIOs ----------
void motorGPIOs_Init(void);

// ---------- Read Motor Hall Status ----------
uint8_t readHallStatus(uint8_t motor);

// ---------- Check Fault ----------
void checkMotorFault(void);

// ---------- Operate Motor ----------
void operateMotor(uint8_t motor, uint8_t status, uint8_t dir);

// ---------- CheckSum Calculation ----------
uint8_t calCheckSum(uint8_t *inpBuff, uint32_t Length);

// ---------- Send Health Response ----------
void sendHealth(void);

// ---------- Send Ack/Nack ----------
void sendAckNack(uint8_t stat, uint8_t cmdID);

// ---------- Send Actuator Position Response ----------
void sendActPosResp(void);

#endif /* INC_FUNCTIONS_H_ */




