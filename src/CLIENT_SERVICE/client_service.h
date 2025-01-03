#ifndef CLIENT_SERVICE_H
#define CLIENT_SERVICE_H

#include <sys/types.h>

// Ici toutes les communications entre les services et les clients :
// - les deux tubes nommés pour la communication bidirectionnelle

#define PIPE_SSOTC "pipe_s2c_0" //tube service somme -> client
#define PIPE_CTSSO "pipe_c2s_0" //tube client -> service somme

#define PIPE_SCTC "pipe_s2c_1" //tube service compression -> client
#define PIPE_CTSC "pipe_c2s_1" //tube client -> service compression

#define PIPE_SSITC "pipe_s2c_2" //tube service sigma -> client
#define PIPE_CTSSI "pipe_c2s_2" //tube client -> service sigma

void create_pipes_CS();
void open_pipes_CS(int isClient, const char* filenameSC, int* fd_STC, const char* filenameCS, int* fd_CTS);
void close_pipes_CS(int fd_STC, int fd_CTS);

#endif
