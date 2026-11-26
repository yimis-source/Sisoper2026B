/*
 * Integrantes: Yimis Buenaventura Castro y Fernando José Lenis Núñez
 *
 * minips.c - Laboratorio final de Sistemas Operativos 2026B (USC)
 * Recorre /proc y muestra, para cada proceso, PID, PPID, nombre, estado,
 * memoria residente (VmRSS en KiB) y cambios de contexto voluntarios e
 * involuntarios. Al final imprime el total de procesos y la suma de RSS.
 *
 * Compilar: gcc -Wall -Wextra -std=gnu99 -o minips minips.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>

/* Datos del PCB de un proceso que se muestran en cada fila. */
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
    char ruta[64];
    char linea[256];
    FILE *f;
    int n, error_lectura;

    /* En cero: los hilos del kernel sin VmRSS quedan con rss_kb = 0. */
    memset(p, 0, sizeof *p);
    p->pid = pid;
    p->estado = '?';

    /* snprintf nunca desborda 'ruta'; si la ruta no cupiera, se descarta. */
    n = snprintf(ruta, sizeof ruta, "/proc/%d/status", pid);
    if (n < 0 || (size_t)n >= sizeof ruta)
        return -1;

    /* Si no abre, el proceso pudo terminar: se informa con -1. */
    f = fopen(ruta, "r");
    if (f == NULL)
        return -1;

    while (fgets(linea, sizeof linea, f) != NULL) {
        char c;
        long v;

        /* El nombre se copia completo porque puede tener espacios. */
        if (strncmp(linea, "Name:", 5) == 0) {
            const char *ini = linea + 5;
            size_t len;
            while (*ini == ' ' || *ini == '\t')
                ini++;
            len = strcspn(ini, "\n");
            if (len >= sizeof p->nombre)
                len = sizeof p->nombre - 1;
            memcpy(p->nombre, ini, len);
            p->nombre[len] = '\0';
        /* Los demas campos se extraen con sscanf (retorna 1 si coincide). */
        } else if (sscanf(linea, "State: %c", &c) == 1) {
            p->estado = c;
        } else if (sscanf(linea, "PPid: %ld", &v) == 1) {
            p->ppid = (int)v;
        } else if (sscanf(linea, "VmRSS: %ld", &v) == 1) {
            p->rss_kb = v;
        } else if (sscanf(linea, "voluntary_ctxt_switches: %ld", &v) == 1) {
            p->cs_vol = v;
        } else if (sscanf(linea, "nonvoluntary_ctxt_switches: %ld", &v) == 1) {
            p->cs_inv = v;
        }
    }

    /* fgets devuelve NULL al final del archivo y tambien ante un error.
     * Si el proceso murio despues del fopen, la lectura falla (ferror) y
     * nunca se leyo Name: se omite en vez de imprimir una fila vacia. */
    error_lectura = ferror(f);
    if (fclose(f) != 0)
        perror(ruta);
    if (error_lectura || p->nombre[0] == '\0')
        return -1;
    return 0;
}

int main(void)
{
    DIR *d;
    struct dirent *e;
    InfoProc p;
    long total = 0, rss_total = 0;
    int rc = 0;

    d = opendir("/proc");
    if (d == NULL) {
        perror("opendir /proc");
        return 1;
    }

    printf("%6s %6s %-16s %c %9s %8s %8s\n",
           "PID", "PPID", "NOMBRE", 'E', "RSS(KiB)", "VOL", "INV");

    for (;;) {
        /* readdir devuelve NULL al terminar y tambien ante un error;
         * solo errno los distingue, por eso se pone en 0 antes. */
        errno = 0;
        e = readdir(d);
        if (e == NULL) {
            if (errno != 0) {
                perror("readdir /proc");
                rc = 1;
            }
            break;
        }
        /* Solo las carpetas numericas son procesos. */
        if (!isdigit((unsigned char)e->d_name[0]))
            continue;
        /* Si el proceso desaparecio, se omite sin contarlo. */
        if (leer_proceso(atoi(e->d_name), &p) != 0)
            continue;
        printf("%6d %6d %-16.16s %c %9ld %8ld %8ld\n",
               p.pid, p.ppid, p.nombre, p.estado, p.rss_kb, p.cs_vol, p.cs_inv);
        total++;
        rss_total += p.rss_kb;
    }

    printf("Procesos: %ld | RSS total: %ld KiB\n", total, rss_total);

    if (closedir(d) != 0) {
        perror("closedir /proc");
        return 1;
    }
    return rc;
}
