
#include <msp430.h> 
#include <intrinsics.h>

volatile unsigned char RXDta;



/*
 * main.c
 */
void main( void )
{
    // Stop watchdog timer to prevent time out reset
    WDTCTL = WDTPW | WDTHOLD;
    if(CALBC1_1MHZ==0xFF || CALDCO_1MHZ==0xFF)
    {
        __bis_SR_register(LPM4_bits);
    }
    else
    {
        // Factory Set.
        BCSCTL1= CALBC1_1MHZ; //frequence d¡¯horloge 1MHz
        DCOCTL= CALDCO_1MHZ; //
    }

    //--------------- Secure mode
    P1SEL = 0x00;        // GPIO
    P1DIR = 0x00;         // IN

    // led
    P1DIR |=  BIT0;
    P1OUT &= ~BIT0;

    TACCR0 = 20000 - 1; // determine la periode du signal
    TACCR1 = 1500;      // determine le rapport cyclique du signal

    // USI Config. for SPI 3 wires Slave Op.
    // P1SEL Ref. p41,42 SLAS694J used by USIPEx
    USICTL0 |= USISWRST;
    USICTL1 = 0;

    P1DIR |= BIT2; // P2.2 en sortie
    P1SEL |= BIT2; // selection fonction TA1.1
    TACTL = TASSEL_2 | MC_1; // source SMCLK pour TimerA (no 2), mode comptage Up
    TACCTL1 |= OUTMOD_7; // activation mode de sortie n¡ã7


    // 3 wire, mode Clk&Ph / 14.2.3 p400
    // SDI-SDO-SCLK - LSB First - Output Enable - Transp. Latch
    USICTL0 |= (USIPE7 | USIPE6 | USIPE5 | USILSB | USIOE | USIGE );
    // Slave Mode SLAU144J 14.2.3.2 p400
    USICTL0 &= ~(USIMST);
    USICTL1 |= USIIE;
    USICTL1 &= ~(USICKPH | USII2C);

    USICKCTL = 0;           // No Clk Src in slave mode
    USICKCTL &= ~(USICKPL | USISWCLK);  // Polarity - Input ClkLow

    USICNT = 0;
    USICNT &= ~(USI16B | USIIFGCC ); // Only lower 8 bits used 14.2.3.3 p 401 slau144j
    USISRL = 0x23;  // hash, just mean ready; USISRL Vs USIR by ~USI16B set to 0
    USICNT = 0x08;

    // Wait for the SPI clock to be idle (low).
    while ((P1IN & BIT5)) ;

    USICTL0 &= ~USISWRST;
    __bis_SR_register(GIE); // general interrupts enable
}

// --------------------------- R O U T I N E S   D ' I N T E R R U P T I O N S

/* ************************************************************************* */
/* VECTEUR INTERRUPTION USI                                                  */
/* ************************************************************************* */
#pragma vector=USI_VECTOR
__interrupt void universal_serial_interface(void)
{
    while( !(USICTL1 & USIIFG) );   // waiting char by USI counter flag
    RXDta = USISRL;

    if (RXDta == 0x31) //if the input buffer is 0x31 (mainly to read the buffer)
    {
        P1OUT |= BIT0; //turn on LED
        TACCR1 = 950;//camera turn left
    }
    else if (RXDta == 0x32)
    {
        P1OUT &= ~BIT0; //turn off LED
        TACCR1 = 2050;//camera turn right
    }
    else if (RXDta == 0x33)
    {
        TACCR1 = 1500;//camera reposition
    }
    USISRL = RXDta;
    USICNT &= ~USI16B;  // re-load counter & ignore USISRH
    USICNT = 0x08;      // 8 bits count, that re-enable USI for next transfert
}
//------------------------------------------------------------------ End ISR

