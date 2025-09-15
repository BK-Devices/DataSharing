// ============================================================
// File Name	: flashData.h
// Author		: bhavesh
// Created on	: Jul 26, 2025
// ============================================================


#ifndef INC_FLASHDATA_H_
#define INC_FLASHDATA_H_

// ---------- Header Inclusion ----------
#include "main.h"
#include <string.h>


#define SEC_ADDR         	0x080E0000
#define SEC_SIZE         	0x20000
#define FLASHWORD_SIZE		0x20



// ---------- read Flash Data ----------
uint32_t readFlashData(uint32_t FlashAddr, uint8_t *Data, uint32_t Size);

// ---------- Erase Flash Last Sector ----------
HAL_StatusTypeDef eraseFlash_LastSector(void);


// ---------- Write Flash Data ----------
uint32_t writeFlashData(uint8_t *Data, uint32_t Size);

// ---------- Modify Flash Data ----------
// HAL_StatusTypeDef Flash_ModifyBytes(uint32_t offset, uint8_t *newData, uint32_t len);

#endif /* INC_FLASHDATA_H_ */




