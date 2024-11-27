#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h> 

#include "../UTILS/myassert.h"
#include "../UTILS/io.h"
#include "../CLIENT_SERVICE/client_service.h"
#include "client_sigma.h"


/*----------------------------------------------*
 * usage pour le client sigma
 *----------------------------------------------*/

static void usage(const char *exeName, const char *numService, const char *message)
{
    fprintf(stderr, "Client sigma de float\n");
    fprintf(stderr, "usage : %s %s <nbThreads> <f1> <f2> .. <fn>\n", exeName, numService);
    fprintf(stderr, "        %s           : numéro du service\n", numService);
    fprintf(stderr, "        <nbThreads>   : nombre de threads\n");
    fprintf(stderr, "        <f1> ... <fn> : les nombres à tester (au moins un)\n");
    fprintf(stderr, "exemple d'appel :\n");
    fprintf(stderr, "    %s %s 2 6.3 5.8 -2.33 0.0 8.34 12.98\n", exeName, numService);
    fprintf(stderr, "    -> 6 nombres à tester avec 2 threads\n");
    if (message != NULL)
        fprintf(stderr, "message : %s\n", message);
    exit(EXIT_FAILURE);
}

/*----------------------------------------------*
 * fonction de vérification des paramètres
 *----------------------------------------------*/

void client_sigma_verifArgs(int argc, char * argv[])
{
    if (argc < 4)
        usage(argv[0], argv[1], "nombre d'arguments");
    // éventuellement d'autres tests
    else if(io_strToInt(argv[2]) <= 0)
    {
        usage(argv[0], argv[1], "Nombre de threads invalide");
    }
}


/*----------------------------------------------*
 * fonctions de communication avec le service
 *----------------------------------------------*/

// ---------------------------------------------
// fonction d'envoi des données du client au service
// Les paramètres sont
// - le file descriptor du tube de communication vers le service
// - le nombre de threads que doit utiliser le service
// - le tableau de float dont on veut la somme
static void sendData(const int pts, const int num_thr, const float to_sum[], const int tab_size)
{
    // envoi du nombre de threads
    int ret = write(pts, &num_thr, sizeof(int));

    myassert(ret != -1, "Erreur : Echec de l'écriture dans le tube");
    myassert(ret == sizeof(int), "Erreur : Données mal écrites");

    //Envoi de la taille du tableau de float
    ret = write(pts, &tab_size, sizeof(int));

    myassert(ret != -1, "Erreur : Echec de l'écriture dans le tube");
    myassert(ret == sizeof(int), "Erreur : Données mal écrites");

    //Envoi du tableau de float
    for(int i = 0; i < tab_size; i++)
    {
        ret = write(pts, &(to_sum[i]), sizeof(float));

        myassert(ret != -1, "Erreur : Echec de l'écriture dans le tube");
        myassert(ret == sizeof(float), "Erreur : Données mal écrites");
    }

}

// ---------------------------------------------
// fonction de réception des résultats en provenance du service et affichage
// Les paramètres sont
// - le file descriptor du tube de communication en provenance du service
// - autre chose si nécessaire
static void receiveResult(int pfs/* autres paramètres si nécessaire */)
{
    // récupération du résultat
    float res; 

    int ret = read(pfs, &res, sizeof(float));

    myassert(ret != -1, "Erreur : Echec de la lecture dans le tube");
    myassert(ret == sizeof(float), "Erreur : Données mal lues");

    // affichage du résultat
    printf("Le résultat est : %f\n", res);
}


// ---------------------------------------------
// Fonction appelée par le main pour gérer la communications avec le service
// Les paramètres sont
// - les deux file descriptors des tubes nommés avec le service
// - argc et argv fournis en ligne de commande
// Cette fonction analyse argv et en déduit les données à envoyer
//    - argv[2] : nombre de threads
//    - argv[3] à argv[argc-1]: les nombres flottants
void client_sigma(int pts, int pfs, int argc, char * argv[])
{
    //Pour ne pas avoir de warning sur l'inutilisation de argc
    myassert(argc >= 5, "Nombre de paramètres invalide");

    // variables locales éventuelles
    int tab_size = argc - 3; //car trois premières cases de argv sont d'autres arguments

    //Création du tableau de float
    float to_sum[tab_size];
    for(int i = 0; i < tab_size; i++)
    {
        to_sum[i] =io_strToFloat(argv[i + 3]);
    }

    sendData(pts, io_strToInt(argv[2]), to_sum, tab_size);
    receiveResult(pfs);
}

