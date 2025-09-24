/*
 ============================================================================
 Name        : comunicacion_colectiva_solucion.c
 Compile     : mpicc -g comunicacion_colectiva_solucion.c -o comunicacion_colectiva_solucion.exe -lm
 Run         : mpiexec  -n 8 ./comunicacion_colectiva_solucion
 ============================================================================
*/

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <math.h>
#include "mpi.h"

void ObtenerEstadisticas(float *numeros_aleatorios, int n, float *valor_resultado);

int main(int argc, char **argv)
{
    int numero_tareas, id_tarea, ii;
    unsigned int semilla;
    float numero_aleatorio[5], suma, valor_medio, numeros[100], valores_resultado[2];

    MPI_Status estado;
    FILE *archivo;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &id_tarea);
    MPI_Comm_size(MPI_COMM_WORLD, &numero_tareas);

    archivo = fopen("datos_salida.data", "w");

    if (id_tarea == 0)
    {
        printf("Ingrese un numero aleatorio como entero positivo\n");
        scanf("%u", &semilla);
    }

    MPI_Bcast(&semilla, 1, MPI_UNSIGNED, 0, MPI_COMM_WORLD);

    srand(semilla + id_tarea);
    numero_aleatorio[0] = 100. * (float)rand() / (float)RAND_MAX;

    printf("\nTarea %d despues del broadcast: semilla = %u : aleatorio = %8.3f", id_tarea, semilla, numero_aleatorio[0]);

    MPI_Reduce(&numero_aleatorio[0], &suma, 1, MPI_FLOAT, MPI_SUM, numero_tareas - 1, MPI_COMM_WORLD);

    if (id_tarea == (numero_tareas - 1))
    {
        valor_medio = suma / (float)numero_tareas;
        printf("\nTarea %d calcula el valor medio: suma = %8.3f - media = %8.3f", id_tarea, suma, valor_medio);
        fprintf(archivo, "Para semilla = %d valor medio = %10.3f\n", semilla, valor_medio);
    }

    for (ii = 1; ii < 5; ii++)
    {
        numero_aleatorio[ii] = 100. * (float)rand() / (float)RAND_MAX;
    }

    // Uso de Gather + Bcast

    printf("\nTarea %d (1:4) = ", id_tarea);
    for (ii = 1; ii < 5; ii++)
    {
        printf(" %8.3f", numero_aleatorio[ii]);
    }

    MPI_Gather(&numero_aleatorio[1], 4, MPI_FLOAT, numeros, 4, MPI_FLOAT, numero_tareas - 1, MPI_COMM_WORLD);

    if (id_tarea == (numero_tareas - 1))
    {
        printf("\nTarea %d listando los numeros:", id_tarea);
        for (ii = 0; ii < numero_tareas * 4; ii++)
        {
            printf(" %8.3f", numeros[ii]);
        }

        ObtenerEstadisticas(numeros, numero_tareas * 4, valores_resultado);
        fprintf(archivo, " (Max, Desv.Est) = %10.3f%10.3f\n", valores_resultado[0], valores_resultado[1]);
    }

    MPI_Bcast(valores_resultado, 2, MPI_FLOAT, numero_tareas - 1, MPI_COMM_WORLD);
    printf("\n en tarea %d (Max, Desv.Est.) = %10.3f%10.3f\n", id_tarea, valores_resultado[0], valores_resultado[1]);

    // Uso de Allgather
    MPI_Allgather(valores_resultado, 2, MPI_FLOAT, numeros, 2, MPI_FLOAT, MPI_COMM_WORLD);

    printf("\nTarea %d (0:1) = ", id_tarea);
    for (ii = 0; ii < numero_tareas * 2; ii++)
    {
        printf(" %8.3f", numeros[ii]);
    }

    printf("\n");

    if (id_tarea == (numero_tareas - 1))
    {
        fprintf(archivo, " (Max, Desv.Est.) = %10.3f%10.3f\n", numeros[0], numeros[1]);
    }

    fclose(archivo);
    MPI_Finalize();

    return 0;
}

void ObtenerEstadisticas(float *numeros_aleatorios, int N, float *datos_salida)
{
    float suma, valor_medio, desv_estandar;
    int ii;

    suma = 0.;
    *datos_salida = 0.;

    for (ii = 0; ii < N; ii++)
    {
        suma += *(numeros_aleatorios + ii);
        if (*(numeros_aleatorios + ii) > *datos_salida)
            *datos_salida = *(numeros_aleatorios + ii);
    }

    valor_medio = suma / (float)N;
    desv_estandar = 0.;
    for (ii = 0; ii < N; ii++)
        desv_estandar += (*(numeros_aleatorios + ii) - valor_medio) * (*(numeros_aleatorios + ii) - valor_medio);

    *(datos_salida + 1) = sqrt(desv_estandar / (float)N);
}