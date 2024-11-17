#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


#include "../ORCHESTRE_SERVICE/orchestre_service.h"
#include "client_service.h"
#include "service.h"
#include "service_somme.h"
#include "service_compression.h"
#include "service_sigma.h"

#include "../UTILS/io.h"
#include "../UTILS/myassert.h"

static void usage(const char *exeName, const char *message)
{
    fprintf(stderr, "usage : %s <num_service> <clé_sémaphore> <fd_tube_anonyme> "
            "<nom_tube_service_vers_client> <nom_tube_client_vers_service>\n",
            exeName);
    fprintf(stderr, "        <num_service>     : entre 0 et %d\n", SERVICE_NB - 1);
    fprintf(stderr, "        <clé_sémaphore>   : entre ce service et l'orchestre (clé au sens ftok)\n");
    fprintf(stderr, "        <fd_tube_anonyme> : entre ce service et l'orchestre\n");
    fprintf(stderr, "        <nom_tube_...>    : noms des deux tubes nommés reliés à ce service\n");
    if (message != NULL)
        fprintf(stderr, "message : %s\n", message);
    exit(EXIT_FAILURE);
}

static int my_semget(int key)
{
    int semId;

    //Récupération du sémaphore
    semId = semget(key, 1, 0);
    myassert(semId != -1, "Erreur : Echec de la récupération du sémaphore");

    return semId;
}

static void my_op_moins(int semId)
{

    struct sembuf op = {0, -1, 0};

    int ret = semop(semId, &op, 1);
    myassert(ret != -1, "Echec de l'operation sur le semaphor");

}

static void open_pipes(int* fd_rd, int* fd_wr, char* pipe_rd, char* pipe_wr)
{
    *fd_rd = open(pipe_rd, O_RDONLY);
    myassert(*fd_rd != -1, "Erreur : Echec de l'ouverture du tube");

    *fd_wr = open(pipe_wr, O_WRONLY);
    myassert(*fd_wr != -1, "Erreur : Echec de l'ouverture du tube");
}

static int read_int(int fd)
{
    int res;

    int ret = read(fd, &res, sizeof(int));
    myassert(ret != -1, "Echec de la lecture dans le tube anonyme");
    myassert(ret == sizeof(int), "Erreur, données mal lues");
    
    return res;
}

static void write_int(int fd, const int msg)
{
    int ret = write(fd, &msg, sizeof(int));
    myassert(ret != -1, "Erreur : Echec de l'écriture dans le tube");
    myassert(ret == sizeof(int), "Erreur : Données mal écrites");
}

static void close_pipes(int fd_rd, int fd_wr)
{
    int ret = close(fd_rd);
    myassert(ret != -1, "Erreur : Echec de la fermeture du tube");

    ret = close(fd_wr);
    myassert(ret != -1, "Erreur : Echec de la fermeture du tube");
}

/*----------------------------------------------*
 * fonction main
 *----------------------------------------------*/
int main(int argc, char * argv[])
{
    if (argc != 6)
        usage(argv[0], "nombre paramètres incorrect");

    // initialisations diverses : analyse de argv

    int numService = io_strToInt(argv[1]);
    int fd = io_strToInt(argv[3]);
    int key = io_strToInt(argv[2]);

    const char * pipe_name_stc = argv[4];
    const char * pipe_name_cts = argv[5];

    int ret_code_orchestre, mdp_orc, mdp_cli;
    
    int fd_stc, fd_cts;

    int semId = my_semget(key);

    while (true)
    {
        // attente d'un code de l'orchestre (via tube anonyme)
        ret_code_orchestre = read_int(fd);
        // si code de fin
        if(ret_code_orchestre == -1)
        {
            //sortie de la boucle
            break;
        }
        else
        {
            // sinon
            //    réception du mot de passe de l'orchestre
            
            mdp_orc = read_int(fd);

            //    ouverture des deux tubes nommés avec le client
            if(numService == SERVICE_SOMME){
                open_pipes(&fd_cts, &fd_stc, pipe_name_cts, pipe_name_stc);
            }
            else if(numService == SERVICE_COMPRESSION)
            {
                open_pipes(&fd_cts, &fd_stc, pipe_name_cts, pipe_name_stc);
            }
            else if(numService == SERVICE_SIGMA)
            {
                open_pipes(&fd_cts, &fd_stc, pipe_name_cts, pipe_name_stc);
            }
            //    attente du mot de passe du client
            mdp_cli = read_int(fd_cts);
            //    si mot de passe incorrect
            if(mdp_cli != mdp_orc){
            //        envoi au client d'un code d'erreur // = 1 pr l'instant (tu peux changer stv mais faudra me prévenir que je change ds client)
                int code_err = 1;
                write_int(fd_stc, code_err);
            }
            else //    sinon
            {
                //        envoi au client d'un code d'acceptation // ce que tu veux sauf code d'erreur lol
                //        appel de la fonction de communication avec le client :
                //            une fct par service selon numService (cf. argv[1]) :
                //                   . service_somme
                //                ou . service_compression
                //                ou . service_sigma
                //        attente de l'accusé de réception du client
                int code_acc = 0;
                write_int(fd_stc, code_acc);

                if(numService == SERVICE_SOMME){
                    service_somme(fd_stc, fd_cts);
                }
                else if(numService == SERVICE_COMPRESSION)
                {
                    service_compression(fd_stc, fd_cts);
                }
                else if(numService == SERVICE_SIGMA)
                {
                    service_sigma(fd_stc, fd_cts);
                }

            } //    finsi
            //    fermeture ici des deux tubes nommés avec le client
            close_pipes(fd_cts, fd_stc);
            //    modification du sémaphore pour prévenir l'orchestre de la fin
            
            my_op_moins(semId);

            // finsi
        }
    }

    // libération éventuelle de ressources
    
    return EXIT_SUCCESS;
}
