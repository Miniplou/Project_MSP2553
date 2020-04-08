#include <msp430.h>
#include <stdlib.h>
#include "ADC.h"

/*
 * Fichier contenant le main de la carte dite "capteur", c'est a dire celle ajouter a la samBord
 * pour le projet Bus de Communication. La carte capteur gere la récupération des donner des capteurs et
 * et les envoie a la carte mere.
 *
 * Auteurs : Estelle et Barbara
 */

/* Definitions */
#define C_US_Triger BIT2  /* Port assosie au pulse du capteur ultrason */
#define C_US_Echo   BIT1  /* Port assosie au retour du capteur ultrason */
#define C_IF        BIT0  /* Port assosie au retour du cateur infra-rouge */
#define CLK         BIT5  /* Port assosie a l'horloge recu par le maitre pour la liaison SPI */
#define MISO        BIT6  /* Port assosie a  */
#define MOSI        BIT7  /* Port assosie a  */

/* Variable global */
unsigned char Devant = 0;  /* contient un nombre représentant la distance entre le robot et un potentiel obstacle devant (plus le chiffre est grand, plus l'obstacle est loin) */
unsigned char Sol = 0;     /* contient un nombre représentant la distance entre le robot et le sol (plus le chiffre est petit, plus le sol est loin) */
int miliseconds = 0;       /* compteur de milliseconde utile au calcule de distance pour le capteur Ultason */
long Result_US = 0;		   /* contient la valeur du retour   */

/* Prototypes  */
extern void init_ultrason(void);  /* Fonction d'initialisation pour le capteur ultrason */
extern int get_distance(void);   /* Fonction de calcule de distance pour le capteur ultrason */

/* Fonction de calcule de distance pour le capteur ultrason */
int get_distance(void) 
{
    P1DIR |= C_US_Triger;          /* Mettre la pine du pulse en sortie */ 
    P1OUT |= C_US_Triger;          /* genéré le pulse */ 
    __delay_cycles(10);            /* attendre 10us */ 
    P1OUT &= ~C_US_Triger;         /* arreter le pulse */
    P1DIR &= ~C_US_Echo;           /* Mettre la pine de l'echo en entree */ 
    P1IFG = 0x00;                  /* Reset le flag (par precaution) */ 
    P1IE |= C_US_Echo;             /* Autorise les interuption sur la pine de l'echo */
    P1IES &= ~C_US_Echo;           /* interuption sur front montant */ 
    __delay_cycles(30000);         /* attendre 30ms (temps max d'attente de retour, après on estime qu'il n'y a aucun obstacle) */
	return Result_US ;			   /* Retourne le resultat (mis a jours avec les interuptions) */
}

/* fonction d'initialisation de l'ultrason */
void init_ultrason(void){
    BCSCTL1 = CALBC1_1MHZ;					/* frequence d’horloge 1MHz */
    DCOCTL = CALDCO_1MHZ;                   /* frequence d’horloge 1MHz */ 
    CCTL0 = CCIE;                           /* autoriser les interuption pour l'horloge */ 
    CCR0 = 1000;                            
    TACTL = TASSEL_2 + MC_1;                /* orloge comptant les état hauts */ 
}

/* fonction principale */
void main(void)
{
    __enable_interrupt();
	P1IFG  = 0x00; /* Reset tout les flags */    
    WDTCTL = WDTPW + WDTHOLD; /* Arreter le watchdog. */

    TA1CTL = TASSEL_2 | MC_1|TAIE; /* configure l'horloge pour le TimerA (en mode compte les etat haut)*/
    TA1CTL &=~ TAIFG;
    TA1CCTL1 |= OUTMOD_7;         /* activation mode de sortie n°7*/
    TA1CCTL2 |= OUTMOD_7;

    
    while ((P1IN & CLK)); /* Attendre que l'horloge SPICLK soit au repos (niveau bas) */

    /* Fonctions secondaires pour les pines MISO, MOSI et CLK */
    P1SEL  = MISO + MOSI + CLK;
    P1SEL2 = MISO + MOSI + CLK;
    
    UCA0CTL1 |= UCSWRST; /* Réinitialiser et placer le contrôleur SPI en mode configuration.*/
    UCA0CTL0 = UCCKPH*0 | UCCKPL*0 | UCMSB*1 | UC7BIT*0 | UCMST*0 | UCMODE_0 | UCSYNC*1; /*  Configuration SPI */

    /* Valider les interruptions de réception SPI */
    IE2 &= 0xF0;
    IE2 |= UCA0RXIE;
   
    __bis_SR_register(GIE);  /* Passer en mode interruptions */

	/* initialisation de la pine pour le capteur IF */
    /* mode entree sortie */
    P1SEL &= ~(C_IF);
    P1SEL2 &= ~(C_IF);
    /* dirrection de la pine : entree */
    P1DIR &= ~(C_IF);

	/* appelle des fonctions d'initialisation */
    init_ultrason();
    ADC_init();

	/* une fois les initialisations faites, les capteurs sont mis a jour en en boucle. Les envois sont geres pas les interruptions */
    while (1) 
    {
    ADC_Demarrer_conversion(0);     /* demare la convertion */
    Sol = (ADC_Lire_resultat())/4;  /* recupère le resultat du capteur IF (divise par 4 pour entree dans un unsigned char) */
    Devant = get_distance();		/* recupère le resultat du capteur US */
    }
}


/* fonction d'interruption de réception SPI */
#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR (void)
{
    unsigned char demande = '0'; 
	unsigned char data_to_send;
	
    if ((IFG2 & UCA0RXIFG) != 0)     /* Si il y a une interuption sur le port de l'echo de l'ultrason */
    {
        demande = UCA0RXBUF;         /* recuperation du char donner en consigne*/

        if (demande == 's')          /* Si la demande est la donnee du capteur du sol */
        {
            data_to_send = Sol;      /* on stock la donnee du sol dans le la donner a envoyer */
        }
        else if (demande == 'd')     /* Si la demande est la donnee du capteur du devant */
        {
            data_to_send = Devant;   /* on stock la donnee du devant dans le la donner a envoyer */   
        }
        else
        {
            data_to_send = Devant;   /* Si la demande n'a pas ete comprise, on met le devant par defaut */
        }
		
        if ((UCA0STAT & UCBUSY) ==0) /* lorsque la communicatione est prete */
        {
            UCA0TXBUF = ((data_to_send) & 0xFF);  /* on charche la donnee a envoyer dans le buffer */
        }
		else
        {
            /* nothing to do */
        }
    }
	else
    {
        /* nothing to do */
    }
}

/* fonction d'interruption pour l'echo du capteur ultrason */
#pragma vector = PORT1_VECTOR
__interrupt void Port_1()
{
    if(P1IFG&C_US_Echo)          /* Si il y a une interuption sur le port de l'echo de l'ultrason */
	{
		if(!(P1IES&C_US_Echo))   /* si c'est un front montant */
		{
            TACTL|=TACLR;        /* reset du timer */
            miliseconds = 0;	 /* reset du compteur */
            P1IES |= C_US_Echo;  /* mettre en front decendant */
        }
		else 				         /* si ce n'est pas un front montant */
		{
			Result_US = ((long)miliseconds*1000 + (long)TAR)/4; /* calcule de la distance grace au temps mis par l'echo a revenir */
									 /* (divise par 4 pour que le resultat entre dans un usigned char ) */
		}
        P1IFG &= ~C_US_Echo;     /* reset du flag */
    }
	else
    {
        /* nothing to do */
    }
}

/* fonction d'interruption pour incrémenter le compteur du capteur ultrason */
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A (void)
{
	miliseconds++;  /* on incremante miliseconde pour compter le temps qui s'est ecouler depuis l'envoie du signal */
}

