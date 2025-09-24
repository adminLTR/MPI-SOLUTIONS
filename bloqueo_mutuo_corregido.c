/*
 ============================================================================
 Name        : bloqueo_mutuo_corregido.c
 Compile     : mpicc -g bloqueo_mutuo_corregido.c -o bloqueo_mutuo_corregido.exe
 Run         : mpiexec  -n 2 ./bloqueo_mutuo_corregido
 ============================================================================
*/

#include <stdio.h>
#include "mpi.h"
#define LONGITUD_MENSAJE 2048 /* longitud del mensaje en elementos */
#define ETIQUETA_A 100
#define ETIQUETA_B 200

int main(int argc, char **argv)
{
    float mensaje1[LONGITUD_MENSAJE], /* buffers de mensajes                      */
        mensaje2[LONGITUD_MENSAJE];
    int rango,                           /* rango de tarea en comunicador            */
        destino, origen,                 /* rango en comunicador de destino          */
                                         /* y tareas origen                          */
        etiqueta_envio, etiqueta_recibo, /* etiquetas de mensaje                     */
        i;
    MPI_Status estado; /* estado de comunicacion                   */

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rango);
    printf(" Tarea %d inicializada\n", rango);

    /* inicializar buffers de mensaje */
    for (i = 0; i < LONGITUD_MENSAJE; i++)
    {
        mensaje1[i] = 100;
        mensaje2[i] = -100;
    }

    /* ---------------------------------------------------------------
     * cada tarea establece sus etiquetas de mensaje para el envio y recibo, ademas
     * del destino para el envio, y el origen para el recibo
     * --------------------------------------------------------------- */
    if (rango == 0)
    {
        destino = 1;
        origen = 1;
        etiqueta_envio = ETIQUETA_A;
        etiqueta_recibo = ETIQUETA_B;
    }
    else if (rango == 1)
    {
        destino = 0;
        origen = 0;
        etiqueta_envio = ETIQUETA_B;
        etiqueta_recibo = ETIQUETA_A;
    }

    /* ---------------------------------------------------------------
     * enviar y recibir mensajes
     * --------------------------------------------------------------- */
    if (rango == 0)
    {
        printf(" Tarea %d enviando mensaje...\n", rango);
        MPI_Send(mensaje1, LONGITUD_MENSAJE, MPI_FLOAT,
                 destino, etiqueta_envio, MPI_COMM_WORLD);

        MPI_Recv(mensaje2, LONGITUD_MENSAJE, MPI_FLOAT,
                 origen, etiqueta_recibo, MPI_COMM_WORLD, &estado);
        printf(" Tarea %d ha recibido el mensaje\n", rango);
    }
    else if (rango == 1)
    {
        printf(" Tarea %d esperando mensaje...\n", rango);
        MPI_Recv(mensaje2, LONGITUD_MENSAJE, MPI_FLOAT,
                 origen, etiqueta_recibo, MPI_COMM_WORLD, &estado);
        printf(" Tarea %d ha recibido el mensaje\n", rango);

        printf(" Tarea %d enviando mensaje ...\n", rango);
        MPI_Send(mensaje1, LONGITUD_MENSAJE, MPI_FLOAT,
                 destino, etiqueta_envio, MPI_COMM_WORLD);
    }

    MPI_Finalize();
    return 0;
}
