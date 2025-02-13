#include <LPC21xx.h>
#include "types.h"
#include "defines.h"

#define SCL 2
#define SDA 3

// I2C_SPEED Configuration
#define FOSC 12000000
#define CCLK 5*FOSC
#define PCLK CCLK/4 // Hz
#define I2C_SPEED 100000 // Hz
#define BITRATE ((PCLK/I2C_SPEED)/2)

// Bit Defines of I2CONSET & I2CONCLR SFRs
#define AA 2 // For Both
#define SI 3 // For Both
#define STO 4 // Only for I2CONSET
#define STA 5 // For Both
#define EN 6 // For Both

// Initlize I2C Master
void initI2C(void){
	// Cfg SCL & SDA pins as FUN2
	CFGPIN(PINSEL0, SCL, FUN2);
	CFGPIN(PINSEL0, SDA, FUN2);
	
	//Cfg speed for i2c communicaton
	I2SCLL = BITRATE;
	I2SCLH = BITRATE;
	
	//I2C Peripheral Enable For Communication
	SSETBIT(I2CONSET, EN);
}

// I2C START Event
void startI2C(void){
	SSETBIT(I2CONSET, STA); // Start Condition
	while(!READBIT(I2CONSET, SI)); // Wait for start bit status
	SCLRBIT(I2CONCLR, STA); // Clear start condition
}


// I2C RESTART Event
void restartI2C(void){
	SSETBIT(I2CONSET, STA); // Start Condition
	SCLRBIT(I2CONCLR, SI); // Clear Previous SI Bit Status
	while(!READBIT(I2CONSET, SI)); // Wait for SI Bit Status
	SCLRBIT(I2CONCLR, STA); // Clear Start Condition
}

// I2C WRITE Event
void writeI2C(u8 data){
	I2DAT = data;
	SCLRBIT(I2CONCLR, SI); // Clear SI Bit
	// wait for SI Bit Status using I2CONSET
	while(!READBIT(I2CONSET, SI));	
}

// I2C STOP Event
void stopI2C(void){
	SSETBIT(I2CONSET, STO); // issue stop Condition
	SCLRBIT(I2CONCLR, SI); // Clear SI Bit status
	// Stop will automatically cleared
	//while(!READBIT(I2CONSET, SI)); // Wait for stop bit clear
}

// I2C NACK Event
u8 nackI2C(void){
	I2CONSET = 0x00; // Assert Not of ACK
	SCLRBIT(I2CONCLR, SI);
	while(!READBIT(I2CONSET, SI));
	return I2DAT;
}


// I2C MACK Event
u8 mackI2C(void){
	SSETBIT(I2CONSET, AA); // Assert ACK
	SCLRBIT(I2CONCLR, SI);
	while(!READBIT(I2CONSET, SI));
	SCLRBIT(I2CONCLR, AA); // Clear Assert ACK
	return I2DAT;
}
