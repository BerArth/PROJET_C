#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include "../UTILS/myassert.h"
#include "orchestre_service.h"
#include "client_service.h"

#include "service_compression.h"

// définition éventuelle de types pour stocker les données


/*----------------------------------------------*
 * fonctions appelables par le service
 *----------------------------------------------*/

// fonction de réception des données
static void receiveData(int pfc, int * len, char ** s)
{
    //de Leïla : la longueur de la chaîne de caractère est envoyée en premier (c'est toujours le cas)
    int ret = read(pfc, len, sizeof(int));
    myassert(ret != -1 ,"Erreur : Echec de la lecture dans le tube.");
    myassert(ret == sizeof(float) ,"Erreur : Données mal lues");

    *s = malloc(*len * sizeof(char));

    ret = read(pfc, *s, *len * sizeof(char));
    myassert(ret != -1 ,"Erreur : Echec de la lecture dans le tube.");
    myassert(ret == sizeof(float) ,"Erreur : Données mal lues");

}

// fonction de traitement des données
static void computeResult(char * input, char ** result)
{
    int i = 0;
    int n = strlen(input);
    char temp[20];
    int output_size = 1; 

    *result = (char*)malloc(output_size);

    (*result)[0] = '\0';

    while (i < n) {
        char current_char = input[i];
        int count = 0;


        while (i < n && input[i] == current_char) {
            count++;
            i++;
        }

        sprintf(temp, "%d%c", count, current_char);


        int temp_length = strlen(temp);
        output_size += temp_length;

        *result = (char*)realloc(*result, output_size);

        strcat(*result, temp);
    }

}

// fonction d'envoi du résultat
static void sendResult(int ptc, int len, const char * result)
{
    
    int ret = write(ptc, &len, sizeof(int));
    myassert(ret != -1 ,"Erreur : Echec de l'écriture dans le tube.");
    myassert(ret == sizeof(float) ,"Erreur : Données mal écrites");
    ret = write(ptc, result, sizeof(char) * len);
    myassert(ret != -1 ,"Erreur : Echec de l'écriture dans le tube.");
    myassert(ret == sizeof(float) ,"Erreur : Données mal écrites");

}


/*----------------------------------------------*
 * fonction appelable par le main
 *----------------------------------------------*/
void service_compression(const char * pipe_to_client, const char * pipe_from_client)
{
    // initialisations diverses

    int pfc = open(pipe_from_client, O_RDONLY);
    myassert(pfc != -1, "Erreur : Echec de l'ouveture du tube pipe_from_client");

    int ptc = open(pipe_to_client, O_WRONLY);
    myassert(ptc != -1, "Erreur : Echec de l'ouveture du tube pipe_to_client");

    char * s = NULL;
    int len = 0;
    char * result = NULL;

    receiveData(pfc, &len, &s);
    computeResult(s, &result);
    int len_result = strlen(result);
    sendResult(ptc, len_result, result);

    // libération éventuelle de ressources
}
