#include <msp430.h> 
#include "ADC.h"



/* Fonction d'emission d'une chaine de caracteres
 * Entree : pointeur sur chaine de caracteres
 * Sorties:  -
 */
void envoi_msg_UART(unsigned char *msg)
{
    unsigned int i = 0;
    for(i=0 ; msg[i] != 0x00 ; i++)
    {
        while( !(USICTL1 & USIIFG));    //attente de fin du dernier envoi
        USICTL0 |= USIGE;
        USISR=msg[i];
    }
}




/**
 * main.c
 */

volatile unsigned char received_ch = 0;
ADC_Init;

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    int i =0;


    P1OUT &= ~(BIT0); // led éteinte
    P1DIR |= (BIT0); // led en output
    //P1DIR |= (BIT4); // Respect the order (output line)

    USICTL0 = USISWRST; // USCI soft. reset
    USICTL1 = 0;

    USICTL0 |= (USIPE7 | USIPE6 | USIPE5 |USIOE | USIGE); // SDI-SDO-SCLK - Output Enable - Transp. Latch
    USICTL0 &= ~(USIMST);  // SPI Slave
    USICTL0 |= USILSB; // LSB first
    USICTL1 |= USIIE;
    USICTL1 &= ~(USICKPH | USII2C);


    USICKCTL = 0; // No Clk Src in slave mode
    USICKCTL &= ~(USICKPL | USISWCLK);  // Polarity - Input ClkLow

    //informe qur tu es prêt
    USICNT = 0;
    USICNT &= ~(USI16B | USIIFGCC ); // Only lower 8 bits used 14.2.3.3 p 401 slau144j
    USISRL = 0x23;  // hash, just mean ready; USISRL Vs USIR by ~USI16B set to 0
    USICNT = 0x08;


    // Wait for the SPI clock to be idle (low).
    while (P1IN & BIT5) ;

    USICTL0 &= ~(USISWRST);


    __bis_SR_register(LPM4_bits | GIE); // general interrupts enable & Low Power Mode

   // if(received_ch =='a')
   // {

        /*TIMER*/
        P1DIR |= BIT4;
        P1SEL |= BIT4;             // selection fonction TA1.1
        TACTL = TASSEL_2 | MC_1;  // source SMCLK pour TimerA (no 2), mode comptage Up
        TACCTL1 |= OUTMOD_7; // activation mode de sortie n°7
        TACCR0 = 5000;            // determine la periode du signal
        TACCR1 = 2000;            // determine le rapport cyclique du signal













        for (i=0; i<500;i++)
        {

        }


   // }
}


// interruption

#pragma vector=USI_VECTOR
__interrupt void universal_serial_interface(void)
{
    while( !(USICTL1 & USIIFG));   // waiting char by USI counter flag
    received_ch = USISRL;

    switch (received_ch)
        {
        case 'a':
            P1OUT |= BIT0; // allume la led
            envoi_msg_UART("La lampe est allumee\n");
        break;

        case 'e':
            P1OUT &= ~(BIT0);  // éteint la led
            envoi_msg_UART("La lampe est eteinte\n");
        break;

        default :
        envoi_msg_UART("Un probleme est survenu\n");
        break;
        }

//autorise transfert suivant
    USISRL = received_ch;
    USICNT &= ~(USI16B);  // re-load counter & ignore USISRH
    USICNT = 0x08;      // 8 bits count, that re-enable USI for next transfert



}
