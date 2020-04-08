#include <msp430.h>
#include <string.h>
#include "Moove.h"

/*
 * Fichier contenant le main de la carte dite "mere", c'est a dire celle sur la samBord
 * pour le projet Bus de Communication. La carte mere gere les états du robot (manuel et 
 * automatique), la communication avec l'utilisateur, la récupération des inforatiodes capteurs 
 * et le comportement en Automatique du robot
 *
 * Auteurs : Estelle et Barbara
 */

/* Prototypes
 */
void init_BOARD( void );
void init_UART( void );
void init_USCI( void );
void interpreteur( void );
void envoi_msg_UART(unsigned char * );
void Send_char_SPI( unsigned char );

/*Definitions*/
#define RELEASE "\r\t\tSPI-rIII162018"
#define PROMPT  "\r\nmaster>"

#define CMDLEN  10

#define TRUE    1
#define FALSE   0

#define Manuel  0
#define Auto    1
#define Debug   2

#define LF      0x0A            /* \n */
#define CR      0x0D            /* \r */
#define BSPC    0x08            /* retour a la ligne */
#define DEL     0x7F            /* retour */
#define ESC     0x1B            /* espace */ 

#define _CS         BIT4        /* chip select pour la liaison SPI*/
#define SCK         BIT5        /* horloge */
#define DATA_OUT    BIT6        /* sortie donnees spi */
#define DATA_IN     BIT7        /* entree donnees spi */

#define LED_R       BIT0        /* Lampe rouge de la carte */ 
#define LED_G       BIT6        /* Lampe verte de la carte */

/*
 * Variables globales
 */

unsigned char ComSPI;
unsigned char cmd[CMDLEN];      /* tableau de caracteres lie a la commande user*/
unsigned char car = 0x30;       
unsigned int  nb_car = 0;
unsigned char intcmd = FALSE;   
char Mode = 1;
unsigned char cap_devant= 0;
unsigned char cap_sol = 0;

/* Fonction d'interpretation des commandes utilisateur */
void interpreteur( void )
{
    if( 0 == strcmp((const char *)cmd, "h"))  /* Si l'utilsateur demande l'aide */
    {
        envoi_msg_UART("\r\nCommandes :");
        envoi_msg_UART("\r\n'a' : acancer");
        envoi_msg_UART("\r\n'r' : reculer");
        envoi_msg_UART("\r\n's' : stop");
        envoi_msg_UART("\r\n'd' : tourner droite");
        envoi_msg_UART("\r\n'g' : tourner gauche\r\n");
        envoi_msg_UART("\r\n'h' : aide\r\n");
        envoi_msg_UART("\r\n'b' : debug mode\r\n");
        envoi_msg_UART("\r\n'm' : mode manuel\r\n");
        envoi_msg_UART("\r\n'o' : mode automatique\r\n");
    }

    else if ( 0 == strcmp((const char *)cmd, "m")) /* si l'utilisateur demande le mode manuel */
    {
       /* Le robot passe en manuel */
        Mode = Manuel;
    }
    else if (strcmp((const char *)cmd, "o") == 0)  /* si l'utilisateur demande le mode automatique */
    {
       /* Le robot passe en automatic */
        Mode = Auto;
    }
    else if ( 0 == strcmp((const char *)cmd, "b"))  /* si l'utilisateur demande le mode debug */
    {
        /* Le robot passe en mode debug  */
        Mode = Debug;
    }

    else if (Mode == Manuel) /* Si le robot est en mode manuel */
    {
        if ( 0 == strcmp((const char *)cmd, "a") )
        {
            /* met le robot en marche avant */
            R_avancer_continu();
        }
        else if ( 0  == strcmp((const char *)cmd, "r"))
        {
            /* met le robot en marche arriere */
            R_reculer_continu();
        }
        else if ( 0 == strcmp((const char *)cmd, "s"))
        {
            /* met le robot a l'arret */
            R_stop();
        }
        else if (0 == strcmp((const char *)cmd, "d"))
        {
            /* Le robot tourne a droite puis continue */
            R_tourner_gauche();
        }
        else if ( 0 == strcmp((const char *)cmd, "g"))
        {
            /* Le robot tourne a gauche puis continue */
            R_tourner_droite();
        }
        else
        {
            envoi_msg_UART("\r\n ?");
            envoi_msg_UART((unsigned char *)cmd);
        }
    }
    else
    {
        envoi_msg_UART("\r\n ?");
        envoi_msg_UART((unsigned char *)cmd);
    }
    envoi_msg_UART(PROMPT);
}

/* fonction d'initialisation de la carte  */
void init_BOARD( void )
{
    /* arret du watchdog */
    WDTCTL = WDTPW | WDTHOLD;

	/* reset en cas de mauvaise extinction */
    if( (CALBC1_1MHZ==0xFF) || (CALDCO_1MHZ==0xFF) )
    {
        __bis_SR_register(LPM4_bits);
    }
    else
    {
        DCOCTL = 0;
        BCSCTL1 = CALBC1_1MHZ;
        DCOCTL = (0 | CALDCO_1MHZ);
    }

    /* Reset de toutes les pin par securité */
    P1SEL  = 0x00;        
    P1SEL2 = 0x00;        
    P2SEL  = 0x00;       
    P2SEL2 = 0x00;        
    P1DIR = 0x00;    
    P2DIR = 0x00;   

    P1SEL  &= ~LED_R;
    P1SEL2 &= ~LED_R;
    P1DIR |= LED_R ;  
    P1OUT &= ~LED_R ;
}

/* Fonction d'initialisation de l'UART */
void init_UART( void )
{
    P1SEL  |= (BIT1 | BIT2);                    // P1.1 = RXD, P1.2=TXD
    P1SEL2 |= (BIT1 | BIT2);                    // P1.1 = RXD, P1.2=TXD
    UCA0CTL1 |= UCSWRST;                        // SOFTWARE RESET
    UCA0CTL1 |= UCSSEL_2;                       // SMCLK (2 - 3)
    UCA0BR0 = 104;                             // 104 1MHz, OSC16, 9600 (8Mhz : 52) : 8 115k - 226/12Mhz
    UCA0BR1 = 0;                                // 1MHz, OSC16, 9600 - 4/12Mhz
    UCA0MCTL = 10;
    UCA0CTL0 &= ~(UCPEN  | UCMSB | UCDORM);
    UCA0CTL0 &= ~(UC7BIT | UCSPB  | UCMODE_3 | UCSYNC); // dta:8 stop:1 usci_mode3uartmode
    UCA0CTL1 &= ~UCSWRST;                       // **Initialize USCI state machine**
    /* Enable USCI_A0 RX interrupt */
    IE2 |= UCA0RXIE;
}

/* ----------------------------------------------------------------------------
 * Fonction d'initialisation de l'USCI POUR SPI SUR UCB0
 */
void init_USCI( void )
{
    // Waste Time, waiting Slave SYNC
    __delay_cycles(250);

    // SOFTWARE RESET - mode configuration
    UCB0CTL0 = 0;
    UCB0CTL1 = (0 + UCSWRST*1 );

    // clearing IFg /16.4.9/p447/SLAU144j
    // set by setting UCSWRST just before
    IFG2 &= ~(UCB0TXIFG | UCB0RXIFG);

    // Configuration SPI (voir slau144 p.445)
    // UCCKPH = 0 -> Data changed on leading clock edges and sampled on trailing edges.
    // UCCKPL = 0 -> Clock inactive state is low.
    //   SPI Mode 0 :  UCCKPH * 1 | UCCKPL * 0
    //   SPI Mode 1 :  UCCKPH * 0 | UCCKPL * 0  <--
    //   SPI Mode 2 :  UCCKPH * 1 | UCCKPL * 1
    //   SPI Mode 3 :  UCCKPH * 0 | UCCKPL * 1
    // UCMSB  = 1 -> MSB premier
    // UC7BIT = 0 -> 8 bits, 1 -> 7 bits
    // UCMST  = 0 -> CLK by Master, 1 -> CLK by USCI bit CLK / p441/16.3.6
    // UCMODE_x  x=0 -> 3-pin SPI,
    //           x=1 -> 4-pin SPI UC0STE active high,
    //           x=2 -> 4-pin SPI UC0STE active low,
    //           x=3 -> i²c.
    // UCSYNC = 1 -> Mode synchrone (SPI)
    UCB0CTL0 |= ( UCMST | UCMODE_0 | UCSYNC );
    UCB0CTL0 &= ~( UCCKPH | UCCKPL | UCMSB | UC7BIT );
    UCB0CTL1 |= UCSSEL_2;

    UCB0BR0 = 0x0A;     // divide SMCLK by 10
    UCB0BR1 = 0x00;

    // SPI : Fonctions secondaires
    // MISO-1.6 MOSI-1.7 et CLK-1.5
    // Ref. SLAS735G p48,49
    P1SEL  |= ( SCK | DATA_OUT | DATA_IN);
    P1SEL2 |= ( SCK | DATA_OUT | DATA_IN);
    UCB0CTL1 &= ~UCSWRST;                                // activation USCI
}

/* ----------------------------------------------------------------------------
 * Fonction d'emission d'une chaine de caracteres
 * Entree : pointeur sur chaine de caracteres
 * Sorties:  -
 */
void envoi_msg_UART(unsigned char *msg)
{
    unsigned int i = 0;
    for(i=0 ; msg[i] != 0x00 ; i++)
    {
        while(!(IFG2 & UCA0TXIFG));    //attente de fin du dernier envoi (UCA0TXIFG à 1 quand UCA0TXBUF vide)
        UCA0TXBUF=msg[i];
    }
}

/* Fonction d'envoie d'un caractère sur USCI en SPI 3 fils MASTER Mode
 */
void Send_char_SPI(unsigned char carac)
{
    while ((UCB0STAT & UCBUSY));   // attend que USCI_SPI soit dispo.
    while(!(IFG2 & UCB0TXIFG)); // p442
    UCB0TXBUF = carac;              // Put character in transmit buffer
}

/* main.c
 */
void main( void )
{
	/* initialisation */
    init_BOARD();
    init_UART();
    init_USCI();
    Moove_Init();

    envoi_msg_UART("\rReady !\r\n"); /* message de debut */
    envoi_msg_UART(PROMPT);

 while(1)
    {
        if( intcmd ) /* si l'utilisateur envoie une commande, on la traite */
        {
            while ((UCB0STAT & UCBUSY));   /* attend que USCI_SPI soit dispo. */
            interpreteur();                /* execute la commande utilisateur */
            intcmd = FALSE;                /* acquitte la commande en cours */
        }
        else /* Si il n'y a pas de commande a traiter, le robot reagis selon son mode */
        {
            switch (Mode)
            {
            case Auto:
                /* Demande la donnee du capteur devant */
                Send_char_SPI('d');
                cap_devant = ComSPI;

                /* Demande la donnee du capteur sol */
                Send_char_SPI('s');
                cap_sol = ComSPI;

                /* Si il y a un trou, reculer un peu et faire demi-tour */
                if (cap_sol < 150)
                {
                    R_reculer;
                    R_tourner_gauche();
                    R_tourner_gauche();
                }
                /* Sinon si il y a un mur devant, on tourne a gauche */
                else if (cap_devant > 150 )
                {
                    R_tourner_gauche();
                }
                /* Sinon on avance un peu */
                else
                {
                    R_avancer;
                }

            break;
            case Manuel:
                /* rien a faire, le robot est diriger par les commendes utilisateur (cf interpreteur) */
            break;
            case Debug:
                /* afficher les valeur envoyer par les capteurs */
                Send_char_SPI('d');
                envoi_msg_UART(ComSPI);

                Send_char_SPI('s');
                envoi_msg_UART(ComSPI);
            break;
            }
        }
    }
}

/* Interuption */
#pragma vector = USCIAB0RX_VECTOR
__interrupt void USCIAB0RX_ISR()
{
    /* Interuption Uart */
    if (IFG2 & UCA0RXIFG)
    {
        while(!(IFG2 & UCA0RXIFG));
        cmd[nb_car]=UCA0RXBUF;         /* lecture caractere recu */

        while(!(IFG2 & UCA0TXIFG));    /* attente de fin du dernier envoi (UCA0TXIFG à 1 quand UCA0TXBUF vide) */
        UCA0TXBUF = cmd[nb_car];

        if( cmd[nb_car] == ESC)
        {
            nb_car = 0;
            cmd[1] = 0x00;
            cmd[0] = CR;
        }

        if( (cmd[nb_car] == CR) || (cmd[nb_car] == LF))
        {
            cmd[nb_car] = 0x00;
            intcmd = TRUE;
            nb_car = 0;
            __bic_SR_register_on_exit(LPM4_bits);
        }
        else if( (nb_car < CMDLEN) && !((cmd[nb_car] == BSPC) || (cmd[nb_car] == DEL)) )
        {
            nb_car++;
        }
        else
        {
            cmd[nb_car] = 0x00;
            nb_car--;
        }
    }

    /*  Interuptoin SPI */
    else if (IFG2 & UCB0RXIFG)
    {
        while( (UCB0STAT & UCBUSY) && !(UCB0STAT & UCOE) );
        while(!(IFG2 & UCB0RXIFG));
        ComSPI= UCB0RXBUF;
    }
}
