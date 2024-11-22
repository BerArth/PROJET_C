#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include "myassert.h"
#include "client_service.h"

//Fonction de création de 2 tubes nommés service -> client et client -> service
static void create_pipe(const char* filenameSC, const char* filenameCS)
{
    int ret = mkfifo(filenameSC, 0644);
    myassert(ret == 0, "Erreur : Echec de la création du tube");

    ret = mkfifo(filenameCS, 0644);
    myassert(ret == 0, "Erreur : Echec de la création du tube");
}

//Fonction de création des tubes nommés service -> client et client -> service pour chaque service 
void create_pipes_CS()
{
    //Service somme
    create_pipe(PIPE_SSOTC, PIPE_CTSSO);

    //Service compression
    create_pipe(PIPE_SCTC, PIPE_CTSC);

    //Service sigma
    create_pipe(PIPE_SSITC, PIPE_CTSSI);
}


//Fonction permettant d'ouvrir les 2 tubes client -> service et orchestre -> service pour chaque service
//Modifie les file descriptors passés en paramètre
void open_pipes_CS(int isClient, const char* filenameSC, int* fd_STC, const char* filenameCS, int* fd_CTS)
{  
    printf("Entrez dans open pipe\n");

    //Ouverture des tubes côté client
    if(isClient == 0)
    {
        printf("Ouvertur pipe coté client\n");

        *fd_STC = open(filenameSC, O_RDONLY);
        myassert(*fd_STC != -1, "Erreur : Echec de l'ouverture du tube");
        
        printf("pipe 1 open\n");

        *fd_CTS = open(filenameCS, O_WRONLY);
        myassert(*fd_CTS != -1, "Erreur : Echec de l'ouverture du tube");
        printf("pipe 2 open\n");

        


        
    }

    //Ouverture des tubes côté service
    else
    {

        printf("Ouvertur pipe coté service\n");

        *fd_STC = open(filenameSC, O_WRONLY);
        myassert(*fd_STC != -1, "Erreur : Echec de l'ouverture du tube");

        printf("pipe 1 open\n");

        *fd_CTS = open(filenameCS, O_RDONLY);
        myassert(*fd_CTS != -1, "Erreur : Echec de l'ouverture du tube");
        printf("pipe 2 open\n");

        

        
        
    }

    printf("ouverture des pipes OK\n");
    
}


//Fonction de fermeture des 2 tubes nommés client -> service et service -> client
void close_pipes_CS(int fd_STC, int fd_CTS)
{
    int ret = close(fd_STC);
    myassert(ret != -1, "Erreur : Echec de la fermeture du tube");

    ret = close(fd_CTS);
    myassert(ret != -1, "Erreur : Echec de la fermeture du tube");
}
