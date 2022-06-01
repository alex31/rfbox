/*
  Nom(s), prénom(s) du ou des élèves : 

  QUESTION 1 : influence de la suppression du tableau prendDeLaPlaceSurLaPile ?

 */
#include <ch.h>
#include <hal.h>
#include "stdutil.h"		// necessaire pour initHeap
#include "ttyConsole.hpp"		// fichier d'entête du shell


/*
  Câbler une LED sur la broche C0


  ° connecter B6 (uart1_tx) sur PROBE+SERIAL Rx AVEC UN JUMPER
  ° connecter B7 (uart1_rx) sur PROBE+SERIAL Tx AVEC UN JUMPER
  ° connecter C0 sur led0

 */


static THD_WORKING_AREA(waBlinker, 304);	// declaration de la pile du thread blinker
static void  /*noreturn*/ blinker (void *arg)			// fonction d'entrée du thread blinker
{
  (void)arg;					// on dit au compilateur que "arg" n'est pas utilisé
  chRegSetThreadName("blinker");		// on nomme le thread
  
  while (true) {				// boucle infinie
    palToggleLine(LINE_LED_GREEN);		// clignotement de la led 
    chThdSleepMilliseconds(100);		// à la féquence de 1 hertz
  }
}



int main (void)
{

  halInit();
  chSysInit();
  initHeap();		// initialisation du "tas" pour permettre l'allocation mémoire dynamique 

  consoleInit();	// initialisation des objets liés au shell
  chThdCreateStatic(waBlinker, sizeof(waBlinker), NORMALPRIO, &blinker, NULL); // lancement du thread 

  // cette fonction en interne fait une boucle infinie, elle ne sort jamais
  // donc tout code situé après ne sera jamais exécuté.
  consoleLaunch();  // lancement du shell
  
  // main thread does nothing
  chThdSleep(TIME_INFINITE);
}


