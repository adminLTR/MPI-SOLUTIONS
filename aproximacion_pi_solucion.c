#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "mpi.h"

#define f(x) ((4.0/ (1.0 + (x)*(x))))
#define PI_REF (4.0 * atan(1.0))

int main(int argc, char *argv[])
{
    int tamaño, rango;
    int N;
    double ancho;
    double sumaLocal, sumaTotal;
    
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &tamaño);
    MPI_Comm_rank(MPI_COMM_WORLD, &rango);


    while (1) {
        /* solo el proceso 0 lee de stdin */
        if (rango == 0) {
            printf("Ingrese numero de intervalos de aproximacion:(0 para salir)\n");
            if (scanf("%d", &N) != 1) {
                N = 0; /* si hay error en la lectura, salir */
            }
        }

        /* mandar N a todos */
        MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);

        if (N <= 0) break; /* terminar si N==0 o lectura errónea */

        ancho = 1.0 / (double) N;

        /* repartir índices [0..N-1] entre los 'tamaño' procesos */
        int q = N / tamaño;        /* cociente */
        int r = N % tamaño;        /* resto */
        int local_n;               /* cuantos índices calcula este proceso */
        int inicio;                /* índice inicial (desde 0) */

        if (rango < r) {
            local_n = q + 1;
            inicio = rango * local_n;
        } else {
            local_n = q;
            inicio = r * (q + 1) + (rango - r) * q;
        }

        /* calcular suma local */
        sumaLocal = 0.0;
        for (int i = 0; i < local_n; ++i) {
            int idx = inicio + i; /* índice global */
            double x = ( (double)idx + 0.5 ) * ancho; /* punto medio */
            sumaLocal += f(x);
        }

        /* reducir sumas locales a la suma total en el proceso 0 */
        MPI_Reduce(&sumaLocal, &sumaTotal, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

        if (rango == 0) {
            double pi_approx = sumaTotal * ancho;
            double error = pi_approx - PI_REF;
            printf("N=%d procesos=%d pi_approx = %.12f error = %.12e\n",
                   N, tamaño, pi_approx, error);
        }
        /* aquí se repite el bucle: el proceso 0 pedirá otro N */
    }

    MPI_Finalize();
    return 0;
}
