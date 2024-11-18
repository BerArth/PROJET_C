/*
 * Indications (à respecter) :
 * - Les erreurs sont gérées avec des assert ; les erreurs traitées sont :
 *    . appel trop tôt ou trop tard d'une méthode (cf. config.h)
 *    . fichier de configuration inaccessible
 *    . une position erronée
 * - Le fichier (si on arrive à l'ouvrir) est considéré comme bien
 *   formé sans qu'il soit nécessaire de le vérifier
 *
 * Un code minimal est fourni et permet d'utiliser le module "config" dès
 * le début du projet ; il faudra le remplacer par l'utilisation du fichier
 * de configuration.
 * Il est inutile de faire plus que ce qui est demandé
 *
 * Dans cette partie vous avez le droit d'utiliser les entrées-sorties
 * de haut niveau (fopen, fgets, ...)
 */


// TODO include des .h système
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#include "../UTILS/myassert.h"
#include "../UTILS/io.h"

#include "config.h"

// TODO Définition des données ici
static char * name;
static bool is_init = false;
static bool is_exit = false;
static bool *services_open = NULL;
static int nb_services = 0;
char name_tmp[100] = {0};

void config_init(const char *filename)
{
    // TODO erreur si la fonction est appelée deux fois

    myassert(!is_init, "Config_init a deja ete appele");
    myassert(!is_exit, "Config_exit a deja ete appele");

    // TODO code vide par défaut, à remplacer
    //      il faut lire le fichier et stocker toutes les informations en
    //      mémoire

    FILE * fd = fopen(filename, "r");
    myassert(fd != NULL, "Erreur lors de l'ouverture du fichier config");

    char line[100];
    int nb_line = 0;

    while (fgets(line, sizeof(line), fd) != NULL)
    {
        
        nb_line ++;

        if(line[0] == '#')
        {
            break;
        }

        if(nb_line == 1)
        {
            nb_services = atoi(line);
            services_open = malloc(nb_services * sizeof(bool));
        }

        if(nb_line == 2)
        {
            
            strcpy(name_tmp, line);

            int len = strlen(name_tmp) -1 ;
            name = malloc(len * sizeof(char));

            strcpy(name, name_tmp);

            name[strcspn(name, "\n")] = '\0';

        }

        if(nb_line > 2 && nb_line <= 2 + nb_services)
        {
            int i = nb_line - 3;
            services_open[i] = (strstr(line, "ouvert") != NULL);
        }

    }

    fclose(fd);
    is_init = true;

}

void config_exit()
{
    // TODO erreur si la fonction est appelée avant config_init

    myassert(is_init , "Config_init n'a pas encore été appelé");

    // TODO code vide par défaut, à remplacer
    //      libération des ressources

    free(services_open);
    services_open = NULL;
    free(name);
    is_init = false;
    is_exit = true;
}

int config_getNbServices()
{
    // erreur si la fonction est appelée avant config_init
    // erreur si la fonction est appelée après config_exit

    myassert(is_init, "Config_init n'a pas encore été appelé");
    myassert(!is_exit, "Config_exit a deja ete appele");

    // code par défaut, à remplacer
    return nb_services;
}

const char * config_getExeName()
{
    // TODO erreur si la fonction est appelée avant config_init
    // TODO erreur si la fonction est appelée après config_exit

    myassert(is_init, "Config_init n'a pas encore été appelé");
    myassert(!is_exit, "Config_exit a deja ete appele");

    // TODO code par défaut, à remplacer
    return name;
}

bool config_isServiceOpen(int pos)
{
    // TODO erreur si la fonction est appelée avant config_init
    // TODO erreur si la fonction est appelée après config_exit
    // TODO erreur si "pos" est incorrect

    myassert(is_init, "Config_init n'a pas encore été appelé");
    myassert(!is_exit, "Config_exit a deja ete appele");
    myassert(pos >= 0 && pos <= nb_services, "La position rechercher n'est pas correcte");

    // TODO code par défaut, à remplacer
    return services_open[pos];
}
