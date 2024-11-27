/*****************************************************************************
 * auteurs : Arthur Bertrand-Bernard, Leïla Cooper
 *
 * fichier : orchestre.c
 *
 * note :
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/wait.h>
#include <string.h>


#include "../CONFIG/config.h"
#include "../CLIENT_SERVICE/client_service.h"
#include "../CLIENT_ORCHESTRE/client_orchestre.h"
#include "../ORCHESTRE_SERVICE/orchestre_service.h"
#include "../SERVICE/service.h"
#include "../UTILS/io.h"
#include "../UTILS/myassert.h"


static void usage(const char *exeName, const char *message)
{
    fprintf(stderr, "usage : %s <fichier config>\n", exeName);
    if (message != NULL)
        fprintf(stderr, "message : %s\n", message);
    exit(EXIT_FAILURE);
}

//Fonction dupliquant le programme avec un fork et remplace le code du fils par celui du service avec un exec
static void my_fork_exec(const int numService, const key_t semKey, const int fd_ano, char* pipe_stc, char* pipe_cts)
{
    int ret;
    pid_t pid;

    char* argv[7];

    argv[0] = "SERVICE/service";
    argv[1] = io_intToStr(numService);
    argv[2] = io_intToStr(semKey);
    argv[3] = io_intToStr(fd_ano);
    argv[4] = pipe_stc;
    argv[5] = pipe_cts;
    argv[6] = NULL;

    pid = fork();
    myassert(pid != -1, "Erreur : Echec fork\n");

    if(pid == 0)
    {
        ret = execv(argv[0], argv);
        myassert(ret!= -1, "Erreur : Echec execv\n");
    }
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

static void write_str(int fd ,const char * msg)
{
    int len = strlen(msg);
    write_int(fd, len);
    int ret = write(fd, msg, sizeof(char)* len);
    myassert(ret != -1 , "Erreur : Echec de l'écriture dans le tube");
    myassert(ret == (int)(sizeof(char) * len), "Erreur : Données mal écrites");
}

//Fonction générant un entier aléatoire dans un intervalle
//Retourne l'entier généré
int generate_number()
{
    int min = 10;
    int max = 20;

    return rand() % (max - min) + min;
}

//Fonction qui permet de voir la fin des traitements d'un service grâce au sémaphore
//Renvoie true si le sémaphore est à 0, false sinon
static bool is_service_finish(int semId)
{
    int state = semctl(semId, 0, GETVAL);
    myassert(state != -1, "Echec de la lecture du semaphore");

    if(state == 1)
    {
        return false;
    }
    else
    {
        return true;
    }
}

//Fonction qui permet de voir si le client a terminé la communication avec l'orchestre
//Renvoie true si le sémaphore est à 1, false sinon
static bool is_client_finish(int semId)
{
    int state = semctl(semId, 0, GETVAL);
    myassert(state != -1, "Echec de la lecture du semaphore");

    if(state == 0)
    {
        return false;
    }
    else
    {
        return true;
    }
}

//Fonction qui augmente le sémaphore de 1
static void my_op_plus(int semId)
{
    struct sembuf op = {0, 1, 0};
    
    int ret = semop(semId, &op, 1);
    myassert(ret != -1, "Echec de l'operation sur le semaphor");
}

//Fonction qui attend que le sémaphore passe à 0
static void my_op_wait_0(int semId)
{
    struct sembuf op = {0, 0, 0};
    
    int ret = semop(semId, &op, 1);
    myassert(ret != -1, "Echec de l'operation sur le semaphor");
}

//Fonction de suppression de tous les tubes nommés
static void my_unlink(){
    int ret = unlink(PIPE_SSOTC);
    myassert(ret != -1, "Echec de la supression du tube");
    ret = unlink(PIPE_CTSSO);
    myassert(ret != -1, "Echec de la supression du tube");
    ret = unlink(PIPE_SCTC);
    myassert(ret != -1, "Echec de la supression du tube");
    ret = unlink(PIPE_CTSC);
    myassert(ret != -1, "Echec de la supression du tube");
    ret = unlink(PIPE_SSITC);
    myassert(ret != -1, "Echec de la supression du tube");
    ret = unlink(PIPE_CTSSI);
    myassert(ret != -1, "Echec de la supression du tube");
    ret = unlink(PIPE_OTC);
    myassert(ret != -1, "Echec de la supression du tube");
    ret = unlink(PIPE_CTO);
    myassert(ret != -1, "Echec de la supression du tube");
}

//Fonction de destruction de tous les sémaphores
static void my_sem_dl(int semIdCO, int semIdSOC, int semIdCOC, int semIdSIC){
    int ret = semctl(semIdCO, -1, IPC_RMID);
    myassert(ret != -1 , "Echec de la destruction du semaphore");
    ret = semctl(semIdSOC, -1, IPC_RMID);
    myassert(ret != -1 , "Echec de la destruction du semaphore");
    ret = semctl(semIdCOC, -1, IPC_RMID);
    myassert(ret != -1 , "Echec de la destruction du semaphore");
    ret = semctl(semIdSIC, -1, IPC_RMID);
    myassert(ret != -1 , "Echec de la destruction du semaphore");
}

int main(int argc, char * argv[])
{
    srand(time(NULL));

    if (argc != 2)
    {
        usage(argv[0], "nombre paramètres incorrect");
    }
        
    bool fin = false;
    bool client_finish = false;

    //test
    int code_envoie;

    // lecture du fichier de configuration
    config_init(argv[1]);

    //Initialisation diverses

    //Variable contenant la demande du client
    int dmdc = -2;

    //Files descriptors des tubes orchestre -> client et client -> orcherstre
    int fd_otc, fd_cto;

    //Identifiants des sémaphores
    int semIdCO, semIdSOC, semIdCOC, semIdSIC;
    //Clé pour création des sémaphores
    key_t key_c, key_soc, key_coc, key_sic;

    //File descriptors des tubes anynomes orchestre -> service
    int fdsSOMME[2];
    int fdsCOMP[2];
    int fdsSIGMA[2];

    //Mot de passe à envoyer au client et au service concerné
    int password; 

    // Pour la communication avec les clients
    // - création de 2 tubes nommés pour converser avec les clients
    create_pipes_CO();

    // - création d'un sémaphore pour que deux clients ne
    //   ne communiquent pas en même temps avec l'orchestre
    semIdCO = create_sem_CO(&key_c);
    
    // lancement des services, avec pour chaque service :

    // - création d'un tube anonyme pour converser (orchestre vers service)
    create_pipes_ano(fdsSOMME, fdsCOMP, fdsSIGMA);

    // - un sémaphore pour que le service prévienne l'orchestre de la
    //   fin d'un traitement
    create_sem_SO(&semIdSOC, &key_soc, &semIdCOC, &key_coc, &semIdSIC, &key_sic);    
    
    // - création de deux tubes nommés (pour chaque service) pour les
    //   communications entre les clients et les services
    create_pipes_CS();

    //Lancement des services
    my_fork_exec(SERVICE_SOMME, key_soc, fdsSOMME[0], PIPE_SSOTC, PIPE_CTSSO);
    my_fork_exec(SERVICE_COMPRESSION, key_coc, fdsCOMP[0], PIPE_SCTC, PIPE_CTSC);
    my_fork_exec(SERVICE_SIGMA, key_sic, fdsSIGMA[0], PIPE_SSITC, PIPE_CTSSI); 
    

    while (!fin)
    {
     
        printf("En attente d'un client...\n");

        // ouverture ici des tubes nommés avec un client
        open_pipes_CO(1, &fd_cto, &fd_otc);

        // attente d'une demande de service du client
        dmdc = read_int(fd_cto);

        // détecter la fin des traitements lancés précédemment via
        // les sémaphores dédiés (attention on n'attend pas la
        // fin des traitement, on note juste ceux qui sont finis)

        bool state_somme = is_service_finish(semIdSOC);
        bool state_comp = is_service_finish(semIdCOC);
        bool state_sigma = is_service_finish(semIdSIC);

        // analyse de la demande du client
        // si ordre de fin
        //     envoi au client d'un code d'acceptation (via le tube nommé)
        //     marquer le booléen de fin de la boucle
        if(dmdc == SERVICE_ARRET)
        {
            code_envoie = -1;
            write_int(fd_otc, code_envoie);
            fin = true;
        }
        // sinon si service non ouvert
        //     envoi au client d'un code d'erreur (via le tube nommé)
        else if(!config_isServiceOpen(dmdc))
        {
            code_envoie = -3;
            write_int(fd_otc, code_envoie);
        }
        // sinon si service déjà en cours de traitement
        //     envoi au client d'un code d'erreur (via le tube nommé)
        else if(dmdc == SERVICE_SOMME && !(state_somme))
        {
            code_envoie = -2;
            write_int(fd_otc, code_envoie);
        }
        else if(dmdc == SERVICE_COMPRESSION && !(state_comp))
        {
            code_envoie = -2;
            write_int(fd_otc, code_envoie);
        }
        else if(dmdc == SERVICE_SIGMA && !(state_sigma))
        {
            code_envoie = -2;
            write_int(fd_otc, code_envoie);
        }
        // sinon
        //     envoi au client d'un code d'acceptation (via le tube nommé)
        //     génération d'un mot de passe
        //     envoi d'un code de travail au service (via le tube anonyme)
        //     envoi du mot de passe au service (via le tube anonyme)
        //     envoi du mot de passe au client (via le tube nommé)
        //     envoi des noms des tubes nommés au client (via le tube nommé)
        else
        {

            //envoi du code d'acceptation au client
            code_envoie = 0;
            write_int(fd_otc, code_envoie);

            //Génération du mot de passe
            password = generate_number();

            //envoi d'un code de travail au service et des noms des tubes nommés au client
            if(dmdc == SERVICE_SOMME)
            {
                code_envoie = 0;
                write_int(fdsSOMME[1], code_envoie);
                my_op_plus(semIdSOC);
                write_int(fdsSOMME[1], password);
                write_str(fd_otc, PIPE_CTSSO);
                write_str(fd_otc, PIPE_SSOTC);
                
            }
            else if(dmdc == SERVICE_COMPRESSION)
            {
                write_int(fdsCOMP[1], 0);
                my_op_plus(semIdCOC);
                write_int(fdsCOMP[1], password);
                write_str(fd_otc, PIPE_CTSC);
                write_str(fd_otc, PIPE_SCTC);
                
                
            }
            else if(dmdc == SERVICE_SIGMA)
            {
                write_int(fdsSIGMA[1], 0);
                my_op_plus(semIdSIC);
                write_int(fdsSIGMA[1], password);
                write_str(fd_otc, PIPE_CTSSI);
                write_str(fd_otc, PIPE_SSITC);
                
            }

            //envoi du mdp au client
            write_int(fd_otc, password);
            
        }
        // finsi

        // attente d'un accusé de réception du client
        read_int(fd_cto);
        
        // fermer les tubes vers le client
        close_pipes_CO(fd_cto, fd_otc);

        // il peut y avoir un problème si l'orchestre revient en haut de la
        // boucle avant que le client ait eu le temps de fermer les tubes
        // il faut attendre avec un sémaphore.
        // attendre avec un sémaphore que le client ait fermé les tubes
        
        while (!client_finish)
        {
            client_finish = is_client_finish(semIdCO);
        }

    }

    // attente de la fin des traitements en cours (via les sémaphores)
    my_op_wait_0(semIdSOC);
    my_op_wait_0(semIdCOC);
    my_op_wait_0(semIdSIC);

    // envoi à chaque service d'un code de fin
    write_int(fdsSOMME[1], -1);
    write_int(fdsCOMP[1], -1);
    write_int(fdsSIGMA[1], -1);

    // attente de la terminaison des processus services
    //3 wait car 3 processus fils (correspondant aux 3 services)
    int ret = wait(NULL);
    myassert(ret != -1, "Erreur : echec wait\n");
    ret = wait(NULL);
    myassert(ret != -1, "Erreur : echec wait\n");
    ret = wait(NULL);
    myassert(ret != -1, "Erreur : echec wait\n");

    //Fermeture des tubes anonymes
    close_pipes_ano(fdsSOMME, fdsCOMP, fdsSIGMA);

    // libération des ressources 
    //Finalisation de la configuration
    config_exit();

    //Destruction des tubes
    my_sem_dl(semIdCO, semIdSOC, semIdCOC, semIdSIC);
    my_unlink();
    
    return EXIT_SUCCESS;
}
