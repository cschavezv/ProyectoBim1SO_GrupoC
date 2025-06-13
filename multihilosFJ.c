#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>

#define NUM_CAMARAS 8 //Cámaras de la A hasta la H, cada cámara va a representa a un hilo
#define IMAGENES_POR_CAMARAS 5 //Cada hilo/cámara tratará de detectar 5 caras
int caras_detectadas = 0; //Contador para todas las caras detectadas por la camara


pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; //iniciamos la variable mutex (herramienta de sincronizacion)

void procesarImagen(int camara, int imagen){
    /*Se mostrara que la cámara está procesando una imagen, esto se realiza para poder ver el flujo del trabajo*/
    printf("Camara %d procesando imagen %d ...\n", camara, imagen);

    /*Se hara una pausa de 100 ms (Milisegundos) para simular el uso del CPU
    la pausa se escribira en 100000 microsegundos que es igual a 100 ms,
    se lo hace en microsegundos por que C solo tiene la funcion para sleep en segundos "sleep" y
    microsegundos "usleep", entonces si queremos trabajar en milisegundos, debemos usar microsegundos
    para no*/
    usleep(100000); 
}

int cara_detectada(){
    /*Va retornar un numero random entre 0 a 9 para simular que se encontro una cara, 
    es decir si el numero random es menor que 3 (0,1,2) se devolverá 1, señalando una detección facial exitosa,
    por otro lado si no es menor que 3, se devolvera 0, indicando una deteccion facil fallida.*/
    return rand() % 10 < 3;
}

void* procesarCamara(void* arg){
    /*El argumento es un puntero que se va a convertir a entero para saber qué número de cámara (int) es, se usa
    (size_t) para evitar advertencias de conversion*/
    int camara = (int)(size_t)arg;

    /*Con un for se va ir recorriendo todas las imagenes que debe procesar la camara (5 en total como se lo establecio 
    en la variable global)*/

    for (int i = 0; i < IMAGENES_POR_CAMARAS; i++){

        //Se empieza por las simulaciones, primero se va a simular que la camara esta procesando la imagen actual

        procesarImagen(camara, i);

        //Luego de procesar la imagen se detecta de manera aleatoria si se detectó una cara con la ayuda de un "if"

        if(cara_detectada){

            /*Antes de que se modifique la variable (caras_detectadas), se necesita que un solo hilo a la vez acceda
            a ella, para evitar que varios hilos lleguen y modifiquen el valor de manera simultanea, ya que, eso genera
            errores, por esa razon se bloquea el mutex.*/

            phtread_mutex_lock(&mutex);

            //se incrementa el contador de las caras detectadas

            caras_detectadas++;

            /*Se imprime un mensaje que indica cual camara detectó la cara, en que imagen se detecto la cara y cuantas
            caras se han detectado en total*/

            printf("Camara %d detectó cara en imagen %d (total: %d)\n",camara,i,caras_detectadas);

            /*Una vez ya se modifico la variable, se desbloquea el mutex para que los otros hilos puedan acceder
            al contador*/

            pthread_mutex_unlock(&mutex);

        }

    }

    /*Finalmente, se retorna NULL por que la funcion debe devolver un puntero void*,
    para cumplir con lo que requiere el pthread_create*/

    return NULL;

}


