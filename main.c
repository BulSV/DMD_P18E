#include <c8051f410.h>
#include <stdio.h>

extern void Init_Device(void);

// SPI config pins for control ADF4002BCPZ, ADF4351BCPZ and AD8366ACPZ
sbit CS_AMPL = P1^2;	// Chip select for AD8366ACPZ (NSS) (output)
sbit LE_4351 = P1^3;	// Load eneble for ADF4351BCPZ (NSS) (output)
sbit LE_4002 = P1^2;	// Load enable for ADF4002BCPZ (NSS) (output)

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

#define SYSCLK 24500000 // SYSCLK frequency in Hz

// Configure Timer0 to delay <ms> milliseconds
void Timer0_ms (unsigned ms)
{
    unsigned i; // millisecond counter

    TCON &= ~0x30; // STOP Timer0 and clear overflow flag
    TMOD &= ~0x0f; // configure Timer0 to 16-bit mode
    TMOD |= 0x01;
    CKCON |= 0x08; // Timer0 counts SYSCLKs

    for (i = 0; i < ms; i++) // count milliseconds
    {
        TR0 = 0; // STOP Timer0
        TH0 = (-SYSCLK/1000) >> 8; // set Timer0 to overflow in 1ms
        TL0 = -SYSCLK/1000;
        TR0 = 1; // START Timer0

        while (TF0 == 0); // wait for overflow

        TF0 = 0; // clear overflow indicator
    }
}

// Configure Timer0 to delay <us> microseconds
void Timer0_us (unsigned us)
{
    unsigned i; // microsecond counter

    TCON &= ~0x30; // STOP Timer0 and clear overflow flag
    TMOD &= ~0x0f; // configure Timer0 to 16-bit mode
    TMOD |= 0x01;
    CKCON |= 0x08; // Timer0 counts SYSCLKs

    for (i = 0; i < us; i++) { // count microseconds
        TR0 = 0; // STOP Timer0
        TH0 = (-SYSCLK/1000000) >> 8; // set Timer0 to overflow in 1us
        TL0 = -SYSCLK/1000000;
        TR0 = 1; // START Timer0

        while (TF0 == 0); // wait for overflow

        TF0 = 0; // clear overflow indicator
    }
}

// SPI send subrouting
void SPI_send(char Data)
{
	SPI0DAT = Data;
}

// SPI lock detect of ADF4351BCPZ subrouting
void SPI_LD_4351(void) interrupt 6
{
	if(SPI0DAT & 1) {
		LOCK_4351 = 1;
	}

	SPIF = 0;	// Clear SPI flag
}

// Program ADF4002BCPZ as 128-divider
void ADF4002_divider(void)
{
	LE_4002 = 1;	// Select ADF4002BCPZ
	Timer0_ms(1);

	// init latch
	SPI_send(0x1F);
	SPI_send(0xA6);
	SPI_send(0xA3);
	Timer0_ms(1);
	// func latch
	SPI_send(0x1F);
	SPI_send(0xA6);
	SPI_send(0xA2);
	Timer0_ms(1);
	// R-counter
	SPI_send(0x12);
	SPI_send(0x00);
	SPI_send(0x04);
	Timer0_ms(1);
	// N-counter
	SPI_send(0x20);
	SPI_send(0x80);
	SPI_send(0x01);
	Timer0_ms(1);

	LE_4002 = 0;	// De-select ADF4002BCPZ
}

// Strobe Selector (MC74HCT160D)
void strobeSelect(char divFactor)
{
	PE_COUNTER = 0;	// Parallel enable write data into counter
	MR_COUNTER = 0;	// Reset counter (data don't cleared)
	Timer0_ms(1);

	switch(divFactor)
	{
	case 1:
		D0_COUNTER = 1;	// Data Low
		D1_COUNTER = 0;	// Data Midle
		D2_COUNTER = 0;	// Data Midle
		D3_COUNTER = 1;	// Data High
		break;
	case 2:
		D0_COUNTER = 0;	// Data Low
		D1_COUNTER = 0;	// Data Midle
		D2_COUNTER = 0;	// Data Midle
		D3_COUNTER = 1;	// Data High
		break;
	case 3:
		D0_COUNTER = 1;	// Data Low
		D1_COUNTER = 1;	// Data Midle
		D2_COUNTER = 1;	// Data Midle
		D3_COUNTER = 0;	// Data High
		break;
	case 4:
		D0_COUNTER = 0;	// Data Low
		D1_COUNTER = 1;	// Data Midle
		D2_COUNTER = 1;	// Data Midle
		D3_COUNTER = 0;	// Data High
		break;
	case 5:
		D0_COUNTER = 1;	// Data Low
		D1_COUNTER = 0;	// Data Midle
		D2_COUNTER = 1;	// Data Midle
		D3_COUNTER = 0;	// Data High
		break;
	case 6:
		D0_COUNTER = 0;	// Data Low
		D1_COUNTER = 0;	// Data Midle
		D2_COUNTER = 1;	// Data Midle
		D3_COUNTER = 0;	// Data High
		break;
	case 7:
		D0_COUNTER = 1;	// Data Low
		D1_COUNTER = 1;	// Data Midle
		D2_COUNTER = 0;	// Data Midle
		D3_COUNTER = 0;	// Data High
		break;
	case 8:
		D0_COUNTER = 0;	// Data Low
		D1_COUNTER = 1;	// Data Midle
		D2_COUNTER = 0;	// Data Midle
		D3_COUNTER = 0;	// Data High
		break;
	case 9:
		D0_COUNTER = 1;	// Data Low
		D1_COUNTER = 0;	// Data Midle
		D2_COUNTER = 0;	// Data Midle
		D3_COUNTER = 0;	// Data High
		break;
	default: break;
	}

	Timer0_ms(1);
	PE_COUNTER = 1;	// Parallel disable write data into counter
	MR_COUNTER = 1;	// Cancel Reset counter
}

// Gain set (AD8366ACPZ)
void gainSetCode(char Code)
{
	CS_AMPL = 1;	// Select AD8366ACPZ
	Timer0_ms(1);
	SPI_send(Code);	// Setting differential output A gain
	SPI_send(Code);	// Setting differential output B gain
	Timer0_ms(1);
	CS_AMPL = 0;	// De-select AD8366ACPZ
}

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

	ADF4002_divider();
	
	while(1)
	{
		
	}
}