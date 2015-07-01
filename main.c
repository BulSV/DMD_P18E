#include <c8051f410.h>
#include <stdio.h>

extern void Init_Device(void);

// SPI config pins for control ADF4002BCPZ, ADF4351BCPZ and AD8366ACPZ
sbit CS_AMPL = P1^2;	// Chip select for AD8366ACPZ (NSS) (output)
sbit LE_4351 = P1^3;	// Load eneble for ADF4351BCPZ (NSS) (output)
sbit LE_4002 = P1^2;	// Load enable for ADF4002CPZ (NSS) (output)

sbit CLK = P1^4;	// Serial SPI clock (output)
sbit DATA = P1^6;	// MOSI (output)

sbit LD_4351 = P1^5;	// Lock detect for ADF4351BCPZ (input) (MISO)

// Config pins for control MC74HCT160D
sbit PE_COUNTER = P2^0;	// Parallel enable write data into counter (active low) (output)
sbit MR_COUNTER = P2^5;	// Reset counter (data don't cleared) (active low) (output)
sbit D0_COUNTER = P2^4;	// Data Low (output)
sbit D1_COUNTER = P2^3;	// Data Midle (output)
sbit D2_COUNTER = P2^2;	// Data Midle (output)
sbit D3_COUNTER = P2^1;	// Data High (output)

// LEDs pins config
sbit POWER_ON = P0^0;	// (active high) (output)
sbit LOCK_4351 = P0^1;	// When ADF4351BCPZ locked PLL (active high) (output)
sbit SPI_LED = P0^7;	// Indicate SPI work (active high) (output)
// blinking - transfering;
// lit after blinking - error;
// not lit after blinking - transfer is over successfully
sbit COUNTER_LED = P2^6;// Indicate counter switching (active high) (output)

void main(void)
{
	Init_Device();

	// LEDs init
	POWER_ON = 1;
	LOCK_4351 = 0;

	// De-select chip for SPI (init)
	CS_AMPL = 0;
	LE_4351 = 0;
	LE_4002 = 0;
	
	while(1)
	{
		
	}
}