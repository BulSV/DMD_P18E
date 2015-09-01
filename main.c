//------------------------------------------------------------------------------
// main.c
//------------------------------------------------------------------------------
// Author: BulSV
// Controller: C8051F330A
// Synthesizers: ADF4002BCPZ, ADF4351BCPZ
// Operational Amplifier: AD8366ACPZ
// Counter: MC74HCT160D ( 155»≈9)
//------------------------------------------------------------------------------
// Program Description:
//

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
// memory models
//#pragma large
//#pragma compact
//#pragma small
#include <c8051f410.h>
#include <stdio.h>

extern void Init_Device(void);

//------------------------------------------------------------------------------
// Global CONSTANTS
//------------------------------------------------------------------------------

// SPI config pins for control ADF4002BCPZ, ADF4351BCPZ and AD8366ACPZ
sbit CS_AMPL = P1^2;	// Chip select for AD8366ACPZ (NSS) (output)
sbit LE_4351 = P1^3;	// Load enable for ADF4351BCPZ (NSS) (output)
sbit LE_4002 = P1^7;	// Load enable for ADF4002BCPZ (NSS) (output)

sbit CLK = P1^4;	// Serial SPI clock (output)
sbit DATA = P1^6;	// MOSI (output)

sbit LD_4351 = P1^5;	// Lock detect for ADF4351BCPZ (input) (MISO)

// Config pins for control MC74HCT160D
sbit CEP_COUNTER = P1^0;	// Count enable (active high) (output)
//sbit PE_COUNTER = P2^0;	// Parallel enable write data into counter (active low) (output)
sbit MR_COUNTER = P2^5;	// Reset counter (data don't cleared) (active low) (output)
sbit D0_COUNTER = P2^4;	// Data Low (output)
sbit D1_COUNTER = P2^3;	// Data Middle (output)
sbit D2_COUNTER = P2^2;	// Data Middle (output)
sbit D3_COUNTER = P2^1;	// Data High (output)

sbit RS_485_EN = P0^3;	// RS-485 receive enable (active low)

// LEDs pins config
sbit POWER_ON = P0^0;	// (active high) (output)
sbit LOCK_4351 = P0^1;	// When ADF4351BCPZ locked PLL (active high) (output)
sbit SPI_LED = P0^7;	// Indicate SPI work (active high) (output)
// blinking - transferring;
// lit after blinking - error - not used;
// not lit after blinking - transfer is over successfully
sbit UART_LED = P2^6;	// Indicate UART work (active high) (output)
// blinking - receiving;
// lit after blinking - error;
// not lit after blinking - receive is over successfully

#define SYSCLK 24500000 // SYSCLK frequency in Hz

unsigned char freq = 0;		    // Frequency number for ADF4351BCPZ
unsigned char phase = 0;		    // Phase for ADF4351BCPZ
unsigned char divFactor = 1;	    // Strobe select factor for MC74HCT160D
unsigned char gainIQ = 0;	    // Gain code for AD8366ACPZ

unsigned char writeData[8];  // Write data buffer
unsigned char readData[8];	 // Read data buffer

bit startByteReaded = 0;            // Start byte read flag
bit stopByteReaded = 0;             // Stop byte read flag
unsigned char byteReadedIndex = 0;  // Current index of reading byte

// INT ratio table for low frequency (ADF4351BCPZ)
unsigned char code INT_LOW[112] = {
    74, 74, 74, 74, 74, 74, 74, 74, 75, 75,
    75, 75, 75, 75, 75, 75, 75, 75, 75, 75,
    75, 75, 75, 75, 76, 76, 76, 76, 76, 76,
    76, 76, 76, 76, 76, 76, 76, 76, 76, 76,
    77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
    77, 77, 77, 77, 77, 77, 78, 78, 78, 78,
    78, 78, 78, 78, 78, 78, 78, 78, 78, 78,
    78, 78, 79, 79, 79, 79, 79, 79, 79, 79,
    79, 79, 79, 79, 79, 79, 79, 79, 80, 80,
    80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
    80, 80, 80, 80, 81, 81, 81, 81, 81, 81,
    81, 81
};

// FRAC ratio table for low frequency (ADF4351BCPZ)
unsigned char code FRAC_LOW[112] = {
    80, 90, 100, 110, 120, 130, 140, 150, 0, 10,
    20, 30, 40, 50, 60, 70, 80, 90, 100, 110,
    120, 130, 140, 150, 0, 10, 20, 30, 40, 50,
    60, 70, 80, 90, 100, 110, 120, 130, 140, 150,
    0, 10, 20, 30, 40, 50, 60, 70, 80, 90,
    100, 110, 120, 130, 140, 150, 0, 10, 20, 30,
    40, 50, 60, 70, 80, 90, 100, 110, 120, 130,
    140, 150, 0, 10, 20, 30, 40, 50, 60, 70,
    80, 90, 100, 110, 120, 130, 140, 150, 0, 10,
    20, 30, 40, 50, 60, 70, 80, 90, 100, 110,
    120, 130, 140, 150, 0, 10, 20, 30, 40, 50,
    60, 70
};

// INT ratio table for high frequency (ADF4351BCPZ)
unsigned char code INT_HIGH[112] = {
    81, 82, 82, 82, 82, 82, 82, 82, 82, 82,
    82, 82, 82, 82, 82, 82, 83, 83, 83, 83,
    83, 83, 83, 83, 83, 83, 83, 83, 83, 83,
    84, 84, 84, 84, 84, 84, 84, 84, 84, 84,
    84, 84, 84, 84, 84, 85, 85, 85, 85, 85,
    85, 85, 85, 85, 85, 85, 85, 85, 85, 86,
    86, 86, 86, 86, 86, 86, 86, 86, 86, 86,
    86, 86, 86, 86, 87, 87, 87, 87, 87, 87,
    87, 87, 87, 87, 87, 87, 87, 87, 88, 88,
    88, 88, 88, 88, 88, 88, 88, 88, 88, 88,
    88, 88, 88, 89, 89, 89, 89, 89, 89, 89,
    89, 89
};

// FRAC ratio table for high frequency (ADF4351BCPZ)
unsigned char code FRAC_HIGH[112] = {
    152, 3, 14, 25, 36, 47, 58, 69, 80, 91,
    102, 113, 124, 135, 146, 157, 8, 19, 30, 41,
    52, 63, 74, 85, 96, 107, 118, 129, 140, 151,
    2, 13, 24, 35, 46, 57, 68, 79, 90, 101,
    112, 123, 134, 145, 156, 7, 18, 29, 40, 51,
    62, 73, 84, 95, 106, 117, 128, 139, 150, 1,
    12, 23, 34, 45, 56, 67, 78, 89, 100, 111,
    122, 133, 144, 155, 6, 17, 28, 39, 50, 61,
    72, 83, 94, 105, 116, 127, 138, 149, 0, 11,
    22, 33, 44, 55, 66, 77, 88, 99, 110, 121,
    132, 143, 154, 5, 16, 27, 38, 49, 60, 71,
    82, 93
};

//------------------------------------------------------------------------------
// Function PROTOTYPES
//------------------------------------------------------------------------------

// Configure Timer0 to delay <ms> milliseconds
void Timer0_ms (unsigned ms);
// Configure Timer0 to delay <us> microseconds
void Timer0_us (unsigned us);
// SPI send subroutine
void SPI_send(unsigned char Data);
// Program ADF4002BCPZ as 128-divider
void ADF4002_divider(void);
// Strobe Selector init (MC74HCT160D)
void strobeSelectInit(void);
// Strobe Selector (MC74HCT160D)
void strobeSelect(char divFactor);
// Gain set (AD8366ACPZ)
void gainSetCode(unsigned char Code);
// Program ADF4351BCPZ init
void ADF4351_init(void);
// Program ADF4351BCPZ for phase and frequency change
void ADF4351_synth(unsigned char INT, unsigned char FRAC, unsigned char PHASE);
// Write to UART0
void writeToUART0(unsigned char* Data, unsigned char bytes);
// Read from UART0
void readFromUART0(void);
// Write information to PC
void infoSend(void);
// Decode commands from PC
void decode(void);
// RES MON handler (write to ADF4351BCPZ)
void resMonHandler(void);
// F-Strobe handler(write to MC74HCT160D)
void fStrobeHandler(void);
// Gain I, Q handler (write to AD8366ACPZ)
void gainIQHandler(void);
// Gain I, Q init
void gainIQInit(void);
// SPI send work
void blink_SPI_LED(unsigned char times);
// UART receive work
void blink_UART_LED(unsigned char times);
// Clearing ReadData array
void clearReadData(void);
// Lock Detect of ADF4351BCPZ
void lockDetect(void);

//------------------------------------------------------------------------------
// main() Routine
//------------------------------------------------------------------------------

void main(void)
{
    Init_Device();
    //IT0 = 1;    // Ext. interrupt 1-0    

    // LEDs init
    POWER_ON = 1;
    LOCK_4351 = 0;
    SPI_LED = 0;
    UART_LED = 0;

    // Deselect chip for SPI (init)
    CS_AMPL = 0;
    LE_4351 = 1;
    LE_4002 = 1;

    // Enable receive via RS-485
    RS_485_EN = 0;

    ADF4002_divider();  // Init divider
    ADF4351_init();    // Init synth
    gainIQInit();       // Init Gain I, Q
	strobeSelectInit(); // Init Strobe Select	
    
    while(1)
    {
        infoSend();
        Timer0_ms(1000);    // Delay 1 sec        
        
        if(startByteReaded && stopByteReaded) {            
            stopByteReaded = 0;
            startByteReaded = 0;
            decode();            
        }
    }
}

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

// SPI send subroutine
void SPI_send(unsigned char Data)
{
    EA = 0;    
    while(!TXBMT);
    SPIF = 0;
    SPI0DAT = Data;
    while(!TXBMT);
    SPIF = 0;
    Timer0_us(1000);
    EA = 1;
}

// Program ADF4002BCPZ as 128-divider
void ADF4002_divider(void)
{	
    // init latch
    LE_4002 = 0;	// Select ADF4002BCPZ
    Timer0_ms(1);
    SPI_send(0x1F);
    SPI_send(0xA6);
    SPI_send(0xA3);
    LE_4002 = 1;	// Deselect ADF4002BCPZ
    Timer0_ms(1);
    
    // func latch
    LE_4002 = 0;	// Select ADF4002BCPZ
    Timer0_ms(1);
    SPI_send(0x1F);
    SPI_send(0xA6);
    SPI_send(0xA2);
    LE_4002 = 1;	// Deselect ADF4002BCPZ
    Timer0_ms(1);
    
    // R-counter
    LE_4002 = 0;	// Select ADF4002BCPZ
    Timer0_ms(1);
    SPI_send(0x12);
    SPI_send(0x00);
    SPI_send(0x04);
    LE_4002 = 1;	// Deselect ADF4002BCPZ
    Timer0_ms(1);
    
    // N-counter
    LE_4002 = 0;	// Select ADF4002BCPZ
    Timer0_ms(1);
    SPI_send(0x20);
    SPI_send(0x80);
    SPI_send(0x01);
    LE_4002 = 1;	// Deselect ADF4002BCPZ
    Timer0_ms(1);    

    blink_SPI_LED(3);   // Packets send indicate
}

// Strobe Selector init (MC74HCT160D)
void strobeSelectInit(void)
{
    MR_COUNTER = 0;	// Reset counter (data don't cleared)
    Timer0_ms(1);
    MR_COUNTER = 1;	// Cancel Reset counter    
    Timer0_ms(1);	
    CEP_COUNTER = 0;// Count disable
}
    
// Strobe Selector (MC74HCT160D)
void strobeSelect(unsigned char divFactor)
{    
    switch(divFactor)
    {
    case 1:
        CEP_COUNTER = 0;	// Count disable
        break;
    case 2:
		CEP_COUNTER = 0;	// Count disable
		Timer0_ms(1);
        D0_COUNTER = 0;	// Data Low
        D1_COUNTER = 0;	// Data Middle
        D2_COUNTER = 0;	// Data Middle
        D3_COUNTER = 1;	// Data High
		Timer0_ms(1);
		CEP_COUNTER = 1;// Count enable
        break;
    case 3:
		CEP_COUNTER = 0;	// Count disable
		Timer0_ms(1);
        D0_COUNTER = 1;	// Data Low
        D1_COUNTER = 1;	// Data Middle
        D2_COUNTER = 1;	// Data Middle
        D3_COUNTER = 0;	// Data High
		Timer0_ms(1);
		CEP_COUNTER = 1;// Count enable
        break;
    case 4:
		CEP_COUNTER = 0;	// Count disable
		Timer0_ms(1);
        D0_COUNTER = 0;	// Data Low
        D1_COUNTER = 1;	// Data Middle
        D2_COUNTER = 1;	// Data Middle
        D3_COUNTER = 0;	// Data High
		Timer0_ms(1);
		CEP_COUNTER = 1;// Count enable
        break;
        /*case 5:
		CEP_COUNTER = 1;// Count enable
		Timer0_ms(1);
        D0_COUNTER = 1;	// Data Low
        D1_COUNTER = 0;	// Data Middle
        D2_COUNTER = 1;	// Data Middle
        D3_COUNTER = 0;	// Data High
        break;*/
    case 6:
		CEP_COUNTER = 0;	// Count disable
		Timer0_ms(1);
        D0_COUNTER = 0;	// Data Low
        D1_COUNTER = 0;	// Data Middle
        D2_COUNTER = 1;	// Data Middle
        D3_COUNTER = 0;	// Data High
		Timer0_ms(1);
		CEP_COUNTER = 1;// Count enable
        break;
        /*case 7:
		CEP_COUNTER = 1;// Count enable
		Timer0_ms(1);
        D0_COUNTER = 1;	// Data Low
        D1_COUNTER = 1;	// Data Middle
        D2_COUNTER = 0;	// Data Middle
        D3_COUNTER = 0;	// Data High
        break;*/
    case 8:
		CEP_COUNTER = 0;	// Count disable
		Timer0_ms(1);
        D0_COUNTER = 0;	// Data Low
        D1_COUNTER = 1;	// Data Middle
        D2_COUNTER = 0;	// Data Middle
        D3_COUNTER = 0;	// Data High
		Timer0_ms(1);
		CEP_COUNTER = 1;// Count enable
        break;
    case 9:
		CEP_COUNTER = 0;	// Count disable
		Timer0_ms(1);
        D0_COUNTER = 1;	// Data Low
        D1_COUNTER = 0;	// Data Middle
        D2_COUNTER = 0;	// Data Middle
        D3_COUNTER = 0;	// Data High
		Timer0_ms(1);
		CEP_COUNTER = 1;// Count enable
        break;
    default: break;
    }        
}

// Gain set (AD8366ACPZ)
void gainSetCode(unsigned char Code)
{
    CS_AMPL = 1;	// Select AD8366ACPZ
    Timer0_ms(1);
    SPI_send(Code);	// Setting differential output A gain
    SPI_send(Code);	// Setting differential output B gain
    Timer0_ms(1);
    CS_AMPL = 0;	// Deselect AD8366ACPZ

    blink_SPI_LED(3);   // Packets send indicate
}

// Program ADF4351BCPZ init
void ADF4351_init(void)
{
    // REG5
    LE_4351 = 0;    // Select ADF4351BCPZ   
    Timer0_ms(1);
    SPI_send(0x00);	
    SPI_send(0x58);	
    SPI_send(0x00);	
    SPI_send(0x05);
    LE_4351 = 1;    // Deselect ADF4351BCPZ
    Timer0_ms(1);

    // REG4
    LE_4351 = 0;    // Select ADF4351BCPZ
    Timer0_ms(1);
    SPI_send(0x00);	
    SPI_send(0xBF);	
    SPI_send(0xA5);	
    SPI_send(0xFC);
    LE_4351 = 1;    // Deselect ADF4351BCPZ
    Timer0_ms(1);

    // REG3
    LE_4351 = 0;    // Select ADF4351BCPZ
    Timer0_ms(1);
    SPI_send(0x00);	
    SPI_send(0x00);	
    SPI_send(0x04);	
    SPI_send(0xB3);
    LE_4351 = 1;    // Deselect ADF4351BCPZ
    Timer0_ms(1);

    // REG2
    LE_4351 = 0;    // Select ADF4351BCPZ
    Timer0_ms(1);
    SPI_send(0x0D);	
    SPI_send(0x01);	
    SPI_send(0x0E);	
    SPI_send(0x42);
    LE_4351 = 1;    // Deselect ADF4351BCPZ
    Timer0_ms(1);    
    
    // REG1
    LE_4351 = 0;    // Select ADF4351BCPZ	
    SPI_send(0x00); // prescaler 4/5
    SPI_send(0x00);	
    SPI_send(0x85);	
    SPI_send(0x01);    
    LE_4351 = 1;    // Deselect ADF4351BCPZ
    Timer0_ms(1);

    // REG0
    LE_4351 = 0;    // Select ADF4351BCPZ
    SPI_send(0x00);	
    SPI_send(0x25);	
    SPI_send(0x02);	
    SPI_send(0x80);
    Timer0_ms(1);
    LE_4351 = 1;    // Deselect ADF4351BCPZ        
    
    blink_SPI_LED(3);   // Packets send indicate
}

// Program ADF4351BCPZ for phase and frequency change
void ADF4351_synth(unsigned char INT, unsigned char FRAC, unsigned char PHASE)
{    
    // REG5
    LE_4351 = 0;    // Select ADF4351BCPZ   
    Timer0_ms(1);
    SPI_send(0x00);	
    SPI_send(0x58);	
    SPI_send(0x00);	
    SPI_send(0x05);
    LE_4351 = 1;    // Deselect ADF4351BCPZ
    Timer0_ms(1);

    // REG4
    LE_4351 = 0;    // Select ADF4351BCPZ
    Timer0_ms(1);
    SPI_send(0x00);	
    SPI_send(0xBF);	
    SPI_send(0xA5);	
    SPI_send(0xFC);
    LE_4351 = 1;    // Deselect ADF4351BCPZ
    Timer0_ms(1);

    // REG3
    LE_4351 = 0;    // Select ADF4351BCPZ
    Timer0_ms(1);
    SPI_send(0x00);	
    SPI_send(0x00);	
    SPI_send(0x04);	
    SPI_send(0xB3);
    LE_4351 = 1;    // Deselect ADF4351BCPZ
    Timer0_ms(1);

    // REG2
    LE_4351 = 0;    // Select ADF4351BCPZ
    Timer0_ms(1);
    SPI_send(0x0D);	
    SPI_send(0x01);	
    SPI_send(0x0E);	
    SPI_send(0x42);
    LE_4351 = 1;    // Deselect ADF4351BCPZ
    Timer0_ms(1);
    
    // REG1
    LE_4351 = 0;    // Select ADF4351BCPZ
    Timer0_ms(1);
    if(INT >= 75) {  // prescaler 8/9
        SPI_send(0x08);
    } else {    	// prescaler 4/5
        SPI_send(0x00);
    }	
    SPI_send(PHASE >> 1);	
    SPI_send( (PHASE << 7) | (0x05) );	
    SPI_send(0x01);    
    LE_4351 = 1;    // Deselect ADF4351BCPZ
    Timer0_ms(1);

    // REG0
    LE_4351 = 0;    // Select ADF4351BCPZ
    Timer0_ms(1);
    SPI_send(0x00);	
    SPI_send(INT >> 1);	
    SPI_send( (INT << 7) | (FRAC >> 5) );	
    SPI_send(FRAC << 3);
    Timer0_ms(1);
    LE_4351 = 1;    // Deselect ADF4351BCPZ
    
    // One more time for phase record with adjust phase bit set
    // REG1
    LE_4351 = 0;    // Select ADF4351BCPZ
    Timer0_ms(1);
    if(INT >= 75) {  // prescaler 8/9
        SPI_send(0x18);
    } else {    	// prescaler 4/5
        SPI_send(0x10);
    }	
    SPI_send(PHASE >> 1);	
    SPI_send( (PHASE << 7) | (0x05) );	
    SPI_send(0x01);    
    LE_4351 = 1;    // Deselect ADF4351BCPZ
    Timer0_ms(1);

    // REG0
    LE_4351 = 0;    // Select ADF4351BCPZ
    Timer0_ms(1);
    SPI_send(0x00);	
    SPI_send(INT >> 1);	
    SPI_send( (INT << 7) | (FRAC >> 5) );	
    SPI_send(FRAC << 3);
    Timer0_ms(1);
    LE_4351 = 1;    // Deselect ADF4351BCPZ
        
    blink_SPI_LED(3);   // Packets send indicate
}

// Write to UART0
void writeToUART0(unsigned char* Data, unsigned char bytes)
{
    unsigned char i;
    EA = 0;
    ES0 = 0;
    RS_485_EN = 1;	// Enable RS-485 to send data
    Timer0_ms(1);
    for(i = 0; i < bytes; ++i) {
        TI0 = 0;
        SBUF0 = Data[i];

        while(TI0 == 0);	// Wait for end data transmit
    }
    RS_485_EN = 0;	// Enable RS-485 to receive data
    Timer0_ms(1);
    TI0 = 0;
    RI0 = 0;
    ES0 = 1;
    EA = 1;
}

// Read from UART0
void readFromUART0(void) interrupt 4
{    
    EA = 0;
    RI0 = 0;
    TI0 = 0;
    
    if(SBUF0 == 0x55 && !startByteReaded && byteReadedIndex == 0) {
        readData[0] = SBUF0;            
        startByteReaded = 1;        
    } else if(SBUF0 == 0xAA && startByteReaded && byteReadedIndex == 6) {
        readData[7] = SBUF0;
        stopByteReaded = 1;
        byteReadedIndex = 0;
        blink_UART_LED(3);  // 8 byte receive indicate                 
    } else if(startByteReaded && !stopByteReaded) {
        readData[++byteReadedIndex] = SBUF0;            
    } else {
        UART_LED = 1;       // Error occurred!!!
        Timer0_ms(1000);    // Delay to indicate an error
        UART_LED = 0;        
    }       
    
    RI0 = 0;
    TI0 = 0;
    EA = 1;
}

// Write information to PC
void infoSend(void)
{
    writeData[0] = 0x55;
    writeData[1] = freq;
    writeData[2] = phase;
    writeData[3] = divFactor;
    writeData[4] = gainIQ;
    writeData[5] = 0x00;
    writeData[6] = 0x00;
    writeData[7] = 0xAA;

    Timer0_ms(2);

    writeToUART0(writeData, 8);
    blink_UART_LED(1);  // Bytes send indicate
}

// Decode commands from PC
void decode(void)
{
    switch(readData[1]) {
    case 0x01:
        resMonHandler();
        break;
    case 0x02:
        fStrobeHandler();
        break;
    case 0x03:
        gainIQHandler();
        break;
    default:
        UART_LED = 1;       // Error occurred!!!
        Timer0_ms(1000);    // Delay to indicate an error
        UART_LED = 0;
        break;
    }
}

// RES MON handler (write to ADF4351BCPZ)
void resMonHandler(void)
{
    freq = readData[2];
    phase = readData[3];
    if(freq & 0x80) {
        ADF4351_synth(INT_HIGH[freq & 0x7F], FRAC_HIGH[freq & 0x7F], phase);
    } else {
        ADF4351_synth(INT_LOW[freq], FRAC_LOW[freq], phase);
    }
}

// F-Strobe handler(write to MC74HCT160D)
void fStrobeHandler(void)
{
    divFactor = readData[2];
    strobeSelect(divFactor);
}

// Gain I, Q handler (write to AD8366ACPZ)
void gainIQHandler(void)
{
    gainIQ = readData[2];
    gainSetCode(gainIQ);
}

// Gain I, Q init
void gainIQInit(void)
{
    gainSetCode(gainIQ);
}

// SPI send work
void blink_SPI_LED(unsigned char times)
{
    unsigned char i;
    for(i = 0; i < times; ++i) {
        SPI_LED = 1;
        Timer0_ms(100);
        SPI_LED = 0;
        Timer0_ms(100);
    }
}

// UART receive work
void blink_UART_LED(unsigned char times)
{
    unsigned char i;
    for(i = 0; i < times; ++i) {
        UART_LED = 1;
        Timer0_ms(100);
        UART_LED = 0;
        Timer0_ms(100);
    }
}

// Clearing ReadData array
void clearReadData(void)
{
    unsigned char i;
    for(i = 0; i < 8; ++i) {
        readData[i] = 0x00;
    }
}

// Lock Detect of ADF4351BCPZ
void lockDetect(void) interrupt 0
{
    EA = 0;
    IE0 = 0;
    LOCK_4351 = 1;
    Timer0_ms(100);
    LOCK_4351 = 0;
    Timer0_ms(100);
    EA = 1;
}
