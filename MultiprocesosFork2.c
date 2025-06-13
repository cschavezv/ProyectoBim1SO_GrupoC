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

// Esta función simula una cámara de seguridad utilizando un proceso.
// Cada cámara procesa 5 imágenes y, si detecta una cara, incrementa el contador global.
// Este contador está en memoria compartida y protegido por un semáforo.
void procesarCamara(int camara, int *caras_detectadas, int semid)
{
    // Se inicializa una semilla única por proceso para evitar que todos generen los mismos números aleatorios
    srand(time(NULL) ^ getpid());

    // Definimos las operaciones para controlar el semáforo:
    struct sembuf p = {0, -1, 0}; // operación "wait" (bloquea)
    struct sembuf v = {0, 1, 0};  // operación "signal" (libera)

    // Cada cámara procesa 5 imágenes
    for (int i = 0; i < IMAGENES_POR_CAMARA; i++)
    {
        // Simulamos que la cámara está procesando la imagen
        procesarImagen(camara, i);

        // Verificamos si se detectó una cara
        if (cara_detectada())
        {
            // Antes de modificar el contador, se bloquea el semáforo para evitar condiciones de carrera
            semop(semid, &p, 1);

            // Se incrementa la variable compartida de caras detectadas
            (*caras_detectadas)++;

            // Se imprime información sobre la detección
            printf("Camara %d detectó cara en imagen %d (total: %d)\n", camara, i, *caras_detectadas);

            // Se libera el semáforo para que otros procesos puedan acceder a la variable compartida
            semop(semid, &v, 1);
        }
    }

    // Al finalizar el procesamiento de las imágenes, se indica que la cámara ha terminado
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