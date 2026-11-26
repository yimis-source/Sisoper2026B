/* minips.c -- Laboratorio final de Sistemas Operativos 2026B
 * Integrantes: [Nombre 1] y [Nombre 2]
 * Compilar: gcc -Wall -Wextra -std=gnu99 -o minips minips.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>

typedef struct {
    int pid, ppid;
    char nombre[64];
    char estado;
    long rss_kb;
    long cs_vol, cs_inv;
} InfoProc;

/* Lee /proc/<pid>/status y llena *p. Retorna 0 si pudo, -1 si el proceso ya no existe. */
static int leer_proceso(int pid, InfoProc *p)
{
    (void)pid; (void)p;   /* borre estas dos lineas al implementar la funcion */
    /* TODO 1: poner todo en cero (memset) y guardar el pid
     * TODO 2: construir la ruta /proc/<pid>/status con snprintf y abrirla con fopen
     *         (si falla, retornar -1: el proceso pudo terminar)
     * TODO 3: leer linea por linea (fgets) y extraer con sscanf los campos:
     *         Name, State, PPid, VmRSS, voluntary_ctxt_switches, nonvoluntary_ctxt_switches
     * TODO 4: cerrar el archivo y retornar 0 */
    return -1;
}

int main(void)
{
    /* TODO 5: abrir /proc con opendir y verificar el retorno
     * TODO 6: imprimir el encabezado; luego, por cada entrada (readdir) cuyo nombre
     *         empiece con digito, llamar a leer_proceso(atoi(nombre), &p) e imprimir una fila
     * TODO 7: acumular el total de procesos y de RSS; imprimir el resumen
     * TODO 8: cerrar el directorio */
    return 0;
}
