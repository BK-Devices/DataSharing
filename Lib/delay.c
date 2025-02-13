#include "types.h"

// Delay functions defination
void delay_us(f32 dlyUS){
	u32 dly;
	dlyUS *= 12; // ~1us @CCLK = 60MHZ
	dly=dlyUS;
	while(dly--);
}

void delay_ms(f32 dlyMS){
	u32 dly;
	dlyMS *= 12000; // ~1ms @CCLK = 60MHZ
	dly=dlyMS;
	while(dly--);
}

void delay_s(f32 dlyS){
	u32 dly;
	dlyS *= 12000000; // ~1s @CCLK = 60MHZ
	dly=dlyS;
	while(dly--);
}
