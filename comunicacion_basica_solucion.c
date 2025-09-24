/*
 ============================================================================
 Name        : comunicacion_basica_solucion.c
 Compile     : mpicc -g comunicacion_basica_solucion.c -o comunicacion_basica_solucion.exe
 Run         : mpiexec  -n 4 ./comunicacion_basica_solucion
 ============================================================================
*/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "mpi.h"

int main(int argc, char **argv)
{
    char mensaje1[20], mensaje2[20];
    int i, rango, tamaño, tag1 = 1, tag2 = 2, etiqueta = 99;
    MPI_Status estado;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &tamaño);
    MPI_Comm_rank(MPI_COMM_WORLD, &rango);

    if (rango == 0)
    {
        strcpy(mensaje1, "Hola, 1er mensaje");
        strcpy(mensaje2, "Hola, 2do mensaje");
        for (i = 1; i < tamaño; i++)
        {
            MPI_Send(mensaje1, 20, MPI_CHAR, i, tag1, MPI_COMM_WORLD);
            MPI_Send(mensaje2, 20, MPI_CHAR, i, tag2, MPI_COMM_WORLD);
        }
        printf("nodo %d : %.20s\n", rango, mensaje1);
        printf("nodo %d : %.20s\n", rango, mensaje2);
        for (i = 1; i < tamaño; i++)
        {
            MPI_Recv(&rango, 1, MPI_INT, i, etiqueta, MPI_COMM_WORLD, &estado);
            printf("nodo %d : Hola de vuelta\n", rango);
        }
    }
    else
    {
        MPI_Recv(mensaje2, 20, MPI_CHAR, 0, tag2, MPI_COMM_WORLD, &estado);
        MPI_Recv(mensaje1, 20, MPI_CHAR, 0, tag1, MPI_COMM_WORLD, &estado);
        MPI_Send(&rango, 1, MPI_INT, 0, etiqueta, MPI_COMM_WORLD);
    }
    MPI_Finalize();
    return 0;
}
