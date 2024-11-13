#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>

#include "orchestre_service.h"
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


static void open_pipes(int* fd_rd, int* fd_wr, char* pipe_rd, char* pipe_wr)
{
    *fd_rd = open(pipe_rd, O_RDONLY);
    myassert(*fd_rd != -1, "Erreur : Echec de l'ouverture du tube");

    *fd_wr = open(pipe_wr, O_WRONLY);
    myassert(*fd_wr != -1, "Erreur : Echec de l'ouverture du tube");
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
    int fd = io_strToInt(argv[2]);

    int ret, ret_code_orchestre, mdp_orc, mdp_cli;
    
    int fd_stc, fd_cts;

    char * pipe_stc;
    char * pipe_cts;


    while (true)
    {
        // attente d'un code de l'orchestre (via tube anonyme)
        ret = read(fd, &ret_code_orchestre, sizeof(int));
        myassert(ret != -1, "Echec de la lecture dans le tube anonyme");
        myassert(ret == sizeof(int));
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
            ret = read(fd, &mdp, sizeof(int));
            //    ouverture des deux tubes nommés avec le client
            if(numService == 0){
                open_pipes(&fd_cts, &fd_stc, PIPE_SSOTC, PIPE_CTSSO);
            }
            else if 
            {
                open_pipes(&fd_cts, &fd_stc, PIPE_SCTC, PIPE_CTSC);
            }
            else if 
            {
                open_pipes(&fd_cts, &fd_stc, PIPE_SSITC, PIPE_CTSSI);
            }
            //    attente du mot de passe du client
            ret = read(fd_cts, &mdp_cli, sizeof(int));
            myassert(ret != -1,);
            //    si mot de passe incorrect
            if(mdp_cli != mdp_orc){
            //        envoi au client d'un code d'erreur // = 1 pr l'instant (tu peux changer stv mais faudra me prévenir que je change ds client)

            }
            //    sinon
            //        envoi au client d'un code d'acceptation // ce que tu veux sauf code d'erreur lol
            //        appel de la fonction de communication avec le client :
            //            une fct par service selon numService (cf. argv[1]) :
            //                   . service_somme
            //                ou . service_compression
            //                ou . service_sigma
            //        attente de l'accusé de réception du client
            //    finsi
            //    fermeture ici des deux tubes nommés avec le client
            //    modification du sémaphore pour prévenir l'orchestre de la fin
            // finsi
        }
    }

    // libération éventuelle de ressources
    
    return EXIT_SUCCESS;
}
