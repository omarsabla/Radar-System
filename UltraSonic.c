
#include <xc.h>
#define _XTAL_FREQ 4000000UL   // 4 MHz crystal

// CONFIG
#pragma config FOSC = HS
#pragma config WDTE = OFF
#pragma config PWRTE = ON
#pragma config BOREN = ON
#pragma config LVP = OFF
#pragma config CPD = OFF
#pragma config WRT = OFF
#pragma config CP = OFF

// Pin mapping
#define TRIG      RB1
#define ECHO      RB2
#define OUT_PULSE RB0

// Detection settings
#define DETECT_CM 20           // threshold distance in cm
#define OUTPUT_PULSE_MS 100    // output pulse length when object detected (ms)

// Timeouts (microseconds)
#define WAIT_ECHO_START_US 30000UL   // max wait for echo to start (30 ms)
#define MAX_ECHO_US         30000UL  // max echo width to accept   (30 ms)

// Timer1 helper for 4MHz: Timer1 tick = Fosc/4 = 1 MHz -> 1 microsecond per tick
// So ticks == microseconds (for 4 MHz and prescaler 1:1)

void init_hw(void)
{
    // Make analog pins digital and disable comparators (PIC16F877A)
    ADCON1 = 0x07;   // All digital
    CMCON  = 0x07;   // Comparators off

    // Disable weak pull-ups and RB-Change interrupts to keep PORTB normal
    OPTION_REGbits.nRBPU = 1; // Disable PORTB internal pull-ups
    INTCON = 0;               // Clear interrupts

    // Configure I/O directions
    TRISB0 = 0;   // OUT_PULSE (RB0) as output
    TRISB1 = 0;   // TRIG       (RB1) as output
    TRISB2 = 1;   // ECHO       (RB2) as input

    // Initial states
    OUT_PULSE = 0;
    TRIG = 0;

    // Timer1 configuration: Fosc/4, prescaler 1:1, Timer1 off for now
    T1CON = 0x00;
}

unsigned int measure_echo_us_timer1(void)
{
    unsigned long timeout;
    unsigned int t_start, t_end;
    unsigned int ticks;

    // Ensure TRIG low for at least 2 µs
    TRIG = 0;
    __delay_us(2);

    // Clear and start Timer1
    TMR1H = 0;
    TMR1L = 0;
    T1CONbits.TMR1ON = 1; // start Timer1 (tick = 1 us at 4 MHz, prescaler 1:1)

    // Send 10 us trigger pulse
    TRIG = 1;
    __delay_us(10);
    TRIG = 0;

    // Wait for echo to go high (with software timeout)
    timeout = WAIT_ECHO_START_US;
    while(!ECHO) {
        if(--timeout == 0) {
            T1CONbits.TMR1ON = 0;
            return 0xFFFF; // timeout - no echo start
        }
        __delay_us(1);
    }

    // echo went high -> capture start time (Timer1)
    t_start = ((unsigned int)TMR1H << 8) | TMR1L;

    // Wait for echo to go low or timeout
    timeout = MAX_ECHO_US;
    while(ECHO) {
        if(--timeout == 0) {
            T1CONbits.TMR1ON = 0;
            return 0xFFFF; // timeout - echo stuck/too long
        }
        __delay_us(1);
    }

    // capture end time and stop Timer1
    t_end = ((unsigned int)TMR1H << 8) | TMR1L;
    T1CONbits.TMR1ON = 0;

    // compute ticks (handle wrap-around if needed; but our timeouts keep it < 0xFFFF)
    if(t_end >= t_start) ticks = t_end - t_start;
    else ticks = (0xFFFF - t_start) + t_end + 1;

    return ticks; // ticks == echo pulse width in microseconds
}

unsigned int get_distance_cm_from_us(unsigned int us)
{
    // HC-SR04: distance_cm = time_us / 58  (approx)
    // return 0xFFFF unchanged for timeout sentinel
    if(us == 0xFFFF) return 0xFFFF;
    return (unsigned int)(us / 58UL);
}

void pulse_output_ms(unsigned int ms)
{
    OUT_PULSE = 1;
    while(ms--) __delay_ms(1);
    OUT_PULSE = 0;
}

void indicate_no_echo_error(void)
{
    // quick blink pattern to show no-echo condition
    OUT_PULSE = 1;
    __delay_ms(80);
    OUT_PULSE = 0;
    __delay_ms(80);
}

void main(void)
{
    unsigned int us, distance_cm;

    init_hw();

    while(1)
    {
        us = measure_echo_us_timer1();

        if(us == 0xFFFF) {
            // No echo detected or timeout
            indicate_no_echo_error();
        } else {
            distance_cm = get_distance_cm_from_us(us);
            if(distance_cm < DETECT_CM) {
                // object detected -> send output pulse
                pulse_output_ms(OUTPUT_PULSE_MS);
            }
            // small delay between measurements
            __delay_ms(60);
        }
    }
}
