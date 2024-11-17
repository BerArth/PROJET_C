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

    int ret = semop(semId, &operationMoins, 1);
    myassert(ret != -1, "Erreur : Echec de l'opération pour entrer en section critique");
}

//Fonction permettant de sortir de la section critique
static void sortir_sc(int semId)
{
    struct sembuf operationPlus = {0, 1, 0};

    int ret = semop(semId, &operationPlus, 1);
    myassert(ret != -1, "Erreur : Echec de l'opération pour sortir de la section critique");
}

//Fonction permettant d'ouvrir 2 tubes pour une communication bidirectionnelle
static void open_pipes(int* fd_rd, int* fd_wr, char* pipe_rd, char* pipe_wr)
{
    *fd_rd = open(pipe_rd, O_RDONLY);
    myassert(*fd_rd != -1, "Erreur : Echec de l'ouverture du tube");

    *fd_wr = open(pipe_wr, O_WRONLY);
    myassert(*fd_wr != -1, "Erreur : Echec de l'ouverture du tube");
}

//Fonction permettant de fermer 2 tubes
static void close_pipes(int fd_rd, int fd_wr)
{
    int ret = close(fd_rd);
    myassert(ret != -1, "Erreur : Echec de la fermeture du tube");

    ret = close(fd_wr);
    myassert(ret != -1, "Erreur : Echec de la fermeture du tube");
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
    myassert(ret == (int)(sizeof(int) * len), "Erreur : Données mal lues");
}

//Fonction permettant de lire un entier dans un tube
//Retourne l'entier lue
static int read_int(int fd)
{
    int res;

    int ret = read(fd, &res, sizeof(int));
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
    entrer_sc(semId);
    
    // ouverture des tubes avec l'orchestre
    open_pipes(&fd_otc, &fd_cto, PIPE_OTC, PIPE_CTO); 

    // envoi à l'orchestre du numéro du service
    write_int(fd_cto, numService);

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
        password = read_int(fd_otc);

        read_str(fd_otc, &pipe_cts);
        read_str(fd_otc, &pipe_stc);
    }
    // finsi
    

    // envoi d'un accusé de réception à l'orchestre
    write_int(fd_cto, 0);

    // fermeture des tubes avec l'orchestre
    close_pipes(fd_otc, fd_cto);

    // on prévient l'orchestre qu'on a fini la communication (cf. orchestre.c)
    // sortie de la section critique
    sortir_sc(semId);


    // si pas d'erreur et service normal
    if(code_ret != -2 && code_ret != -1)
    {
        //     ouverture des tubes avec le service
        open_pipes(&fd_stc, &fd_cts, pipe_stc, pipe_cts);

        //     envoi du mot de passe au service
        write_int(fd_cts, password);

        //     attente de l'accusé de réception du service
        code_ret = read_int(fd_stc);

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
        close_pipes(fd_stc, fd_cts);
        
    }
    //finsi


    // libération éventuelle de ressources
    MY_FREE(pipe_cts);
    MY_FREE(pipe_stc);
    

    return EXIT_SUCCESS;
}
