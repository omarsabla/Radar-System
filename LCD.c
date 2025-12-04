#include <xc.h>

// CONFIG BITS
#pragma config FOSC = HS
#pragma config WDTE = OFF
#pragma config PWRTE = OFF
#pragma config BOREN = OFF
#pragma config LVP = OFF
#pragma config CPD = OFF
#pragma config WRT = OFF
#pragma config CP = OFF

#define _XTAL_FREQ 4000000   // 4Hz crystal

// LCD Control Pins
#define RS  RB2
#define EN  RB3

// LCD Data Pins
#define D4  RB4
#define D5  RB5
#define D6  RB6
#define D7  RB7

#define BACKLIGHT RB1

// Function Prototypes
void LCD_Command(unsigned char cmd);
void LCD_Char(unsigned char data);
void LCD_Init(void);
void LCD_String(const char *str);

// Send Command
void LCD_Command(unsigned char cmd)
{
    RS = 0;

    D4 = (cmd >> 4) & 1;
    D5 = (cmd >> 5) & 1;
    D6 = (cmd >> 6) & 1;
    D7 = (cmd >> 7) & 1;

    EN = 1;
    __delay_ms(2);
    EN = 0;

    D4 = cmd & 1;
    D5 = (cmd >> 1) & 1;
    D6 = (cmd >> 2) & 1;
    D7 = (cmd >> 3) & 1;

    EN = 1;
    __delay_ms(2);
    EN = 0;
}

// Send Data (Character)
void LCD_Char(unsigned char data)
{
    RS = 1;

    D4 = (data >> 4) & 1;
    D5 = (data >> 5) & 1;
    D6 = (data >> 6) & 1;
    D7 = (data >> 7) & 1;

    EN = 1;
    __delay_ms(2);
    EN = 0;

    D4 = data & 1;
    D5 = (data >> 1) & 1;
    D6 = (data >> 2) & 1;
    D7 = (data >> 3) & 1;

    EN = 1;
    __delay_ms(2);
    EN = 0;
}

// Print String
void LCD_String(const char *str)
{
    while (*str)
    {
        LCD_Char(*str++);
    }
}

// LCD Initialization
void LCD_Init(void)
{
    __delay_ms(20);

    LCD_Command(0x02);  // 4-bit mode
    LCD_Command(0x28);  // 2 lines, 5x7 matrix
    LCD_Command(0x0C);  // Display ON, Cursor OFF
    LCD_Command(0x06);  // Auto increment
    LCD_Command(0x01);  // Clear display
    __delay_ms(2);
}

// MAIN PROGRAM
void main(void)
{
    TRISB = 0x00;   // All PORTB as OUTPUT
    PORTB = 0x00;

    BACKLIGHT = 1; // Turn ON backlight

    LCD_Init();
    LCD_String("Hello World");

    while(1); // Infinite loop
}
