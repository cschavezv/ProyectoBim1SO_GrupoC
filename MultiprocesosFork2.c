#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>  // para fork() y sleep
#include <sys/ipc.h> // para memoria compartida y semáforos
#include <sys/shm.h>
#include <sys/sem.h>  // para semáforos
#include <sys/wait.h> // para wait()
#include <time.h>     // para rand y time
#include <sys/time.h> // para gettimeofday()

#define NUM_CAMARAS 8
#define IMAGENES_POR_CAMARA 5

// Unión para semctl (System V semáforos)
union semun
{
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

// Esta función simula el trabajo que hace una cámara cuando procesa una imagen.
// Simplemente muestra un mensaje para indicar qué cámara está trabajando y en qué imagen.
// Luego hace una pausa breve para simular que está usando la CPU.
void procesarImagen(int camara, int imagen)
{
    printf("Camara %d procesando imagen %d ...\n", camara, imagen);
    usleep(100000); // Pausa de 100 milisegundos para simular carga
}

// Esta función decide aleatoriamente si se detecta una cara en la imagen.
// Utiliza una probabilidad del 30% para simular detección exitosa.
int cara_detectada()
{
    // Se genera un número entre 0 y 9.
    // Si el número es menor que 3, se considera que se detectó una cara.
    return rand() % 10 < 3;
}

// Esta función es la que ejecuta cada proceso hijo que simula una cámara.
// Cada cámara procesa 5 imágenes y, cuando detecta una cara, incrementa
// un contador global de caras detectadas, el cual está en memoria compartida.
void procesarCamara(int camara, int *caras_detectadas, int semid)
{
    // Semilla única por proceso para que rand() no devuelva los mismos valores
    srand(time(NULL) ^ getpid());

    // Operaciones para semáforo
    struct sembuf p = {0, -1, 0}; // wait (decrementar semáforo)
    struct sembuf v = {0, 1, 0};  // signal (incrementar semáforo)

    for (int i = 0; i < IMAGENES_POR_CAMARA; i++)
    {
        // Primero se simula el procesamiento de la imagen
        procesarImagen(camara, i);

        // Luego se verifica si se detectó una cara en esta imagen
        if (cara_detectada())
        {
            printf("Camara %d detectó una cara en imagen %d\n", camara, i);

            // Aquí incrementamos el contador global de caras detectadas.
            // Es importante proteger este acceso con un semáforo para evitar
            // condiciones de carrera cuando varios procesos escriben a la vez.

            // Esperamos (wait) a que el semáforo esté libre para entrar a la sección crítica
            semop(semid, &p, 1);

            (*caras_detectadas)++;

            // Liberamos (signal) el semáforo para que otro proceso pueda entrar
            semop(semid, &v, 1);
        }
    }

    // Cuando se terminan de procesar todas las imágenes, se muestra un mensaje.
    printf("Camara %d terminó procesamiento\n", camara);
}

int main()
{
    struct timeval inicio, fin; // Para guardar el tiempo de inicio y fin

    // Guardamos el tiempo justo antes de comenzar el procesamiento
    gettimeofday(&inicio, NULL);

    // Creamos una memoria compartida para almacenar el contador de caras detectadas
    int shmid = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);

    // Verificamos si hubo error al crear la memoria compartida
    if (shmid == -1)
    {
        perror("Error creando memoria compartida");
        exit(1);
    }

    // Nos conectamos a la memoria compartida
    int *caras_detectadas = (int *)shmat(shmid, NULL, 0);

    // Inicializamos el contador en 0
    *caras_detectadas = 0;

    // Creamos un semáforo para controlar acceso a la memoria compartida
    int semid = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
    if (semid == -1)
    {
        perror("Error creando semáforo");
        // Limpiar memoria compartida antes de salir
        shmdt(caras_detectadas);
        shmctl(shmid, IPC_RMID, NULL);
        exit(1);
    }

    // Inicializamos el semáforo en 1 (disponible)
    union semun arg;
    arg.val = 1;
    if (semctl(semid, 0, SETVAL, arg) == -1)
    {
        perror("Error inicializando semáforo");
        shmdt(caras_detectadas);
        shmctl(shmid, IPC_RMID, NULL);
        semctl(semid, 0, IPC_RMID);
        exit(1);
    }

    // Creamos un proceso hijo por cada cámara
    for (int i = 0; i < NUM_CAMARAS; i++)
    {
        pid_t pid = fork(); // Creamos el proceso

        if (pid == -1)
        {
            perror("Error en fork");
            // Limpieza antes de salir
            shmdt(caras_detectadas);
            shmctl(shmid, IPC_RMID, NULL);
            semctl(semid, 0, IPC_RMID);
            exit(1);
        }

        if (pid == 0)
        {
            // Este código solo se ejecuta en el proceso hijo
            procesarCamara(i, caras_detectadas, semid);

            // Nos desconectamos de la memoria compartida
            shmdt(caras_detectadas);

            // Terminamos el proceso hijo
            exit(0);
        }
    }

    // Este código lo ejecuta solo el proceso padre

    // Esperamos a que todos los procesos hijos terminen
    for (int i = 0; i < NUM_CAMARAS; i++)
    {
        wait(NULL);
    }

    // Guardamos el tiempo cuando ya terminó todo
    gettimeofday(&fin, NULL);

    // Calculamos cuánto tiempo pasó en milisegundos
    double tiempo_total = (fin.tv_sec - inicio.tv_sec) * 1000.0; // segundos a milisegundos
    tiempo_total += (fin.tv_usec - inicio.tv_usec) / 1000.0;     // microsegundos a milisegundos

    // Mostramos cuántas caras se detectaron entre todas las cámaras
    printf("\nTotal de caras detectadas: %d\n", *caras_detectadas);

    // Mostramos cuánto tiempo tomó todo el trabajo
    printf("Tiempo total con procesos: %.2f ms\n", tiempo_total);

    // Nos desconectamos de la memoria compartida
    shmdt(caras_detectadas);

    // Liberamos la memoria compartida
    shmctl(shmid, IPC_RMID, NULL);

    // Liberamos el semáforo
    semctl(semid, 0, IPC_RMID);

    return 0;
}