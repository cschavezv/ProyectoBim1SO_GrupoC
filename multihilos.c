#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/time.h>

#define GRUPOS 8 //Grupos de la A hasta la H, cada grupo representa a un hilo
#define TRANSACCIONES_POR_GRUPO 5 //Cada hilo ejecutará 5 transacciones