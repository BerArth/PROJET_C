#ifndef CLIENT_ORCHESTRE_H
#define CLIENT_ORCHESTRE_H

// Ici toutes les communications entre l'orchestre et les clients :
// - le sémaphore pour que 2 clients ne conversent pas en même
//   temps avec l'orchestre
// - les deux tubes nommés pour la communication bidirectionnelle


// fichier pour l'identification du sémaphore
#define FICHIER_CO "CLIENT_ORCHESTRE/client_orchestre.h"

// identifiant pour le deuxième paramètre de ftok
#define ID_CO 5


// les 2 tubes nommées
#define PIPE_OTC "pipe_o2c" //contenu dans le dossier src
#define PIPE_CTO "pipe_c2o" //contenu dans le dossier src


#endif
