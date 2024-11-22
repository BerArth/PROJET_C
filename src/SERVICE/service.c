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
#include "../UTILS/myassert.h"
#include "../ORCHESTRE_SERVICE/orchestre_service.h"
#include "../CLIENT_SERVICE/client_service.h"
#include "service_somme.h"
#include "service_compression.h"
#include "service_sigma.h"
#include "service.h"

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

//Fonction permettant de récupérer le sémaphore créé par l'orchestre
static int my_semget(key_t key)
{
    int semId;

    //Récupération du sémaphore
    semId = semget(key, 1, 0);
    myassert(semId != -1, "Erreur : Echec de la récupération du sémaphore");

    return semId;
}

//Fonction diminuant le sémaphore de 1
static void my_op_moins(int semId)
{

    struct sembuf op = {0, -1, 0};

    int ret = semop(semId, &op, 1);
    myassert(ret != -1, "Echec de l'operation sur le semaphor");

}

//Fonction de lecture d'un entier dans un fichier
//Retourne l'entier lu
static int read_int(int fd)
{
    int res;

    int ret = read(fd, &res, sizeof(int));
    myassert(ret != -1, "Echec de la lecture dans le tube anonyme");
    myassert(ret == sizeof(int), "Erreur, données mal lues");
    
    return res;
}

//Fonction d'écriture d'un entier dans un fichier
static void write_int(int fd, const int msg)
{
    int ret = write(fd, &msg, sizeof(int));
    myassert(ret != -1, "Erreur : Echec de l'écriture dans le tube");
    myassert(ret == sizeof(int), "Erreur : Données mal écrites");
}


/*----------------------------------------------*
 * fonction main
 *----------------------------------------------*/
int main(int argc, char * argv[])
{
    if (argc != 6)
        usage(argv[0], "nombre paramètres incorrect");

    printf("Entre dans service main\n");
    // initialisations diverses : analyse de argv
    //Numéro de service demandé 
    int numService = io_strToInt(argv[1]);
    printf("numService = %d\n", numService);
    
    //File descriptor du tube anonyme
    int fd = io_strToInt(argv[3]);

    //Clé du sémaphore
    key_t key = io_strToInt(argv[2]);

    //Noms des 2 tubes nommés
    const char * pipe_name_stc = argv[4];
    const char * pipe_name_cts = argv[5];

   

    //Code de retour
    int ret_code_orchestre;

    //Mots de passe envoyé par l'orchestre et par le client
    int mdp_orc, mdp_cli;
    
    //File descriptors des 2 tubes nommées service -> client et client -> service
    int fd_stc, fd_cts; 

    //Récupération de l'identifiant du sémaphore
    int semId = my_semget(key);


    while (true)
    {
        printf("Entre dans while main\n");
        // attente d'un code de l'orchestre (via tube anonyme)
        ret_code_orchestre = read_int(fd);
        printf("ret_code_orchest = %d\n", ret_code_orchestre);
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
            printf("mdp_orchestre_service (sans service) : %d\n", mdp_orc);

            //    ouverture des deux tubes nommés avec le client
            printf("pipe stc = %s\n", pipe_name_stc);
            printf("pipe cts = %s\n", pipe_name_cts);
            open_pipes_CS(1, pipe_name_stc, &fd_stc, pipe_name_cts, &fd_cts);
            printf("ouverture des pipes cs (dans service)\n");

            //    attente du mot de passe du client
            mdp_cli = read_int(fd_cts);
            printf("mdp du client = %d (dans service)\n", mdp_cli);

            //    si mot de passe incorrect
            if(mdp_cli != mdp_orc){
            //        envoi au client d'un code d'erreur // = 1 pr l'instant (tu peux changer stv mais faudra me prévenir que je change ds client)
                int code_err = 1;
                write_int(fd_stc, code_err);
                printf("mdp incorrecte envoie de 1 ou client\n");

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
                printf("ecrit 0 vers client\n");
                if(numService == SERVICE_SOMME){
                    printf("on lance service somme\n");
                    service_somme(fd_stc, fd_cts);
                    printf("service somme, est lancé\n");
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
            printf("service va fermé les pipes");
            close_pipes_CS(fd_stc, fd_cts);
            printf("service a fermé les pipes");
            //    modification du sémaphore pour prévenir l'orchestre de la fin
            
            my_op_moins(semId);

            // finsi
        }
    }

    // libération éventuelle de ressources
    
    return EXIT_SUCCESS;
}
