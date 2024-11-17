#ifndef ORCHESTRE_SERVICE_H
#define ORCHESTRE_SERVICE_H

#include <sys/types.h>

// Ici toutes les communications entre l'orchestre et les services :
// - le tube anonyme pour que l'orchestre envoie des données au service
// - le sémaphore pour que  le service indique à l'orchestre la fin
//   d'un traitement

#define FICHIER_SO "ORCHESTRE_SERVICE/orchestre_service.c"
#define ID_SOMME 1 //Identifiant clé sémaphore service somme - orchestre
#define ID_COMP 2 //Identifiant clé sémaphore service compression - orchestre
#define ID_SIGMA 3 //Identifiant clé sémaphore service sigma - orchestre

void create_sem_SO(int* semId_SSO, key_t* key_SSO, int* semId_SC, key_t* key_SC, int* semId_SSI, key_t* key_SSI);
void create_pipes_ano(int fds_SSO[2], int fds_SC[2], int fds_SSI[2]);
void close_pipes_ano(int fds_SSO[2], int fds_SC[2], int fds_SSI[2]);

#endif
