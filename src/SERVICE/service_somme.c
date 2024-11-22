#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "../UTILS/myassert.h" 
#include "../ORCHESTRE_SERVICE/orchestre_service.h"
#include "../CLIENT_SERVICE/client_service.h"
#include "service_somme.h"

// définition éventuelle de types pour stocker les données


/*----------------------------------------------*
 * fonctions appelables par le service
 *----------------------------------------------*/

// fonction de réception des données
static void receiveData(int pfc, float * a, float * b)
{

    int ret = read(pfc, a, sizeof(float));
    myassert(ret != -1 ,"Erreur : Echec de la lecture dans le tube.");
    myassert(ret == sizeof(float) ,"Erreur : Données mal lues");

    ret = read(pfc, b, sizeof(float));
    myassert(ret != -1 ,"Erreur : Echec de la lecture dans le tube.");
    myassert(ret == sizeof(float) ,"Erreur : Données mal lues");

}

// fonction de traitement des données
static void computeResult(const float a, const float b, float * result)
{

    *result = a + b;

}

// fonction d'envoi du résultat
static void sendResult(int ptc, const float result)
{

    int ret = write(ptc, &result, sizeof(float));
    myassert(ret != -1 ,"Erreur : Echec de l'écriture dans le tube.");
    myassert(ret == sizeof(float) ,"Erreur : Données mal écrites");


}


/*----------------------------------------------*
 * fonction appelable par le main
 *----------------------------------------------*/
void service_somme(int ptc, int pfc)
{
    // initialisations diverses
    printf("Je suis service somme\n");
    float a,b,result;

    receiveData(pfc, &a, &b);
    computeResult(a, b, &result);
    sendResult(ptc ,result);

    // libération éventuelle de ressources


}
