#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "myassert.h"
#include "client_orchestre.h"

//Fonction de création du sémaphore pour la communication entre client et orchestre
//Retourne l'identifiant du sémaphore et modifie la clé passée en paramètre
int create_sem_CO(key_t* key)
{
    int semId;

    //Création de la clé
    *key = ftok(FICHIER_CO, ID_CO);
    myassert(*key != -1, "Erreur : Echec de la création de la clé IPC");

    //Création du sémaphore
    semId = semget(*key, 1, IPC_CREAT | IPC_EXCL | 0641);
    myassert(semId != -1, "Erreur : Echec de la récupération du sémaphore");

    int ret = semctl(semId, 0, SETVAL, 1);
    myassert(ret != -1, "Erreur : Echec de l'initialisation du sémaphore");

    return semId;
}

//Fonction de création de 2 tubes nommés orchestre -> client et client -> orchestre
void create_pipes_CO()
{
    int ret = mkfifo(PIPE_OTC, 0644);
    myassert(ret == 0, "Erreur : Echec de la création du tube");

    ret = mkfifo(PIPE_CTO, 0644);
    myassert(ret == 0, "Erreur : Echec de la création du tube");
}

//Fonction permettant d'ouvrir les 2 tubes client -> orchestre et orchestre -> client
//Modifie les file descriptors passés en paramètre
void open_pipes_CO(const int isClient, int* fd_cto, int* fd_otc)
{
    //Ouverture des tubes côtés client
    if(isClient == 0)
    {
        *fd_cto = open(PIPE_CTO, O_WRONLY);
        myassert(*fd_cto != -1, "Erreur : Echec de l'ouverture du tube");

        *fd_otc = open(PIPE_OTC, O_RDONLY);
        myassert(*fd_otc != -1, "Erreur : Echec de l'ouverture du tube");
    }

    //Ouverture des tubes côté orchestre
    else
    {
        *fd_cto = open(PIPE_CTO, O_RDONLY);
        myassert(*fd_cto != -1, "Erreur : Echec de l'ouverture du tube");

        *fd_otc = open(PIPE_OTC, O_WRONLY);
        myassert(*fd_otc != -1, "Erreur : Echec de l'ouverture du tube");
    }
    
}

//Fonction permettant de fermer les 2 tubes nommés client -> orchestre et orchestre -> client
void close_pipes_CO(int fd_cto, int fd_otc)
{
    int ret = close(fd_cto);
    myassert(ret != -1, "Erreur : Echec de la fermeture du tube");

    ret = close(fd_otc);
    myassert(ret != -1, "Erreur : Echec de la fermeture du tube");
}