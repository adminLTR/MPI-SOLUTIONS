/******************************************************************************
 * ARCHIVO: latencia_mpi.c
 * DESCRIPCION:
 *   Programa de Medicion de Latencia MPI - Version C
 *   En este codigo de ejemplo, se realiza una prueba de tiempo de comunicacion MPI.
 *   La tarea MPI 0 enviara "repeticiones" numero de mensajes de 1 byte a la tarea MPI 1,
 *   esperando una respuesta entre cada repeticion. Se toman tiempos antes y despues
 *   para cada repeticion y se calcula un promedio cuando se completa.
 * AUTOR: Blaise Barney
 * ULTIMA REVISION: 04/13/05
 ******************************************************************************/
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#define NUMERO_REPETICIONES 1000

int main(int argc, char *argv[])
{
    int repeticiones,    /* numero de muestras por prueba */
        etiqueta,        /* parametro de etiqueta de mensaje MPI */
        numero_tareas,   /* numero de tareas MPI */
        rango,           /* mi numero de tarea MPI */
        destino, origen, /* designadores de tarea envio/recibo */
        promedio_T,      /* tiempo promedio por repeticion en microsegundos */
        codigo_retorno,  /* codigo de retorno */
        n;
    double T1, T2,     /* tiempos de inicio/fin por repeticion */
        suma_T,        /* suma de todos los tiempos de repeticiones */
        deltaT;        /* time for one rep */
    char msg;          /* buffer containing 1 byte message */
    MPI_Status status; /* MPI receive routine parameter */

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numero_tareas);
    MPI_Comm_rank(MPI_COMM_WORLD, &rango);
    if (rango == 0 && numero_tareas != 2)
    {
        printf("Number of tasks = %d\n", numero_tareas);
        printf("Only need 2 tasks - extra will be ignored...\n");
    }
    MPI_Barrier(MPI_COMM_WORLD);

    suma_T = 0;
    msg = 'x';
    etiqueta = 1;
    repeticiones = NUMERO_REPETICIONES;

    if (rango == 0)
    {
        /* round-trip latency timing test */
        printf("task %d has started...\n", rango);
        printf("Beginning latency timing test. Number of reps = %d.\n", repeticiones);
        printf("***************************************************\n");
        printf("Rep#       T1               T2            deltaT\n");
        destino = 1;
        origen = 1;
        for (n = 1; n <= repeticiones; n++)
        {
            T1 = MPI_Wtime(); /* start time */
            /* send message to worker - message tag set to 1.  */
            /* If return code indicates error quit */
            codigo_retorno = MPI_Send(&msg, 1, MPI_BYTE, destino, etiqueta, MPI_COMM_WORLD);
            if (codigo_retorno != MPI_SUCCESS)
            {
                printf("Send error in task 0!\n");
                MPI_Abort(MPI_COMM_WORLD, codigo_retorno);
                exit(1);
            }
            /* Now wait to receive the echo reply from the worker  */
            /* If return code indicates error quit */
            codigo_retorno = MPI_Recv(&msg, 1, MPI_BYTE, origen, etiqueta, MPI_COMM_WORLD,
                                      &status);
            if (codigo_retorno != MPI_SUCCESS)
            {
                printf("Receive error in task 0!\n");
                MPI_Abort(MPI_COMM_WORLD, codigo_retorno);
                exit(1);
            }
            T2 = MPI_Wtime(); /* end time */

            /* calculate round trip time and print */
            deltaT = T2 - T1;
            printf("%4d  %8.8f  %8.8f  %2.8f\n", n, T1, T2, deltaT);
            suma_T += deltaT;
        }
        promedio_T = (suma_T * 1000000) / repeticiones;
        printf("***************************************************\n");
        printf("\n*** Avg round trip time = %d microseconds\n", promedio_T);
        printf("*** Avg one way latency = %d microseconds\n", promedio_T / 2);
    }

    else if (rango == 1)
    {
        printf("task %d has started...\n", rango);
        destino = 0;
        origen = 0;
        for (n = 1; n <= repeticiones; n++)
        {
            codigo_retorno = MPI_Recv(&msg, 1, MPI_BYTE, origen, etiqueta, MPI_COMM_WORLD,
                                      &status);
            if (codigo_retorno != MPI_SUCCESS)
            {
                printf("Receive error in task 1!\n");
                MPI_Abort(MPI_COMM_WORLD, codigo_retorno);
                exit(1);
            }
            codigo_retorno = MPI_Send(&msg, 1, MPI_BYTE, destino, etiqueta, MPI_COMM_WORLD);
            if (codigo_retorno != MPI_SUCCESS)
            {
                printf("Send error in task 1!\n");
                MPI_Abort(MPI_COMM_WORLD, codigo_retorno);
                exit(1);
            }
        }
    }

    MPI_Finalize();
    exit(0);
}
