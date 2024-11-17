#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include <assert.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>


#include "../UTILS/memory.h"
#include "../UTILS/myassert.h"
#include "../ORCHESTRE_SERVICE/orchestre_service.h"
#include "../CLIENT_SERVICE/client_service.h"
#include "service_sigma.h"



// définition éventuelle de types pour stocker les données
typedef struct{
    float * tab;
    int start, end;
    float *res;
    pthread_mutex_t *mutex;
}ThreadData;

/*----------------------------------------------*
 * fonctions appelables par le service
 *----------------------------------------------*/

// fonction de réception des données
static void receiveData(int pfc, int * nb_thread, int * size, float ** tab)
{
    int ret = read(pfc, nb_thread, sizeof(int));
    myassert(ret != -1, "Erreur : Echec de la lecture dans le tube");
    myassert(ret == sizeof(int), "Erreur : Données mal lues");

    ret = read(pfc, size, sizeof(int));
    myassert(ret != -1, "Erreur : Echec de la lecture dans le tube");
    myassert(ret == sizeof(int), "Erreur : Données mal lues");

    MY_MALLOC(*tab, float, *size);

    for(int i = 0; i < *size; i++){
        ret = read(pfc, tab[i], sizeof(float));
        myassert(ret != -1, "Erreur : Echec de la lecture dans le tube");
        myassert(ret == sizeof(float), "Erreur : Données mal lues");
    }

}

void * codeThread(void * arg){
    ThreadData * data = (ThreadData *)arg;
    float res_l = 0.0;

    for(int i = data->start; i < data->end; i++){
        res_l += data->tab[i];
    }

    int ret = pthread_mutex_lock(data->mutex);
    myassert(ret == 0, "Echec du vérouillage du mutex");
    *(data->res) += res_l;
    ret = pthread_mutex_unlock(data->mutex);
    myassert(ret == 0, "Echec du dévérouillage du mutex");

    return NULL;

}

// fonction de traitement des données
static void computeResult(int size, int nb_thread, float * tab, float * result)
{

    pthread_t tabId[nb_thread];
    ThreadData datas[nb_thread];
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

    float res = 0.0;
    int part = size/nb_thread;

    for(int i = 0; i<nb_thread; i++){
        datas[i].tab = tab;
        datas[i].start = i * part;
        if(i == nb_thread - 1){
            datas[i].end = size;
        }else{
            datas[i].end = (i + 1) * part;
        }
        datas[i].res = &res;
        datas[i].mutex =  &mutex;
    }

    for(int i = 0; i<nb_thread; i++){
        int ret = pthread_create(&(tabId[i]), NULL, codeThread, &(datas[i]));
        myassert(ret == 0, "Echec de la creation du thread");
    }

    for(int i = 0; i<nb_thread; i++){
        int ret = pthread_join(tabId[i], NULL);
        myassert(ret == 0, "Echec de l'attente de tout les threads");
    }

    int ret = pthread_mutex_destroy(&mutex);
    myassert(ret == 0, "Echec de la destruction du mutex");

    *result = res;

}

// fonction d'envoi du résultat
static void sendResult(int ptc, float result)
{

    int ret = write(ptc, &result, sizeof(float));
    myassert(ret != -1 ,"Erreur : Echec de l'écriture dans le tube.");
    myassert(ret == sizeof(float) ,"Erreur : Données mal ecrites");

}


/*----------------------------------------------*
 * fonction appelable par le main
 *----------------------------------------------*/
void service_sigma(int ptc, int pfc)
{
    // initialisations diversesœ
    
    int nb_thread, size;
    float * tab;
    float result = 0.0;

    receiveData(pfc, &nb_thread, &size, &tab);
    computeResult(size, nb_thread, tab, &result);
    sendResult(ptc, result);

    // libération éventuelle de ressources
    free(tab);
}
