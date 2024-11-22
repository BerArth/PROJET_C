#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "../UTILS/io.h"
#include "../UTILS/memory.h"
#include "../UTILS/myassert.h"
#include "../SERVICE/service.h"
#include "../CLIENT_ORCHESTRE/client_orchestre.h"
#include "../CLIENT_SERVICE/client_service.h"
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
    int semId;

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

    int ret = semop(semId, &operationMoins, 1); //Bloque si le sémaphore est à 0
    myassert(ret != -1, "Erreur : Echec de l'opération pour entrer en section critique");
}

//Fonction permettant de sortir de la section critique
static void sortir_sc(int semId)
{
    struct sembuf operationPlus = {0, 1, 0};

    int ret = semop(semId, &operationPlus, 1);
    myassert(ret != -1, "Erreur : Echec de l'opération pour sortir de la section critique");
}

//Fonction permettant de lire une chaîne de caractères dans un tube
//Retourne la chaîne de caractère lue
static void read_str(int fd, char** res)
{
    int ret, len;

    ret = read(fd, &len, sizeof(int));
    myassert(ret != -1, "Erreur : Echec de la lecture dans le tube");
    myassert(ret == sizeof(int), "Erreur : Données mal lues");

    MY_REALLOC(*res, *res, char, len);

    ret = read(fd, *res, sizeof(char) * len);
    myassert(ret != -1, "Erreur : Echec de la lecture dans le tube");
    myassert(ret == (int)(sizeof(char) * len), "Erreur : Données mal lues");
}

//Fonction permettant de lire un entier dans un tube
//Retourne l'entier lue
static int read_int(int fd)
{
    int res;

    int ret = read(fd, &res, sizeof(int));
    printf("read_int client res = %d (ret = %d)\n", res, ret);
    myassert(ret != -1, "Erreur : Echec de la lecture dans le tube");
    myassert(ret == sizeof(int), "Erreur : Données mal lues");

    return res;
}

//Fonction permettant d'écrire un entier dans un tube
static void write_int(int fd, const int msg)
{
    int ret = write(fd, &msg, sizeof(int));
    myassert(ret != -1, "Erreur : Echec de l'écriture dans le tube");
    myassert(ret == sizeof(int), "Erreur : Données mal écrites");
}


int main(int argc, char * argv[])
{
    if (argc < 2)
        usage(argv[0], "nombre paramètres incorrect");

    printf("Entre dans le maine client\n");

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
    //Variable pour récupérer les codes de retour
    int code_ret;
    
    //Variable pour (eventuellement) récuperer le mot de passe
    int password = 0;

    //Récupération du sémaphore
    int semId = my_semget();

    //Descripteurs des tubes
    int fd_otc, fd_cto; //orchestre -> client et client -> orchestre 
    int fd_stc, fd_cts; //service -> client et client -> service

    //Variables pr récupérer les noms des tubes service <-> client
    char* pipe_stc;
    MY_MALLOC(pipe_stc, char, 1); //Allocation de la taille d'un caractère pour faire systématiquement un free
    char* pipe_cts;
    MY_MALLOC(pipe_cts, char, 1); //Allocation de la taille d'un caractère pour faire systématiquement un free


    // entrée en section critique pour communiquer avec l'orchestre
    printf("client va entre en sc\n");
    entrer_sc(semId);
    printf("client entre en sc\n");
    // ouverture des tubes avec l'orchestre
    open_pipes_CO(0, &fd_cto, &fd_otc); 
    printf("client ouvre les pipes\n");
    // envoi à l'orchestre du numéro du service
    write_int(fd_cto, numService);
    printf("client ecrtit dans le pipe\n");
    // attente code de retour
    code_ret = read_int(fd_otc);

    // si code d'erreur (service déjà en cours d'utilisation ou fermé)
    //     afficher un message erreur
    if(code_ret == -2)
    {
        printf("Service en cours d'utilisation.\n");
    }
    else if(code_ret == -3)
    {
        printf("Service non ouvert.\n");
    }
    // sinon si demande d'arrêt (i.e. numService == -1)
    //     afficher un message
    else if(code_ret == -1)
    {
        printf("Arrêt en cours.\n");
    }
    // sinon
    //     récupération du code d'acceptation?? (= celui contenu dans code_ret) puis du mot de passe et des noms des 2 tubes
    else
    {
        read_str(fd_otc, &pipe_cts);
        printf("pipe cts = %s\n", pipe_cts);
        read_str(fd_otc, &pipe_stc);
        printf("pipe stc = %s\n", pipe_stc);
        password = read_int(fd_otc);
        printf("%d password\n", password);
        
    }
    // finsi
    

    // envoi d'un accusé de réception à l'orchestre
    printf("on tente d'envoyer accusé  ce recep\n");
    write_int(fd_cto, 0);
    printf("on a envoyé l'accuse\n");
    // fermeture des tubes avec l'orchestre
    printf("on ferme le pipe co\n");
    close_pipes_CO(fd_cto, fd_otc);
    printf("on a fermé le pipe\n");
    // on prévient l'orchestre qu'on a fini la communication (cf. orchestre.c)
    // sortie de la section critique
    sortir_sc(semId);
    printf("ejufbnzof,ze\n");

    // si pas d'erreur et service normal
    if(code_ret == 0)
    {
        //     ouverture des tubes avec le service
        printf("on tente d'open les pipes\n");
        open_pipes_CS(0, pipe_stc, &fd_stc, pipe_cts, &fd_cts);

        //     envoi du mot de passe au service
        write_int(fd_cts, password);

        //     attente de l'accusé de réception du service
        code_ret = read_int(fd_stc);
        printf("read_int client code ret = %d\n", code_ret);
        //     si mot de passe non accepté
        //         message d'erreur
        if(code_ret == 1)
        {
            printf("Mot de passe incorrect.\n");
        }
        //     sinon
        //         appel de la fonction de communication avec le service :
        //             une fct par service selon numService :
        //                    . client_somme
        //                 ou . client_compression
        //                 ou . client_sigma
        //         envoi d'un accusé de réception au service <- à faire
        else
        {
            if(numService == SERVICE_SOMME)
            {
                client_somme(fd_cts, fd_stc, argc, argv);
            }
            else if(numService == SERVICE_COMPRESSION)
            {
                client_compression(fd_cts, fd_stc, argc, argv);
            }
            else if(numService == SERVICE_SIGMA)
            {
                client_sigma(fd_cts, fd_stc, argc, argv);
            }   
        }
        //     finsi

        //     fermeture des tubes avec le service
        close_pipes_CS(fd_stc, fd_cts);
        
    }
    //finsi


    // libération éventuelle de ressources
    MY_FREE(pipe_cts);
    MY_FREE(pipe_stc);
    

    return EXIT_SUCCESS;
}
