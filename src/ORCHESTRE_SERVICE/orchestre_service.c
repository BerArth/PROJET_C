#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "myassert.h"
#include "orchestre_service.h"

//Fonction de création d'un sémaphore service - orchestre
static void create_sem(int* semId, key_t* key, int idKey)
{
    //Création de la clé
    *key = ftok(FICHIER_SO, idKey);
    myassert(*key != -1, "Erreur : Echec de la création de la clé IPC");

    //Création du sémaphore
    *semId = semget(*key, 1, IPC_CREAT | IPC_EXCL | 0641);
    myassert(*semId != -1, "Erreur : Echec de la récupération du sémaphore");
}

//Fonction de création du sémaphore service - orchestre pour chaque service
//Modifie l'identifiants des sémaphores et les identifiants de clés passés en paramètre
void create_sem_SO(int* semId_SSO, key_t* key_SSO, int* semId_SC, key_t* key_SC, int* semId_SSI, key_t* key_SSI)
{
    //Service somme
    create_sem(semId_SSO, key_SSO, ID_SOMME);

    //Service compression
    create_sem(semId_SC, key_SC, ID_COMP);

    //Service somme
    //Création de la clé
    create_sem(semId_SSI, key_SSI, ID_SIGMA);
}


//Fonction d'ouverture d'un tube anonyme service <-> orchestre
static void create_pipe_ano(int fds[2])
{
    int ret = pipe(fds);
    myassert(ret == 0, "Erreur : Echec de la création du tube anonyme");
}

//Fonction de création du tube anonyme service <-> orchestre pour chaque service
void create_pipes_ano(int fds_SSO[2], int fds_SC[2], int fds_SSI[2])
{   
    //Service somme
    create_pipe_ano(fds_SSO);

    //Service compression
    create_pipe_ano(fds_SC);

    //Service sigma
    create_pipe_ano(fds_SSI);
}


//Fonction permettant de fermer le tube anonyme service <-> orchestre
static void close_pipe_ano(int fds[2])
{
    int ret = close(fds[0]);
    myassert(ret != -1, "Erreur : Echec de la fermeture du tube");

    ret = close(fds[1]);
    myassert(ret != -1, "Erreur : Echec de la fermeture du tube");
}

//Fonction de fermetures du tube anonyme service <-> orchestre pour chaque service
void close_pipes_ano(int fds_SSO[2], int fds_SC[2], int fds_SSI[2])
{
    //Service somme
    close_pipe_ano(fds_SSO);
    close_pipe_ano(fds_SC);
    close_pipe_ano(fds_SSI);
}
