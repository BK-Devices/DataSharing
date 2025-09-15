// ============================================================
// File Name	: flashData.c
// Author		: bhavesh
// Created on	: Jul 26, 2025
// ============================================================


// ---------- Header Inclusion ----------
#include "flashData.h"
#include <string.h>


static uint8_t flashBuff[FLASHWORD_SIZE] __attribute__((aligned(32)));


// ---------- read Flash Data ----------
uint32_t readFlashData(uint32_t FlashAddr, uint8_t *Data, uint32_t Size)
{
	uint32_t ReadSize;

	if(FlashAddr < SEC_ADDR)
	{
		return 0;
	}

	ReadSize = SEC_SIZE - (FlashAddr - SEC_ADDR);
	ReadSize = (ReadSize <= Size)? ReadSize : Size;

	memcpy(Data, (uint8_t *)FlashAddr, ReadSize);

// -------------- Need to update --------------
// another logic instead of memcpy
//	while()
//	*((uint32_t *)Data) = *(__IO uint32_t *)FlashAddr;
//	FlashAddr += 4;
//	((uint32_t *)Data)++;
//	if (!(Size _= 4)) break;
// -------------- Need to update --------------

	return ReadSize;
}


// ---------- Erase Flash Last Sector ----------
HAL_StatusTypeDef eraseFlash_LastSector(void)
{
    FLASH_EraseInitTypeDef eraseInit;
    HAL_StatusTypeDef Status;
    uint32_t sectorError;

    eraseInit.TypeErase     = FLASH_TYPEERASE_SECTORS;
    eraseInit.Banks         = FLASH_BANK_1;
    eraseInit.Sector        = 7;   						// last sector
    eraseInit.NbSectors     = 1;

    HAL_FLASH_Unlock();
    Status = HAL_FLASHEx_Erase(&eraseInit, &sectorError);
    HAL_FLASH_Lock();

    return Status;
}


// ---------- Write Flash Data ----------
uint32_t writeFlashData(uint8_t *Data, uint32_t Size)
{
	uint32_t MemAddr = SEC_ADDR;
	HAL_StatusTypeDef Status = HAL_OK;
	uint32_t WriteSize = 0, Chunk = 0, i = 0;

	WriteSize = Size <= SEC_SIZE ? Size : SEC_SIZE;

	HAL_FLASH_Unlock();

	while(i < WriteSize)
	{
		memset(flashBuff, 0xFF, FLASHWORD_SIZE);
		Chunk = (WriteSize - i >= FLASHWORD_SIZE) ? FLASHWORD_SIZE : (WriteSize - i);
		memcpy(flashBuff, Data + i, Chunk);

		Status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, MemAddr, (uint32_t)flashBuff);
		if(Status != HAL_OK)
		{
			HAL_FLASH_Lock();
			return i;
		}

		MemAddr += FLASHWORD_SIZE;
		i += Chunk;
	}

	HAL_FLASH_Lock();
	return WriteSize;
}


// ---------- Modify Flash Data ----------
//HAL_StatusTypeDef Flash_ModifyBytes(uint32_t offset, uint8_t *newData, uint32_t len)
//{
//    if((offset + len) > SEC_SIZE) return HAL_ERROR;
//
//    HAL_StatusTypeDef status;
//    uint8_t *sectorCopy = malloc(SEC_SIZE);
//    if (!sectorCopy) return HAL_ERROR;
//
//    // Step 1: Read full sector into RAM
//    memcpy(sectorCopy, (uint8_t*)SEC_ADDR, SEC_SIZE);
//
//    // Step 2: Modify requested bytes
//    memcpy(&sectorCopy[offset], newData, len);
//
//    // Step 3: Erase and rewrite full sector
//    status = Flash_SaveData(sectorCopy, SEC_SIZE);
//
//    free(sectorCopy);
//    return status;
//}




