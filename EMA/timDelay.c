// ============================================================
// File Name	: timDelay.c
// Author		: Bhavesh
// Created on	: Jul 26, 2025
// ============================================================

// ---------- Header Inclusion ----------
#include "timDelay.h"


// ---------- DWT Init ----------
void DWT_Init(void)
{
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
	DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

	return;
}


// ---------- DWT Delay NanoSeconds ----------
void dwtdelayNS(uint64_t delay)
{
	uint64_t cycles = (delay * 550) / 1000;
	uint64_t start = DWT->CYCCNT;
	while((DWT->CYCCNT - start) < cycles);

	return;
}


// ---------- Timer Delay MicroSeconds ----------
void timDelayUS(uint64_t delay)
{
	__HAL_TIM_SET_COUNTER(&htim2, 0); // Reset Counter
	HAL_TIM_Base_Start(&htim2);

	while(__HAL_TIM_GET_COUNTER(&htim2) < delay);

	HAL_TIM_Base_Stop(&htim2);

	return;
}

// ---------- Timer Delay MilliSeconds ----------
void timDelayMS(uint64_t delay)
{
	for(uint64_t i = 0; i < delay; i++)
	{
		timDelayUS(1000);
	}

	return;
}

// ---------- Timer Delay Seconds ----------
void timDelayS(uint64_t delay)
{
	for(uint64_t i = 0; i < delay; i++)
	{
		timDelayUS(1000*1000);
	}

	return;
}




