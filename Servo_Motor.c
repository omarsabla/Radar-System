#include <xc.h>

// CONFIG
#pragma config FOSC = HS
#pragma config WDTE = OFF
#pragma config PWRTE = OFF
#pragma config BOREN = ON
#pragma config LVP = OFF
#pragma config CPD = OFF
#pragma config WRT = OFF
#pragma config CP = OFF

// global variables
volatile unsigned int pulse_us = 1500;   // current pulse width (us)
volatile unsigned char pulseState = 0;
volatile unsigned char updateFlag = 0;

// ---- Timer1 preload function ----
// At 4 MHz clock: 1 instruction = 1 µs for Timer1
void loadTimer1_us(unsigned int us)
{
    unsigned int preload = 65536 - us;
    TMR1H = preload >> 8;
    TMR1L = preload & 0xFF;
}

void __interrupt() isr(void)
{
    if (TMR1IF) {
        TMR1IF = 0;

        if (pulseState == 0) {
            RC2 = 1;               // HIGH pulse begins
            pulseState = 1;
            loadTimer1_us(pulse_us);
        }
        else {
            RC2 = 0;               // LOW part
            pulseState = 0;
            loadTimer1_us(20000 - pulse_us); // 20ms frame
            updateFlag = 1;        // signal main loop to adjust sweep
        }
    }
}

void main(void)
{
    TRISC2 = 0;
    RC2 = 0;

    // Timer1 setup
    T1CKPS0 = 0;   // Prescaler 1:1
    T1CKPS1 = 0;
    TMR1CS = 0;    // Fosc/4
    TMR1ON = 1;

    loadTimer1_us(20000 - pulse_us);

    TMR1IF = 0;
    TMR1IE = 1;
    PEIE = 1;
    GIE = 1;

    // sweeping parameters
    int step = 5;                      // smoother = smaller
    int minPulse = 500;               // SG90 full left
    int maxPulse = 2500;              // SG90 full right
    int pauseCounter = 0;

    while (1)
    {
        if (updateFlag) {
            updateFlag = 0;

            // pause briefly at sweep ends
            if (pulse_us <= minPulse || pulse_us >= maxPulse) {
                if (++pauseCounter < 25)  // 25 cycles ≈ 0.5 sec
                    continue;
                pauseCounter = 0;
                step = -step;  // reverse direction
            }

            // update pulse width for smooth sweep
            pulse_us += step;

            // clamp (safety for SG90)
            if (pulse_us < minPulse) pulse_us = minPulse;
            if (pulse_us > maxPulse) pulse_us = maxPulse;
        }
    }
}

