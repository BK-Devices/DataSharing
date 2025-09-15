// ============================================================
// File Name	: struct.h
// Author		: Bhavesh
// Created on	: Jul 26, 2025
// ============================================================


#ifndef INC_STRUCT_H_
#define INC_STRUCT_H_


// ---------- Header Inclusion ----------
#include "main.h"


// ---------- Software Version & CheckSum Details ----------
#define EmaSwVersion			"26/07/25 V02.00.01"

#define StartAddress			0x08000000
#define EndAddress 				0x080DFFFF
#define AppSize					(EndAddress - (StartAddress + 1))


// ---------- Device, Actuator & Command IDs -----------
#define  OBC_ID				0x09
#define  EMA_ID				0x08

#define  Act1_ID			0x23
#define  Act2_ID			0x25
#define  Act3_ID			0x26
#define  Act4_ID			0x29

#define  Act13_ID			0x30
#define  Act24_ID			0x31
#define  ActAll_ID			0x32

#define  ACK				0xE5
#define  NACK				0xB2
#define  ERROR				0xB1

#define  Health_CmdID		0x2B
#define  ActPos_CmdID		0x2C
#define  ActStat_CmdID		0x2A
#define  ReadOffset_CmdID	0x2D


// ---------- Macros -----------
#define  PI 			(22.0f/7.0f)
#define  DegToRad(X)	((X) * (PI / 180.0f))
#define  RadToDeg(X)	((X) * (180.0f / PI))


// ---------- Structure Declaration -----------
#pragma pack(1)


union _Offset_
{
	uint8_t Buffer[17];

	struct _OffsetStruct_
	{
		float Offset[4];
		uint8_t CheckSum;
	} OffsetStruct;
};


// Obc Command Structure
union _ObcCmd_
{
	uint8_t ObcCmdBuff[25];

	struct _HealthCmd_
	{
		uint8_t Header[3];
		uint8_t DataLen;
		uint8_t CheckSum;
	} HealthCmd;

	struct _ReadOffsetCmd_
	{
		uint8_t Header[3];
		uint8_t DataLen;
		uint8_t CheckSum;
	} ReadOffsetCmd;

	struct _ActStatCmd_
	{
		uint8_t Header[3];
		uint8_t DataLen;
		uint8_t ActID;
		uint8_t Status;
		uint8_t CheckSum;
	} ActStatCmd;

	struct _ActPosCmd_
	{
		uint8_t Header[3];
		uint8_t DataLen;
		uint8_t ActID;
		float ActPos[4];
		uint8_t CheckSum;
	} ActPosCmd;
};


// Obc Response Structure
union _ObcResp_
{
	uint8_t ObcRespBuff[40];

	struct _HealthResp_
	{
		uint8_t Header[3];
		uint8_t DataLen;
		uint8_t SwVersion[18];
		uint16_t SwCheckSum;
		uint8_t CheckSum;
	} HealthResp;

	struct _ActPosResp_
	{
		uint8_t Header[3];
		uint8_t DataLen;
		float ActPos[4];
		float ActCurr[4];
		uint8_t FaultStat;
		uint8_t CheckSum;
	} ActPosResp;

	struct _AckNack_
	{
		uint8_t Header[3];
		uint8_t DataLen;
		uint8_t AckByte;
		uint8_t CheckSum;
	} AckNack;
};

#pragma pack()

#endif /* INC_STRUCT_H_ */




