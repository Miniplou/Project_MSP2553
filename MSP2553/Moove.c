#include "msp430.h"

#define mot1 BIT5         /* Moteur 1 */
#define mot2 BIT1         /* Moteur 2 */
#define mots (BIT1|BIT5)  /* Moteurs 1 et 2 */

int sauv_dir;    /* sauvegarde de la dirrection des roues */
int sauv_vit;    /* sauvegarde de la vitesse des roues */

/* fonction d'initialisation des roues */
void Moove_Init() 
{
    TA1CCR0 = 2000;     /* longueur cycle */
    P2SEL |= mot2|mot1; /* configuration des roues en sortie */
}

void R_stop()
{
    /* raport cyclique quasi nul pour arreter les roues (on ne peux pas mettre 0 sans disfonctionnement) */
    TA1CCR1 = 10;
    TA1CCR2 = 10;
}

void R_avancer_continu()
{
    /* directiondes moteur en avant */
    P2DIR |= mot1;
    P2DIR &=~ mot2;

    /* rapport cyclique moyen pour garder une vitesse resonable */
    TA1CCR1 = 450;
    TA1CCR2 = 450;
}

void R_reculer_continu()
{
    /* direction des moteur en arrière */
    P2DIR &=~ mot1;
    P2DIR |= mot2;

    /* rapport cyclique moyen pour garder une vitesse resonable */
    TA1CCR1 = 450;
    TA1CCR2 = 450;
}

void R_tourner_gauche()
{
    volatile unsigned int i;

    /* recuperation des drection et vitesse actuelle */
    sauv_dir = (P2DIR & mots);
    sauv_vit = TA1CCR1;

    /* arret des moteurs pour tourner proprement */
    TA1CCR1 = 10;
    TA1CCR2 = 10;
    for(i=10000;i>0;i--);

    /* Direction des moteur pour tourner a gauche */
    P2DIR |= mot2|mot1;

    /* vitesse douce pour tourner doucement */
    TA1CCR1 = 300;
    TA1CCR2 = 300;
    for(i=53200;i>0;i--);

    /* arret des moteurs pour tourner proprement */
    TA1CCR1 = 10;
    TA1CCR2 = 10;
    for(i=10000;i>0;i--);

    /* remetre l'état de départ */
    P2DIR &= ~(mots);   /* forcer les deux BIT des moteurs a zero*/
    P2DIR |= sauv_dir;  /* ajouter leur etat d'origine */
    TA1CCR1 = sauv_vit;
    TA1CCR2 = sauv_vit;
}

void R_tourner_droite()
{

    volatile unsigned int i, j;

    /* recuperation des drection et vitesse actuelle */
    sauv_dir = (P2DIR & mots);
    sauv_vit = TA1CCR1;

    /* arret des moteurs pour tourner proprement */
    TA1CCR1 = 10;
    TA1CCR2 = 10;
    for(i=10000;i>0;i--);

    /* Direction des moteur pour tourner a droite */
    P2DIR &=~ (mots);

    /* vitesse douce pour tourner doucement */
    TA1CCR1 = 300;
    TA1CCR2 = 300;
    for(j=53200;j>0;j--);

    /* arret des moteurs pour tourner proprement */
    TA1CCR1 = 10;
    TA1CCR2 = 10;
    for(i=10000;i>0;i--);

    /* remetre l'etat de depart */
    P2DIR &= ~(mots);   /* forcer les deux BIT des moteurs a zero*/
    P2DIR |= sauv_dir;  /* ajouter leur etat d'origine */
    TA1CCR1 = sauv_vit;
    TA1CCR2 = sauv_vit;
}

/* fonction avencer sur quelques centimetres */
void R_avancer()
{

    P2DIR |= mot1;
    P2DIR &=~ mot2;

    volatile unsigned int i = 62000;

    TA1CCR1 = 450;
    TA1CCR2 = 450;
    for(i=43200;i>0;i--);

    TA1CCR1 = 10;
    TA1CCR2 = 10;
}

/* fonction reculer sur queques centimetre */
void R_reculer()
{
    P2DIR &=~ mot1;
    P2DIR |= mot2;

    volatile unsigned int i;
    TA1CCR1 = 450;
    TA1CCR2 = 450;

    for(i=60000;i>0;i--);

    TA1CCR1 = 10;
    TA1CCR2 = 10;
}

