#include "LCD_Drivers.h"
#include "msp.h"
#include "driverlib.h"
#include "AsciiLib.h"

static void Delay(unsigned long interval)
{
    while(interval > 0)
    {
        __delay_cycles(48000);
        interval--;
    }
}

static void LCD_initSPI()
{
    /* P10.1 - CLK
     * P10.2 - MOSI
     * P10.3 - MISO
     * P10.4 - LCD CS
     * P10.5 - TP CS
     */


    //Reset the SPI module
    EUSCI_B3->CTLW0 = UCSWRST;

    //3-Pin mode; Polarity and Phase mode; MSB select; SMCLK clock select;
    EUSCI_B3->CTLW0 = (UCCKPL | UCMSB | UCMST | UCMODE_0 | UCSYNC | UCSSEL_3);

    //SMCLK at 12MHz, prescalar at 1
    EUSCI_B3->BRW = 1;

    //Pin function selection
    P10->SEL0 |= (BIT1 | BIT2 | BIT3);
    P10->SEL1 &= ~(BIT1 | BIT2 | BIT3);

    //Pin's set to High and Outputs
    P10->SEL0 &= ~(BIT4|BIT5);
    P10->SEL1 &= ~(BIT4|BIT5);
    P10->OUT |= (BIT4 | BIT5);
    P10->DIR |= (BIT4 | BIT5);

    //SPI Module re-enabled
    EUSCI_B3->CTLW0 &= ~UCSWRST;
}

static void LCD_reset()
{
    P10DIR |= BIT0;
    P10OUT |= BIT0;  // high
    Delay(100);
    P10OUT &= ~BIT0; // low
    Delay(100);
    P10OUT |= BIT0;  // high
}

inline void LCD_ResetRamArea(){
    /* Set area back to span the entire LCD */
    LCD_WriteReg(HOR_ADDR_START_POS, 0x0000);
    LCD_WriteReg(HOR_ADDR_END_POS, (MAX_SCREEN_Y - 1));
    LCD_WriteReg(VERT_ADDR_START_POS, 0x0000);
    LCD_WriteReg(VERT_ADDR_END_POS, (MAX_SCREEN_X - 1));
}

void LCD_DrawHorizontalLine(uint16_t Xstart, uint16_t Xend, uint16_t Ycoordinate, uint16_t color){
    if( Xend > MAX_SCREEN_X - 1 | Ycoordinate > MAX_SCREEN_Y - 1) return;

    /* Set window area for high-speed RAM write */
    LCD_WriteReg(HOR_ADDR_START_POS, Ycoordinate);
    LCD_WriteReg(HOR_ADDR_END_POS, Ycoordinate);
    LCD_WriteReg(VERT_ADDR_START_POS, Xstart);
    LCD_WriteReg(VERT_ADDR_END_POS, Xend);

    /* Set cursor */
    LCD_SetCursor(Xstart, Ycoordinate);

    /* Set index to GRAM */
    LCD_WriteIndex(GRAM);

    /* Send out data only to the entire area */
    SPI_CS_LOW;
    LCD_Write_Data_Start();

    for(int i = 0; i < Xend - Xstart + 1; i++){
        LCD_Write_Data_Only(color);
    }
    SPI_CS_HIGH;
}

void LCD_DrawVerticalLine(uint16_t Xcoordinate, uint16_t Ystart, uint16_t Yend, uint16_t color){
    /* Check special cases for out of bounds */
    if( Xcoordinate > MAX_SCREEN_X - 1 | Yend > MAX_SCREEN_Y - 1) return;

    /* Set window area for high-speed RAM write */
    LCD_WriteReg(HOR_ADDR_START_POS, Ystart);
    LCD_WriteReg(HOR_ADDR_END_POS, Yend);
    LCD_WriteReg(VERT_ADDR_START_POS, Xcoordinate);
    LCD_WriteReg(VERT_ADDR_END_POS, Xcoordinate);


    /* Set cursor */
    LCD_SetCursor(Xcoordinate, Ystart);

    /* Set index to GRAM */
    LCD_WriteIndex(GRAM);

    /* Send out data only to the entire area */
    SPI_CS_LOW;
    LCD_Write_Data_Start();

    for(int i = 0; i < Yend - Ystart + 1; i++){
        LCD_Write_Data_Only(color);
    }
    SPI_CS_HIGH;
}

void LCD_DrawRectangle(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend, uint16_t color){
    /* Check special cases for out of bounds */
    if( Xend > MAX_SCREEN_X - 1 | Yend > MAX_SCREEN_Y - 1) return;

    /* Set window area for high-speed RAM write */
    LCD_WriteReg(HOR_ADDR_START_POS, Ystart);
    LCD_WriteReg(HOR_ADDR_END_POS, Yend);
    LCD_WriteReg(VERT_ADDR_START_POS, Xstart);
    LCD_WriteReg(VERT_ADDR_END_POS, Xend);

    /* Set cursor */
    LCD_SetCursor(Xstart, Ystart);

    /* Set index to GRAM */
    LCD_WriteIndex(GRAM);

    /* Send out data only to the entire area */
    SPI_CS_LOW;
    LCD_Write_Data_Start();

    for(int i = 0; i < (Xend-Xstart+1)*(Yend-Ystart+1); i++){
        LCD_Write_Data_Only(color);
    }
    SPI_CS_HIGH;
}

inline void PutChar( uint16_t Xpos, uint16_t Ypos, uint8_t ASCI, uint16_t charColor)
{
    uint16_t i, j;
    uint8_t buffer[16], tmp_char;
    GetASCIICode(buffer,ASCI);  /* get font data */
    for( i=0; i<16; i++ )
    {
        tmp_char = buffer[i];
        for( j=0; j<8; j++ )
        {
            if( (tmp_char >> 7 - j) & 0x01 == 0x01 )
            {
                LCD_SetPoint( Xpos + j, Ypos + i, charColor );  /* Character color */
            }
        }
    }
}

void LCD_Text(uint16_t Xpos, uint16_t Ypos, uint8_t *str, uint16_t Color)
{
    uint8_t TempChar;

    /* Set area back to span the entire LCD */
    LCD_WriteReg(HOR_ADDR_START_POS, 0x0000);     /* Horizontal GRAM Start Address */
    LCD_WriteReg(HOR_ADDR_END_POS, (MAX_SCREEN_Y - 1));  /* Horizontal GRAM End Address */
    LCD_WriteReg(VERT_ADDR_START_POS, 0x0000);    /* Vertical GRAM Start Address */
    LCD_WriteReg(VERT_ADDR_END_POS, (MAX_SCREEN_X - 1)); /* Vertical GRAM Start Address */
    do
    {
        TempChar = *str++;
        PutChar( Xpos, Ypos, TempChar, Color);
        if( Xpos < MAX_SCREEN_X - 8)
        {
            Xpos += 8;
        }
        else if ( Ypos < MAX_SCREEN_X - 16)
        {
            Xpos = 0;
            Ypos += 16;
        }
        else
        {
            Xpos = 0;
            Ypos = 0;
        }
    }
    while ( *str != 0 );
}

void LCD_Clear(uint16_t Color)
{
    /* Set area back to span the entire LCD */
    LCD_WriteReg(HOR_ADDR_START_POS, 0x0000);
    LCD_WriteReg(HOR_ADDR_END_POS, (MAX_SCREEN_Y - 1));
    LCD_WriteReg(VERT_ADDR_START_POS, 0x0000);
    LCD_WriteReg(VERT_ADDR_END_POS, (MAX_SCREEN_X - 1));

    /* Set cursor to (0,0) */
    LCD_SetCursor(MIN_SCREEN_X, MIN_SCREEN_Y);

    /* Set write index to GRAM */
    LCD_WriteIndex(GRAM);

    /* Start data transmittion */
    SPI_CS_LOW;
    LCD_Write_Data_Start();

    for(int i = 0; i < SCREEN_SIZE; i++){
        LCD_Write_Data_Only(Color);
    }
    SPI_CS_HIGH;
}

static void LCD_DrawLineLoop(uint16_t SweepStart, uint16_t IncStart, uint16_t SweepEnd, uint16_t IncEnd, uint16_t color, void (*DrawFunction)(uint16_t , uint16_t, uint16_t)){
    int dSweep, dInc, p, Sweep, Inc, SweepStep = 1, IncStep = 1;

    //Get Slope
    dSweep = SweepEnd - SweepStart;
    dInc = IncEnd - IncStart;

    Inc = IncStart;
    Sweep = SweepStart;

    //Account for negative sweeps
    if(dSweep < 0) {
        dSweep = -dSweep; SweepStep = -1; uint16_t temp = SweepEnd; SweepEnd = SweepStart; SweepStart = temp;
    }

    //Account for negative increments
    if(dInc < 0) {
        dInc = -dInc; IncStep = -1;
    }

    //Bresenham's Algorithm
    p = (dInc - dSweep) << 1;
    while(Sweep <= SweepEnd && Sweep >= SweepStart){
        DrawFunction(Sweep, Inc, color);
        if(p >= 0){
            Inc += IncStep;
            p = p + ((dInc - dSweep) << 1);
        }
        else{
            p = p + (dInc << 1);
        }
        Sweep += SweepStep;
    }
}

void LCD_DrawLine(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend, uint16_t color){
    int dx, dy;

    //Border Check
    if(Xstart > MAX_SCREEN_X - 1 | Xend > MAX_SCREEN_X - 1 |Yend > MAX_SCREEN_Y - 1 | Ystart > MAX_SCREEN_Y - 1 ) return;

    //Get Magnitude
    dx = Xend - Xstart;
    dy = Yend - Ystart;

    //Get Magnitude of Deltas
    if(dx >= 0 && dy >= 0){}
    else if(dx >= 0 && dy < 0){
        dy = -dy;
    }
    else if(dx < 0 && dy >= 0){
        dx = -dx;
    }
    else{
        dx = -dx;
        dy = -dy;
    }

    //Check Magnitudes to determine sweep axis
    if(dx >= dy){
        LCD_DrawLineLoop(Xstart,Ystart,Xend,Yend,color,LCD_SetPoint);
    }
    else{
        LCD_DrawLineLoop(Ystart,Xstart,Yend,Xend,color,LCD_SetPointFlip);
    }
}


void LCD_SetPoint(uint16_t Xpos, uint16_t Ypos, uint16_t color)
{
    /* Should check for out of bounds */
    if(Xpos > MAX_SCREEN_X - 1 | Ypos > MAX_SCREEN_Y - 1) return;

    /* Set cursor to Xpos and Ypos */
    LCD_SetCursor(Xpos, Ypos);

    /* Write color to GRAM reg */
    LCD_WriteReg(GRAM, color);
}

void LCD_SetPointFlip(uint16_t Xpos, uint16_t Ypos, uint16_t color){
    LCD_SetPoint(Ypos, Xpos, color);
}

inline void LCD_Write_Data_Only(uint16_t data)
{
    /* Send out MSB */
    SPISendRecvByte((data >> 8));
    SPISendRecvByte((data & 0xFF));
}

inline void LCD_WriteData(uint16_t data)
{
    SPI_CS_LOW;

    SPISendRecvByte(SPI_START | SPI_WR | SPI_DATA);    /* Write : RS = 1, RW = 0       */
    SPISendRecvByte((data >>   8));                    /* Write D8..D15                */
    SPISendRecvByte((data & 0xFF));                    /* Write D0..D7                 */

    SPI_CS_HIGH;
}

inline uint16_t LCD_ReadReg(uint16_t LCD_Reg)
{
    /* Write 16-bit Index */
    LCD_WriteIndex(LCD_Reg);

    /* Return 16-bit Reg using LCD_ReadData() */
    return LCD_ReadData();
}

inline void LCD_WriteIndex(uint16_t index)
{
    SPI_CS_LOW;

    /* SPI write data */
    SPISendRecvByte(SPI_START | SPI_WR | SPI_INDEX);   /* Write : RS = 0, RW = 0  */
    SPISendRecvByte(0);
    SPISendRecvByte(index);

    SPI_CS_HIGH;
}

inline uint8_t SPISendRecvByte (uint8_t byte)
{
    DebugToolsUp();
    /* Send byte of data */
    SPI_transmitData(EUSCI_B3_BASE, byte);

    /* Wait as long as busy */
    while(SPI_isBusy(EUSCI_B3_BASE));
    DebugToolsDown();

    /* Return received value*/
    return SPI_receiveData(EUSCI_B3_BASE);
}
inline void LCD_Write_Data_Start(void)
{
    SPISendRecvByte(SPI_START | SPI_WR | SPI_DATA);    /* Write : RS = 1, RW = 0 */
}

inline uint16_t LCD_ReadData()
{
    uint16_t value;
    SPI_CS_LOW;

    SPISendRecvByte(SPI_START | SPI_RD | SPI_DATA);   /* Read: RS = 1, RW = 1   */

    SPISendRecvByte(0);                               /* Dummy read 1           */
    SPISendRecvByte(0);                               /* Dummy read 1           */
    SPISendRecvByte(0);                               /* Dummy read 1           */
    SPISendRecvByte(0);                               /* Dummy read 1           */
    SPISendRecvByte(0);                               /* Dummy read 1           */

    value = (SPISendRecvByte(0) << 8);                /* Read D8..D15           */
    value |= SPISendRecvByte(0);                      /* Read D0..D7            */

    SPI_CS_HIGH;
    return value;
}

uint16_t LCD_ReadPixelColor(uint16_t x, uint16_t y){
    LCD_SetCursor(x,y);
    LCD_WriteIndex(GRAM);
    return LCD_ReadData();
}

inline void LCD_WriteReg(uint16_t LCD_Reg, uint16_t LCD_RegValue)
{
    /* Write 16-bit Index */
    LCD_WriteIndex(LCD_Reg);

    /* Write 16-bit Reg Data */
    LCD_WriteData(LCD_RegValue);
}

inline void LCD_SetCursor(uint16_t Xpos, uint16_t Ypos )
{
    /* Should just be two LCD_WriteReg to appropriate registers */

    /* Set horizonal GRAM coordinate (Ypos) */
    LCD_WriteReg(HORIZONTAL_GRAM_SET, Ypos);

    /* Set vertical GRAM coordinate (Xpos) */
    LCD_WriteReg(VERTICAL_GRAM_SET, Xpos);

}

void LCD_Init()
{
    LCD_initSPI();
    LCD_reset();

    LCD_WriteReg(0xE5, 0x78F0); /* set SRAM internal timing */
    LCD_WriteReg(DRIVER_OUTPUT_CONTROL, 0x0100); /* set Driver Output Control */
    LCD_WriteReg(DRIVING_WAVE_CONTROL, 0x0700); /* set 1 line inversion */
    LCD_WriteReg(ENTRY_MODE, 0x1038); /* set GRAM write direction and BGR=1 */
    LCD_WriteReg(RESIZING_CONTROL, 0x0000); /* Resize register */
    LCD_WriteReg(DISPLAY_CONTROL_2, 0x0207); /* set the back porch and front porch */
    LCD_WriteReg(DISPLAY_CONTROL_3, 0x0000); /* set non-display area refresh cycle ISC[3:0] */
    LCD_WriteReg(DISPLAY_CONTROL_4, 0x0000); /* FMARK function */
    LCD_WriteReg(RGB_DISPLAY_INTERFACE_CONTROL_1, 0x0000); /* RGB interface setting */
    LCD_WriteReg(FRAME_MARKER_POSITION, 0x0000); /* Frame marker Position */
    LCD_WriteReg(RGB_DISPLAY_INTERFACE_CONTROL_2, 0x0000); /* RGB interface polarity */

    /* Power On sequence */
    LCD_WriteReg(POWER_CONTROL_1, 0x0000); /* SAP, BT[3:0], AP, DSTB, SLP, STB */
    LCD_WriteReg(POWER_CONTROL_2, 0x0007); /* DC1[2:0], DC0[2:0], VC[2:0] */
    LCD_WriteReg(POWER_CONTROL_3, 0x0000); /* VREG1OUT voltage */
    LCD_WriteReg(POWER_CONTROL_4, 0x0000); /* VDV[4:0] for VCOM amplitude */
    LCD_WriteReg(DISPLAY_CONTROL_1, 0x0001);
    Delay(200);

    /* Dis-charge capacitor power voltage */
    LCD_WriteReg(POWER_CONTROL_1, 0x1090); /* SAP, BT[3:0], AP, DSTB, SLP, STB */
    LCD_WriteReg(POWER_CONTROL_2, 0x0227); /* Set DC1[2:0], DC0[2:0], VC[2:0] */
    Delay(50); /* Delay 50ms */
    LCD_WriteReg(POWER_CONTROL_3, 0x001F);
    Delay(50); /* Delay 50ms */
    LCD_WriteReg(POWER_CONTROL_4, 0x1500); /* VDV[4:0] for VCOM amplitude */
    LCD_WriteReg(POWER_CONTROL_7, 0x0027); /* 04 VCM[5:0] for VCOMH */
    LCD_WriteReg(FRAME_RATE_AND_COLOR_CONTROL, 0x000D); /* Set Frame Rate */
    Delay(50); /* Delay 50ms */
    LCD_WriteReg(GRAM_HORIZONTAL_ADDRESS_SET, 0x0000); /* GRAM horizontal Address */
    LCD_WriteReg(GRAM_VERTICAL_ADDRESS_SET, 0x0000); /* GRAM Vertical Address */

    /* Adjust the Gamma Curve */
    LCD_WriteReg(GAMMA_CONTROL_1,    0x0000);
    LCD_WriteReg(GAMMA_CONTROL_2,    0x0707);
    LCD_WriteReg(GAMMA_CONTROL_3,    0x0307);
    LCD_WriteReg(GAMMA_CONTROL_4,    0x0200);
    LCD_WriteReg(GAMMA_CONTROL_5,    0x0008);
    LCD_WriteReg(GAMMA_CONTROL_6,    0x0004);
    LCD_WriteReg(GAMMA_CONTROL_7,    0x0000);
    LCD_WriteReg(GAMMA_CONTROL_8,    0x0707);
    LCD_WriteReg(GAMMA_CONTROL_9,    0x0002);
    LCD_WriteReg(GAMMA_CONTROL_10,   0x1D04);

    /* Set GRAM area */
    LCD_WriteReg(HOR_ADDR_START_POS, 0x0000);     /* Horizontal GRAM Start Address */
    LCD_WriteReg(HOR_ADDR_END_POS, (MAX_SCREEN_Y - 1));  /* Horizontal GRAM End Address */
    LCD_WriteReg(VERT_ADDR_START_POS, 0x0000);    /* Vertical GRAM Start Address */
    LCD_WriteReg(VERT_ADDR_END_POS, (MAX_SCREEN_X - 1)); /* Vertical GRAM Start Address */
    LCD_WriteReg(GATE_SCAN_CONTROL_0X60, 0x2700); /* Gate Scan Line */
    LCD_WriteReg(GATE_SCAN_CONTROL_0X61, 0x0001); /* NDL,VLE, REV */
    LCD_WriteReg(GATE_SCAN_CONTROL_0X6A, 0x0000); /* set scrolling line */

    /* Partial Display Control */
    LCD_WriteReg(PART_IMAGE_1_DISPLAY_POS, 0x0000);
    LCD_WriteReg(PART_IMG_1_START_END_ADDR_0x81, 0x0000);
    LCD_WriteReg(PART_IMG_1_START_END_ADDR_0x82, 0x0000);
    LCD_WriteReg(PART_IMAGE_2_DISPLAY_POS, 0x0000);
    LCD_WriteReg(PART_IMG_2_START_END_ADDR_0x84, 0x0000);
    LCD_WriteReg(PART_IMG_2_START_END_ADDR_0x85, 0x0000);

    /* Panel Control */
    LCD_WriteReg(PANEL_ITERFACE_CONTROL_1, 0x0010);
    LCD_WriteReg(PANEL_ITERFACE_CONTROL_2, 0x0600);
    LCD_WriteReg(DISPLAY_CONTROL_1, 0x0133); /* 262K color and display ON */
    Delay(50); /* delay 50 ms */

    LCD_Clear(LCD_BLACK);
}
