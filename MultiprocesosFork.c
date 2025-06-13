#include <stdio.h>      //Entrada y salida estandar
#include <stdlib.h>     //Funciones generales como exit()
#include <unistd.h>     //fork(),sleep(), etc
#include <sys/wait.h>   //Para esperar procesos hijos 
#include <sys/time.h>   //Para medir el tiempo con gettimeofday()

#define GRUPOS8         //Numero total de grupos de transacciones (A-H)
#define TRANSACCIONES_POR_GRUPO //Numero de transacciones que hace cada grupo

//Funcion que simula una transaccion que toma tiempo



