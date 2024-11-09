#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "orchestre_service.h"
#include "client_service.h"

#include "service_compression.h"

// définition éventuelle de types pour stocker les données


/*----------------------------------------------*
 * fonctions appelables par le service
 *----------------------------------------------*/

// fonction de réception des données
static void receiveData(/* fd_pipe_from_client, */ /* données à récupérer */)
{
    //de Leïla : la longueur de la chaîne de caractère est envoyée en premier (c'est toujours le cas)
}

// fonction de traitement des données
static void computeResult(/* données récupérées, */ /* résultat */)
{
}

// fonction d'envoi du résultat
static void sendResult(/* fd_pipe_to_client,*/ /* résultat */)
{
    //de Leïla : ne pas oublier d'envoyer (en premier) la longueur de la chaîne résultat
}


/*----------------------------------------------*
 * fonction appelable par le main
 *----------------------------------------------*/
void service_compression(/* fd tubes nommés avec un client */)
{
    // initialisations diverses
    
    receiveData(/* paramètres */);
    computeResult(/* paramètres */);
    sendResult(/* paramètres */);

    // libération éventuelle de ressources
}