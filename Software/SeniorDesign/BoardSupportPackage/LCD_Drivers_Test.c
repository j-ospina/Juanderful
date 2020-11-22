#include <time.h>
#include "LCD_Drivers.h"
#include "msp.h"
#include "driverlib.h"

static void Delay(unsigned long interval)
{
    while(interval > 0)
    {
        __delay_cycles(48000);
        interval--;
    }
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

static void LCD_initSPI(){
    /*
    const eUSCI_SPI_MasterConfig LCD_SpiConfiguration = {
        EUSCI_SPI_CLOCKSOURCE_SMCLK,
        12000000,
        12000000,
        EUSCI_SPI_MSB_FIRST,
        EUSCI_SPI_PHASE_DATA_CHANGED_ONFIRST_CAPTURED_ON_NEXT,
        EUSCI_SPI_CLOCKPOLARITY_INACTIVITY_HIGH,
        EUSCI_SPI_3PIN
    };

    GPIO_setAsOutputPin(GPIO_PORT_P10, GPIO_PIN4);
    GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P10, GPIO_PIN1, GPIO_PRIMARY_MODULE_FUNCTION);
    GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P10, GPIO_PIN3, GPIO_PRIMARY_MODULE_FUNCTION);
    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P10, GPIO_PIN2, GPIO_PRIMARY_MODULE_FUNCTION);
    SPI_initMaster(EUSCI_B3_BASE, &LCD_SpiConfiguration);
    */

    EUSCI_B3->CTLW0 = UCSWRST;

    //3 pin, 8-bit spi master, high polarity for inactive state, 12MHz, MSB
    EUSCI_B3->CTLW0 = (UCCKPL | UCMSB |UCMST | UCMODE_0 | UCSYNC | UCSSEL_3);
    EUSCI_B3->BRW = 3;

    //Enable port 10 CLK, MOSI, MISO, P10.4 as outputs
    P10->SEL0 = 0b00001110;
    P10->SEL1 = 0b00000000;
    P10->DIR |= BIT4 | BIT5;

    EUSCI_B3->CTLW0 &= ~UCSWRST;
}

inline uint8_t SPISendRecvByte (uint8_t byte)
{
    uint8_t readData;
    /* Send byte of data */

    while(!(EUSCI_B3->IFG & 2)); //Wait for transmit buffer empty
    EUSCI_B3->TXBUF = byte;

    while(EUSCI_B3->STATW & 1);  //Wait till transmission is done

    readData = EUSCI_B3->RXBUF;

    return readData;
}

inline void LCD_Write_Data_Only(uint16_t data)
{
    /* Send out MSB */
    SPISendRecvByte(data >> 8);

    /* Send out LSB */
    SPISendRecvByte(data);
}

inline void LCD_Write_Data_Start(void)
{
    SPISendRecvByte(SPI_START | SPI_WR | SPI_DATA);    /* Write : RS = 1, RW = 0 */
}

inline void LCD_WriteData(uint16_t data)
{
    SPI_CS_LOW;

    SPISendRecvByte(SPI_START | SPI_WR | SPI_DATA);    /* Write : RS = 1, RW = 0       */
    SPISendRecvByte((data >>   8));                    /* Write D8..D15                */
    SPISendRecvByte((data & 0xFF));                    /* Write D0..D7                 */

    SPI_CS_HIGH;
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
    LCD_WriteReg(HORIZONTAL_GRAM_SET, Ypos);
    LCD_WriteReg(VERTICAL_GRAM_SET, Xpos);

}

inline void LCD_WriteIndex(uint16_t index)
{
    SPI_CS_LOW;

    SPISendRecvByte(SPI_START | SPI_WR | SPI_INDEX);   /* Write : RS = 0, RW = 0  */
    SPISendRecvByte(0);
    SPISendRecvByte(index);

    SPI_CS_HIGH;
}

void LCD_SetPoint(uint16_t Xpos, uint16_t Ypos, uint16_t color)
{
    /* Should check for out of bounds */
    if(Xpos > MAX_SCREEN_X | Ypos > MAX_SCREEN_Y) return;

    /* Set cursor to Xpos and Ypos */
    LCD_SetCursor(Xpos, Ypos);

    /* Write color to GRAM reg */
    LCD_WriteReg(GRAM, color);
}

void LCD_DrawLine(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend, uint16_t color){
    int dx, dy, p, x, y;
    if(Xend > MAX_SCREEN_X - 1 |Yend > MAX_SCREEN_Y - 1) return;

    dx = Xend - Xstart;
    dy = Yend - Ystart;

    x = Xstart;
    y = Ystart;

    p = (dy - dx) << 1;

    while(x < Xend){
        if(p >= 0){
            LCD_SetPoint(x, y, color);
            y++;
            p = p + (dy << 1) - (dx << 1);
        }
        else{
            LCD_SetPoint(x, y, color);
            p = p + (dy << 1);
        }
        x++;
    }
}

void LCD_DrawRectangle(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend, uint16_t color){
    if(Xend > MAX_SCREEN_X - 1 | Yend > MAX_SCREEN_Y - 1) return;

    int width = Xend - Xstart;
    int height = Yend - Ystart;

    LCD_WriteReg(ENTRY_MODE, 0x0030);
    LCD_WriteReg(HOR_ADDR_START_POS, Ystart);
    LCD_WriteReg(HOR_ADDR_END_POS, Yend);
    LCD_WriteReg(VERT_ADDR_START_POS, Xstart);
    LCD_WriteReg(VERT_ADDR_END_POS, Xend);
    LCD_SetCursor(Xstart,Ystart);
    LCD_WriteIndex(GRAM);

    SPI_CS_LOW;
    LCD_Write_Data_Start();
    for(int i = 0; i < (width * height)-1; i++){
        LCD_Write_Data_Only(color);
    }
    SPI_CS_HIGH;
}

void LCD_Clear(uint16_t color)
{
    LCD_DrawRectangle(MIN_SCREEN_X, MIN_SCREEN_Y, MAX_SCREEN_X - 1, MAX_SCREEN_Y - 1, color);
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

