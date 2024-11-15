#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>

#include "config.h"
#include "client_orchestre.h"
#include "orchestre_service.h"
#include "service.h"
#include "io.h"


static void usage(const char *exeName, const char *message)
{
    fprintf(stderr, "usage : %s <fichier config>\n", exeName);
    if (message != NULL)
        fprintf(stderr, "message : %s\n", message);
    exit(EXIT_FAILURE);
}

static void my_fork_exec(const int numService, const int semKey, const int fd_ano, const char* pipe_stc, const char* pipe_cts)
{
    pid_t pid;

    char* argv[7];

    argv[0] = "service";
    argv[1] = io_intToStr(numService);
    argv[2] = io_intToStr(semKey);
    argv[3] = io_intToStr(fd_ano);
    argv[4] = pipe_stc;
    argv[5] = pipe_cts;
    argv[6] = NULL;

    pid = fork();

    if(pid == 0)
    {
        execv(argv[0], argv);
    }
}

static void create_pipes_name(const char* filename1, const char* filename2)
{
    int ret = mkfifo(filename1, 0644);
    myassert(ret == 0, "Erreur : Echec de la création du tube");

    ret = mkfifo(filename2, 0644);
    myassert(ret == 0, "Erreur : Echec de la création du tube");
}

static int create_sem(const char* filename, int id_key, key_t* key)
{
    key_t* key;
    int semId;

    //Création de la clé
    *key = ftok(filename, id_key);
    myassert(*key != -1, "Erreur : Echec de la création de la clé IPC");

    //Récupération du sémaphore
    semId = semget(*key, 1, IPC_CREAT | IPC_EXCL | 0641);
    myassert(semId != -1, "Erreur : Echec de la récupération du sémaphore");

    return semId;
}

static int create_pipes_ano(int fds1[2], int fds2[2], int fds3[2])
{
    int ret = pipe(fds1);
    myassert(ret == 0, "Erreur : Echec de la création du tube anonyme");

    ret = pipe(fds2);
    myassert(ret == 0, "Erreur : Echec de la création du tube anonyme");

    ret = pipe(fds3);
    myassert(ret == 0, "Erreur : Echec de la création du tube anonyme");
}

//Fonction permettant d'ouvrir 2 tubes pour une communication bidirectionnelle
static void open_pipes(int* fd_rd, int* fd_wr, char* pipe_rd, char* pipe_wr)
{
    *fd_rd = open(pipe_rd, O_RDONLY);
    myassert(*fd_rd != -1, "Erreur : Echec de l'ouverture du tube");

    *fd_wr = open(pipe_wr, O_WRONLY);
    myassert(*fd_wr != -1, "Erreur : Echec de l'ouverture du tube");
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

int generate_number()
{
    int min = 10;
    int max = 20;

    return rand() % (max - min) + min;
}

int main(int argc, char * argv[])
{
    srand(time(NULL));


    if (argc != 2)
        usage(argv[0], "nombre paramètres incorrect");
    
    bool fin = false;

    // lecture du fichier de configuration
    config_init(argv[1]);

    int ret, ret_pid;

    pid_t pid;

    int fd_otc, fd_cto;

    int semIdCO, semIdSOC, semIdCOC, semIdSIC;
    int key;

    int fdsSOMME[2];
    int fdsCOMP[2];
    int fdsSIGMA[2];

    int password = generate_number();

    // Pour la communication avec les clients
    // - création de 2 tubes nommés pour converser avec les clients
    create_pipes_name(PIPE_OTC, PIPE_CTO);

    // - création d'un sémaphore pour que deux clients ne
    //   ne communiquent pas en même temps avec l'orchestre
    semIdCO = create_sem(FICHIER_CO, ID_CO, &key);
    
    // lancement des services, avec pour chaque service :
    my_fork_exec(dmdc, &key)

    // - création d'un tube anonyme pour converser (orchestre vers service)
    create_pipes_ano(fdsSOMME, fdsCOMP, fdsSIGMA);

    // - un sémaphore pour que le service prévienne l'orchestre de la
    //   fin d'un traitement
    semIdSOC = create_sem(FICHIER_SO, ID_SOMME);    
    semIdCOC = create_sem(FICHIER_SO, ID_COMP);    
    semIdSIC = create_sem(FICHIER_SO, ID_SIGMA);    

    // - création de deux tubes nommés (pour chaque service) pour les
    //   communications entre les clients et les services //voir client_service.h pr les constantes correspondantes aux tubes
    create_pipes_name(PIPE_SSOTC, PIPE_CTSSO);
    create_pipes_name(PIPE_SCTC, PIPE_CTSC);
    create_pipes_name(PIPE_SSITC, PIPE_CTSSI);


    while (! fin)
    {
        // ouverture ici des tubes nommés avec un client
        open_pipes(&fd_cto, &fd_otc, PIPE_CTO, PIPE_OTC);

        // attente d'une demande de service du client
        int dmdc = read_int(fd_cto);

        // détecter la fin des traitements lancés précédemment via
        // les sémaphores dédiés (attention on n'attend pas la
        // fin des traitement, on note juste ceux qui sont finis)


        // analyse de la demande du client
        // si ordre de fin
        //     envoi au client d'un code d'acceptation (via le tube nommé) //dans client.c j'ai mis -1 comme code d'acceptation
        //     marquer le booléen de fin de la boucle
        if(dmdc == SERVICE_ARRET)
        {
            write_int(fd_otc, -1);
            fin = true;
        }
        // sinon si service non ouvert
        //     envoi au client d'un code d'erreur (via le tube nommé) //-3
        else if(!config_isServiceOpen(dmdc))
        {
            write_int(fd_otc, -3);
        }
        // sinon si service déjà en cours de traitement
        //     envoi au client d'un code d'erreur (via le tube nommé) //-2 aussi dcp (tu peux définir autre chose tqt juste faudra que tu me dises que je change ds client.c)
        else if(true//avec le semaphore)
        {
            write_int(fd_otc, -2);
        }
        // sinon
        //     envoi au client d'un code d'acceptation (via le tube nommé)
        //     génération d'un mot de passe
        //     envoi d'un code de travail au service (via le tube anonyme)
        //     envoi du mot de passe au service (via le tube anonyme)
        //     envoi du mot de passe au client (via le tube nommé)
        //     envoi des noms des tubes nommés au client (via le tube nommé) //voir client_service.h, je pense que dcp les noms à envoyer seraient les constantes correspondantes aux tubes
                                                                            //dcp envoyer la taille de la chaîne de caractères avant
                                                                            //tube client -> service puis tube service -> client stp
        else
        {
            write_int(fd_otc, 0);

            write_int()
            write_int(fd_otc, password);




        }
        // finsi

        // attente d'un accusé de réception du client
        // fermer les tubes vers le client

        // il peut y avoir un problème si l'orchestre revient en haut de la
        // boucle avant que le client ait eu le temps de fermer les tubes
        // il faut attendre avec un sémaphore.
        // (en attendant on fait une attente d'1 seconde, à supprimer dès
        // que le sémaphore est en place)
        // attendre avec un sémaphore que le client ait fermé les tubes
        sleep(1);   // à supprimer
    }


    // attente de la fin des traitements en cours (via les sémaphores)

    // envoi à chaque service d'un code de fin

    // attente de la terminaison des processus services

    // libération des ressources
    
    return EXIT_SUCCESS;
}
