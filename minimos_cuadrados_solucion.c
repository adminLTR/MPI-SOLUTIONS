#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "mpi.h"

int main(int argc, char **argv) {
    int mi_id, numero_procesos;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &mi_id);
    MPI_Comm_size(MPI_COMM_WORLD, &numero_procesos);

    int n = 0; /* numero de puntos total */
    double *x_full = NULL, *y_full = NULL; /* solo usados por proceso 0 */
    int promedio_n, n_restante;

    /* Variables locales para cada proceso */
    int mis_puntos = 0;
    int desplazamiento = 0; /* índice inicial en el arreglo global */
    double *x_local = NULL, *y_local = NULL;

    MPI_Status status;
    MPI_Request req;

    /******************************
     * Paso 0: Proceso 0 lee el archivo y distribuye n
     ******************************/
    if (mi_id == 0) {
        FILE *archivo_entrada = fopen("datos_xy.txt", "r");
        if (!archivo_entrada) {
            fprintf(stderr, "Error abriendo archivo datos_xy.txt\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        if (fscanf(archivo_entrada, "%d", &n) != 1) {
            fprintf(stderr, "Error leyendo n desde archivo\n");
            fclose(archivo_entrada);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        /* leer todos los datos */
        x_full = (double *) malloc(n * sizeof(double));
        y_full = (double *) malloc(n * sizeof(double));
        for (int i = 0; i < n; ++i) {
            if (fscanf(archivo_entrada, "%lf %lf", &x_full[i], &y_full[i]) != 2) {
                fprintf(stderr, "Error leyendo punto %d\n", i);
                fclose(archivo_entrada);
                MPI_Abort(MPI_COMM_WORLD, 1);
            }
        }
        fclose(archivo_entrada);
    }

    /* Enviar el n a todos (no bloqueante). Los demás procesos hacen Irecv. */
    if (mi_id == 0) {
        /* proceso 0 envía n a todos (incluye a sí mismo, pero no es necesario) */
        for (int p = 1; p < numero_procesos; ++p) {
            MPI_Isend(&n, 1, MPI_INT, p, 100, MPI_COMM_WORLD, &req);
            MPI_Wait(&req, &status);
        }
    } else {
        MPI_Irecv(&n, 1, MPI_INT, 0, 100, MPI_COMM_WORLD, &req);
        MPI_Wait(&req, &status);
    }

    /* todos ahora conocen n */
    if (n <= 0) {
        if (mi_id == 0) fprintf(stderr, "n debe ser > 0\n");
        MPI_Finalize();
        return 0;
    }

    /* calcular reparto equilibrado: los primeros n_restante procesos obtienen q+1 */
    promedio_n = n / numero_procesos;     /* q */
    n_restante = n % numero_procesos;     /* r */
    /* Para cada proceso p:
     *   if (p < r) mis_puntos = q+1, desplazamiento = p*(q+1)
     *   else mis_puntos = q, desplazamiento = r*(q+1) + (p-r)*q
     * Esto equilibra la carga mejor que dar todo al ultimo proceso.
     */

    if (mi_id < n_restante) {
        mis_puntos = promedio_n + 1;
        desplazamiento = mi_id * mis_puntos;
    } else {
        mis_puntos = promedio_n;
        desplazamiento = n_restante * (promedio_n + 1) + (mi_id - n_restante) * promedio_n;
    }

    /* Todos reservan espacio para su porcion local */
    if (mis_puntos > 0) {
        x_local = (double *) malloc(mis_puntos * sizeof(double));
        y_local = (double *) malloc(mis_puntos * sizeof(double));
    } else {
        /* caso raro cuando numero_procesos > n */
        x_local = NULL;
        y_local = NULL;
    }

    /******************************
     * Paso 1: Proceso 0 distribuye subconjuntos a todos (no bloqueante)
     *
     * Estrategia:
     *  - Proceso 0 envía a cada proceso p su mis_puntos y luego su fragmento x,y.
     *  - Usamos Isend/Irecv y Wait/Waitall para experimentar con comunicaciones no bloqueantes.
     ******************************/
    if (mi_id == 0) {
        /* proceso 0 copia su propia porcion desde x_full/y_full */
        if (mis_puntos > 0) {
            for (int i = 0; i < mis_puntos; ++i) {
                x_local[i] = x_full[desplazamiento + i];
                y_local[i] = y_full[desplazamiento + i];
            }
        }

        /* enviar los fragmentos a los procesos 1..P-1 de forma no bloqueante */
        for (int p = 1; p < numero_procesos; ++p) {
            int p_mis_puntos, p_desplazamiento;
            if (p < n_restante) {
                p_mis_puntos = promedio_n + 1;
                p_desplazamiento = p * p_mis_puntos;
            } else {
                p_mis_puntos = promedio_n;
                p_desplazamiento = n_restante * (promedio_n + 1) + (p - n_restante) * promedio_n;
            }

            /* Enviar primero la cantidad p_mis_puntos (int) */
            MPI_Isend(&p_mis_puntos, 1, MPI_INT, p, 110, MPI_COMM_WORLD, &req);
            MPI_Wait(&req, &status);

            if (p_mis_puntos > 0) {
                /* Enviar el arreglo x y y (Isend + Wait) */
                MPI_Isend(&x_full[p_desplazamiento], p_mis_puntos, MPI_DOUBLE, p, 111, MPI_COMM_WORLD, &req);
                MPI_Wait(&req, &status);
                MPI_Isend(&y_full[p_desplazamiento], p_mis_puntos, MPI_DOUBLE, p, 112, MPI_COMM_WORLD, &req);
                MPI_Wait(&req, &status);
            }
        }

    } else {
        /* procesos distintos de 0 reciben su p_mis_puntos y luego los arreglos */
        MPI_Irecv(&mis_puntos, 1, MPI_INT, 0, 110, MPI_COMM_WORLD, &req);
        MPI_Wait(&req, &status);

        /* Ajustar si recibimos 0 puntos */
        if (mis_puntos > 0) {
            /* reasignar memoria si es distinto (por seguridad) */
            free(x_local);
            free(y_local);
            x_local = (double *) malloc(mis_puntos * sizeof(double));
            y_local = (double *) malloc(mis_puntos * sizeof(double));

            MPI_Irecv(x_local, mis_puntos, MPI_DOUBLE, 0, 111, MPI_COMM_WORLD, &req);
            MPI_Wait(&req, &status);
            MPI_Irecv(y_local, mis_puntos, MPI_DOUBLE, 0, 112, MPI_COMM_WORLD, &req);
            MPI_Wait(&req, &status);
        }
    }

    /* En este punto, cada proceso tiene su subconjunto local en x_local,y_local
     * y la variable mis_puntos contiene cuantos puntos tiene cada proceso.
     */

    /* Imprimir (opcional) los datos locales para verificar el reparto (descomentar si se desea) */
    /*
    printf("Proc %d: mis_puntos=%d desplazamiento=%d\n", mi_id, mis_puntos, desplazamiento);
    for (int ii=0; ii<mis_puntos; ++ii) {
        printf("  proc %d: x[%d]=%f y=%f\n", mi_id, ii, x_local[ii], y_local[ii]);
    }
    */

    /******************************
     * Paso 2: Cada proceso calcula sus sumas parciales
     ******************************/
    double miSUMAx = 0.0, miSUMAy = 0.0, miSUMAxy = 0.0, miSUMAxx = 0.0;
    for (int j = 0; j < mis_puntos; ++j) {
        double xv = x_local[j];
        double yv = y_local[j];
        miSUMAx += xv;
        miSUMAy += yv;
        miSUMAxy += xv * yv;
        miSUMAxx += xv * xv;
    }

    /* Liberar memoria local de los datos (opcional) */
    free(x_local);
    free(y_local);
    if (mi_id == 0) {
        free(x_full);
        free(y_full);
    }

    /******************************
     * Paso 3: Reducción de las sumas parciales usando un árbol binario
     *           (pairwise / recursive halving style)
     *
     * Implementación:
     *  - empaquetamos las 4 sumas en un vector double sums[4]
     *  - en cada paso 'step' los procesos cuyos rangos son múltiplos de 2*step
     *    esperan recibir desde rank+step (si existe), suman localmente y continúan.
     *  - los procesos que no son múltiplos de 2*step envían sus datos a rank-step y salen.
     *
     * Usamos comunicaciones no bloqueantes (Isend/Irecv + Wait) para experimentar.
     ******************************/
    double sums[4];
    sums[0] = miSUMAx;
    sums[1] = miSUMAy;
    sums[2] = miSUMAxy;
    sums[3] = miSUMAxx;

    int size = numero_procesos;
    int step = 1;

    while (step < size) {
        if ((mi_id % (2 * step)) == 0) {
            int src = mi_id + step;
            if (src < size) {
                double recvbuf[4];
                MPI_Request rreq;
                MPI_Irecv(recvbuf, 4, MPI_DOUBLE, src, 200 + step, MPI_COMM_WORLD, &rreq);
                MPI_Wait(&rreq, &status);
                /* acumular las sumas recibidas */
                for (int k = 0; k < 4; ++k) sums[k] += recvbuf[k];
            }
            /* si src >= size no hay emisor en este paso para este receptor */
        } else {
            /* proceso emisor: calcula el destino y manda, luego sale del bucle */
            int dst = mi_id - step;
            MPI_Request sreq;
            MPI_Isend(sums, 4, MPI_DOUBLE, dst, 200 + step, MPI_COMM_WORLD, &sreq);
            MPI_Wait(&sreq, &status);
            /* después de enviar, este proceso no participa en pasos superiores */
            break;
        }
        step *= 2;
    }

    /* Al final, el proceso 0 tiene las sumas totales en sums[] */
    if (mi_id == 0) {
        double SUMAx = sums[0];
        double SUMAy = sums[1];
        double SUMAxy = sums[2];
        double SUMAxx = sums[3];

        /* calcular pendiente e interseccion y y otros resultados */
        double pendiente = (SUMAx * SUMAy - n * SUMAxy) / (SUMAx * SUMAx - n * SUMAxx);
        double interseccion_y = (SUMAy - pendiente * SUMAx) / (double) n;

        printf("\nResultado (proceso 0):\n");
        printf("  n = %d, procesos = %d\n\n", n, numero_procesos);
        printf("  Pendiente (m) = %12.6f\n", pendiente);
        printf("  Intersección y (b) = %12.6f\n\n", interseccion_y);

        /* opcional: si queremos imprimir residuales, necesitamos los datos completos
         * que antes tenía el proceso 0 en x_full,y_full. Como los liberamos para ahorrar memoria,
         * podemos reabrir el archivo y volver a leer para mostrar los residuales.
         * (Esto es opcional y puede quitarse si no se desea.)
         */
        FILE *f = fopen("datos_xy.txt", "r");
        if (f) {
            int nn;
            fscanf(f, "%d", &nn);
            double xi, yi;
            printf("   Original (x,y)     Y estimado     Residual\n");
            printf("--------------------------------------------------\n");
            double suma_residual = 0.0;
            for (int i = 0; i < nn; ++i) {
                fscanf(f, "%lf %lf", &xi, &yi);
                double y_est = pendiente * xi + interseccion_y;
                double resid = yi - y_est;
                suma_residual += resid * resid;
                printf("   (%8.4lf, %8.4lf)    %12.6lf   %12.6lf\n", xi, yi, y_est, resid);
            }
            printf("--------------------------------------------------\n");
            printf("Suma residual = %12.6f\n", suma_residual);
            fclose(f);
        } else {
            printf("No se ha podido reabrir datos_xy.txt para mostrar residuales.\n");
        }
    }

    MPI_Finalize();
    return 0;
}