#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "io.h"
#include "memory.h"
#include "myassert.h"

#include "service.h"
#include "client_orchestre.h"
#include "client_service.h"

#include "client_arret.h"
#include "client_somme.h"
#include "client_sigma.h"
#include "client_compression.h"


static void usage(const char *exeName, const char *message)
{
    fprintf(stderr, "usage : %s <num_service> ...\n", exeName);
    fprintf(stderr, "        <num_service> : entre -1 et %d\n", SERVICE_NB - 1);
    fprintf(stderr, "                        -1 signifie l'arrêt de l'orchestre\n");
    fprintf(stderr, "        ...           : les paramètres propres au service\n");
    if (message != NULL)
        fprintf(stderr, "message : %s\n", message);
    exit(EXIT_FAILURE);
}

//Fonction permettant de récupérer le sémaphore créé par l'orchestre
static int my_semget()
{
    key_t key;
    int sem_id;

    //Création de la clé
    key = ftok(FICHIER_CO, ID_CO);
    myassert(key != -1, "Erreur : Echec de la création de la clé IPC");

    //Récupération du sémaphore
    semId = semget(key, 1, 0);
    myassert(semId != -1, "Erreur : Echec de la récupération du sémaphore");

    return semId;
}

//Fonction permettant de rentrer en section critique
static void entrer_sc(int semId)
{
    struct sembuf operationMoins = {0, -1, 0};

    int ret = semop(semId, &operationMoins, 1);
    myassert(ret != -1, "Erreur : Echec de l'opération pour entrer en section critique");
}

static void sortir_sc(int semId)
{
    struct sembuf operationPlus = {0, 1, 0};

    int ret = semop(semId, &operationPlus, 1);
    myassert(ret != -1, "Erreur : Echec de l'opération pour sortir de la section critique");
}

static void open_pipes(int* fd_rd, int* fd_wr, char* pipe_rd, char* pipe_wr)
{
    *fd_rd = open(pipe_rd, O_RDONLY);
    myassert(*fd_rd != -1, "Erreur : Echec de l'ouverture du tube");

    *fd_wr = open(pipe_wr, O_WRONLY);
    myassert(*fd_wr != -1, "Erreur : Echec de l'ouverture du tube");
}

int main(int argc, char * argv[])
{
    if (argc < 2)
        usage(argv[0], "nombre paramètres incorrect");

    int numService = io_strToInt(argv[1]);
    if (numService < -1 || numService >= SERVICE_NB)
        usage(argv[0], "numéro service incorrect");

    // appeler la fonction de vérification des arguments
    //     une fct par service selon numService
    //            . client_arret_verifArgs
    //         ou . client_somme_verifArgs
    //         ou . client_compression_verifArgs
    //         ou . client_sigma_verifArgs
    if(numService == SERVICE_ARRET)
    {
        client_arret_verifArgs(argc, argv);
    }
    else if(numService == SERVICE_SOMME)
    {
        client_somme_verifArgs(argc, argv);
    }
    else if(numService == SERVICE_COMPRESSION)
    {
        client_compression_verifArgs(argc, argv);
    }
    else if(numService == SERVICE_SIGMA)
    {
        client_sigma_verifArgs(argc, argv);
    }

    // initialisations diverses s'il y a lieu
    int ret, code_ret;
    
    //Variable pour (eventuellement) récuperer le mot de passe
    int password = 0;

    //Récupération du sémaphore
    int semId = my_semget();

    //Descripteurs des tubes orchestre->client et client->orcherstre
    int fd_otc, fd_cto;

    //Variables pr les noms des tubes service <-> client + leurs tailles (peut être non utilisées si service indisponible)
    int len = 0;

    char* pipe_stc, pipe_cts;

    // entrée en section critique pour communiquer avec l'orchestre
    entrer_sc(semId);
    
    // ouverture des tubes avec l'orchestre
    open_pipes(); 

    // envoi à l'orchestre du numéro du service
    ret = write(fd_cto, &numService, sizeof(int));
    myassert(ret != -1, "Erreur : Echec de l'écriture dans le tube");
    myassert(ret == sizeof(int), "Erreur : Données mal écrites");

    // attente code de retour
    ret = read(fd_otc, &code_ret, sizeof(int));
    myassert(ret != -1, "Erreur : Echec de la lecture dans le tube");
    myassert(ret == sizeof(int), "Erreur : Données mal lues");

    // si code d'erreur (service déjà en cours d'utilisation ou fermé)
    //     afficher un message erreur
    if(code_ret == -2)
    {
        printf("Service indisponible pour le moment.\n");
    }
    // sinon si demande d'arrêt (i.e. numService == -1)
    //     afficher un message
    else if(code_ret == -1)
    {
        printf("Arrêt demandé.\n");
    }
    // sinon
    //     récupération du code d'acceptation (= celui contenu dans code_ret) puis du mot de passe et des noms des 2 tubes
    else
    {
        ret = read(fd_otc, &password, sizeof(int));
        myassert(ret != -1, "Erreur : Echec de la lecture dans le tube");
        myassert(ret == sizeof(int), "Erreur : Données mal lues");
        
        ret = read(fd_otc, &len, sizeof(int));
        myassert(ret != -1, "Erreur : Echec de la lecture dans le tube");
        myassert(ret == sizeof(int), "Erreur : Données mal lues");


    }
    // finsi
    

    // envoi d'un accusé de réception à l'orchestre
    // fermeture des tubes avec l'orchestre
    // on prévient l'orchestre qu'on a fini la communication (cf. orchestre.c)
    // sortie de la section critique
    //
    // si pas d'erreur et service normal
    //     ouverture des tubes avec le service
    //     envoi du mot de passe au service
    //     attente de l'accusé de réception du service
    //     si mot de passe non accepté
    //         message d'erreur
    //     sinon
    //         appel de la fonction de communication avec le service :
    //             une fct par service selon numService :
    //                    . client_somme
    //                 ou . client_compression
    //                 ou . client_sigma
    //         envoi d'un accusé de réception au service
    //     finsi
    //     fermeture des tubes avec le service
    // finsi

    // libération éventuelle de ressources
    
    return EXIT_SUCCESS;
}
