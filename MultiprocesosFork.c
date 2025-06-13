#include <stdio.h>      //Entrada y salida estandar
#include <stdlib.h>     //Funciones generales como exit()
#include <unistd.h>     //fork(),sleep(), etc
#include <sys/wait.h>   //Para esperar procesos hijos 
#include <sys/time.h>   //Para medir el tiempo con gettimeofday()

#define GRUPOS8         //Numero total de grupos de transacciones (A-H)
#define TRANSACCIONES_POR_GRUPO //Numero de transacciones que hace cada grupo

//Funcion que simula una transaccion que toma tiempo

int main()
{
    struct timeval inicio, fin;  // Para guardar el tiempo de inicio y fin
    gettimeofday(&inicio, NULL); // Guardar el tiempo inicial

    // Crear un proceso por cada grupo
    for (int i = 0; i < GRUPOS; ++i)
    {
        pid_t pid = fork(); // Crear un proceso hijo

        if (pid == 0)
        {
            // Código del proceso hijo
            procesarGrupo(i); // El hijo atiende su grupo
            exit(0);          // Terminar el hijo cuando acaba
        }
    }

    // Código del proceso padre: espera a todos los hijos
    for (int i = 0; i < GRUPOS; ++i)
        wait(NULL); // Esperar que termine cada proceso hijo

    gettimeofday(&fin, NULL); // Guardar el tiempo final

    // Calcular duración total en milisegundos
    double tiempo = (fin.tv_sec - inicio.tv_sec) * 1000.0; // segundos a milisegundos
    tiempo += (fin.tv_usec - inicio.tv_usec) / 1000.0;     // microsegundos a milisegundos

    printf("Tiempo total con procesos: %.2f ms\n", tiempo); // Mostrar duración total

    return 0;
}
