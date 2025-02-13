

#include "types.h"

void initI2C(void); // Initlize I2C Master
void startI2C(void); // I2C START Event
void restartI2C(void); // I2C RESTART Event
void writeI2C(u8 data); // I2C WRITE Event
void stopI2C(void); // I2C STOP Event
u8 nackI2C(void); // I2C NACK Event
u8 mackI2C(void); // I2C MACK Event



