#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "../UTILS/myassert.h"
#include "../UTILS/memory.h"
#include "../CLIENT_SERVICE/client_service.h"
#include "client_compression.h"



/*----------------------------------------------*
 * usage pour le client compression
 *----------------------------------------------*/

static void usage(const char *exeName, const char *numService, const char *message)
{
    fprintf(stderr, "Client compression de chaîne\n");
    fprintf(stderr, "usage : %s %s <chaîne>\n", exeName, numService);
    fprintf(stderr, "        %s      : numéro du service\n", numService);
    fprintf(stderr, "        <chaine> : chaîne à compresser\n");
    fprintf(stderr, "exemple d'appel :\n");
    fprintf(stderr, "    %s %s \"aaabbcdddd\"\n", exeName, numService);
    if (message != NULL)
        fprintf(stderr, "message : %s\n", message);
    exit(EXIT_FAILURE);
}


/*----------------------------------------------*
 * fonction de vérification des paramètres
 *----------------------------------------------*/

void client_compression_verifArgs(int argc, char * argv[])
{
    if (argc != 3)
        usage(argv[0], argv[1], "nombre d'arguments");
    // éventuellement d'autres tests
    else if(strlen(argv[2]) == 0)
    {
        usage(argv[0], argv[1], "la chaîne à compresser est vide");
    } 
}


/*----------------------------------------------*
 * fonctions de communication avec le service
 *----------------------------------------------*/

// ---------------------------------------------
// fonction d'envoi des données du client au service
// Les paramètres sont
// - le file descriptor du tube de communication vers le service
// - la chaîne devant être compressée
static void sendData(int pts, const char* message)
{
    //mémorisation de la taille de la chaîne à envoyer
    int len = strlen(message);

    //envoi de la longueur de la chaîne à compresser
    int ret = write(pts, &len, sizeof(int));

    myassert(ret != -1, "Erreur : Echec de l'écriture dans le tube");
    myassert(ret == sizeof(int), "Erreur : Données mal écrites");

    // envoi de la chaîne à compresser
    ret = write(pts, message, sizeof(char) * len);

    myassert(ret != -1, "Erreur : Echec de l'écriture dans le tube");
    myassert(ret == (int)(sizeof(char) * len), "Erreur : Données mal écrites");
}

// ---------------------------------------------
// fonction de réception des résultats en provenance du service et affichage
// Les paramètres sont
// - le file descriptor du tube de communication en provenance du service
// - autre chose si nécessaire
static void receiveResult(int pfs)
{
    //récupération de la longueur de la chaîne compressée
    int len = 0;

    int ret = read(pfs, &len, sizeof(int));

    myassert(ret != -1, "Erreur : Echec de la lecture dans le tube");
    myassert(ret == sizeof(int), "Erreur : Données mal lues");

    // récupération de la chaîne compressée
    char* res;
    MY_MALLOC(res, char, len);

    ret = read(pfs, res, sizeof(char) * len);

    myassert(ret != -1, "Erreur : Echec de la lecture dans le  tube");
    myassert(ret == (int)(sizeof(char) * len), "Erreur : Données mal lues");
    
    // affichage du résultat
    printf("La chaîne compressée est : %s\n", res);

    MY_FREE(res);
}

// ---------------------------------------------
// Fonction appelée par le main pour gérer la communications avec le service
// Les paramètres sont
// - les deux file descriptors des tubes nommés avec le service
// - argc et argv fournis en ligne de commande
// Cette fonction analyse argv et en déduit les données à envoyer
//    - argv[2] : la chaîne à compresser
void client_compression(int pts, int pfs, int argc, char * argv[])
{
    //Pour ne pas avoir de warning sur l'inutilisation de argc
    myassert(argc == 3, "Nombre de paramètres invalide");


    sendData(pts, argv[2]);
    receiveResult(pfs);
}

