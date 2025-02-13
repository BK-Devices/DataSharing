#include <LPC21xx.h>
#include "types.h"
#include "defines.h"

#define SCL 2
#define SDA 3

// Bit Defines of I2CONSET & I2CONCLR SFRs
#define AA 2 // For Both
#define SI 3 // For Both
#define STO 4 // Only for I2CONSET
#define STA 5 // For Both
#define EN 6 // For Both


// Initlize I2C Slave
void initSlaveI2C(u8 ADR){
	// Cfg SCL & SDA pins as FUN2
	CFGPIN(PINSEL0, SCL, FUN2);
	CFGPIN(PINSEL0, SDA, FUN2);
	
	I2ADR = ADR<<1; // Set your desired I2C address
	
	//I2C Peripheral Enable For Communication
	SSETBIT(I2CONSET, EN);
	SSETBIT(I2CONSET, AA);
}

// Read data send by I2C Master
u8 slaveReadI2C(void){
	u8 ch=0;
	while(!READBIT(I2CONSET, SI));
		ch = I2DAT;
		SCLRBIT(I2CONCLR, SI);
	
	return ch;
}


void returnI2C(u8 data){
	
	
}

// I2C Slave NACK Event
u8 nackSlaveI2C(void){
	I2CONSET = 0x00; // Assert Not of ACK
	SCLRBIT(I2CONCLR, SI);
	while(!READBIT(I2CONSET, SI));
	return I2DAT;
}

// I2C MACK Event
u8 mackSlaveI2C(void){
	SSETBIT(I2CONSET, AA); // Assert ACK
	SCLRBIT(I2CONCLR, SI);
	while(!READBIT(I2CONSET, SI));
	SCLRBIT(I2CONCLR, AA); // Clear Assert ACK
	return I2DAT;
}
