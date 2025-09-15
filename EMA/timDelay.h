// ============================================================
// File Name	: timDelay.c
// Author		: Bhavesh
// Created on	: Jul 26, 2025
// ============================================================


#ifndef INC_TIMDELAY_H_
#define INC_TIMDELAY_H_

// ---------- Header Inclusion ----------
#include "tim.h"
#include "main.h"
#include "core_cm7.h"


// ---------- DWT Init ----------
void DWT_Init(void);

// ---------- DWT Delay NanoSeconds ----------
void dwtdelayNS(uint64_t delay);

// ---------- Timer Delay MicroSeconds ----------
void timDelayUS(uint64_t delay);

// ---------- Timer Delay MilliSeconds ----------
void timDelayMS(uint64_t delay);

// ---------- Timer Delay Seconds ----------
void timDelayS(uint64_t delay);


#endif /* INC_TIMDELAY_H_ */




