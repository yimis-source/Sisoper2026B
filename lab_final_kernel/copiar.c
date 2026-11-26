/*
 * Integrantes: Yimis Buenaventura Castro y Fernando José Lenis Núñez
 *
 * copiar.c - Bono del laboratorio final de Sistemas Operativos 2026B (USC)
 * Copia 'origen' en 'destino' usando solo open, read, write y close, con un
 * bufer de 'tam_buffer' bytes reservado con malloc. Maneja lecturas y
 * escrituras parciales y verifica los retornos.
 *
 * Uso:      ./copiar origen destino tam_buffer [fsync]
 *           Con el cuarto argumento "fsync" se llama a fsync antes de cerrar.
 * Compilar: gcc -Wall -Wextra -std=gnu99 -o copiar copiar.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

int main(int argc, char *argv[])
{
    int fd_o, fd_d, usar_fsync, rc = 0;
    size_t tam;
    unsigned long valor;
    char *fin, *buf;
    long nlect = 0, nescr = 0;
    ssize_t n;
    struct stat st_o, st_d;

    /* 1. Argumentos: 3 obligatorios y un cuarto opcional que solo puede ser "fsync". */
    if (argc < 4 || argc > 5 || (argc == 5 && strcmp(argv[4], "fsync") != 0)) {
        fprintf(stderr, "Uso: %s origen destino tam_buffer [fsync]\n", argv[0]);
        return 1;
    }
    usar_fsync = (argc == 5);

    /* tam_buffer debe ser un entero positivo escrito completo: se rechazan
     * "12abc" (sobra texto), "-1" (strtoul lo convertiria en un numero
     * enorme) y los valores fuera de rango (errno == ERANGE). */
    errno = 0;
    valor = strtoul(argv[3], &fin, 10);
    if (errno != 0 || fin == argv[3] || *fin != '\0' || argv[3][0] == '-' || valor == 0) {
        fprintf(stderr, "tam_buffer invalido: '%s' (debe ser un entero mayor que 0)\n", argv[3]);
        return 1;
    }
    tam = (size_t)valor;

    /* 2. Bufer y apertura del origen. */
    buf = malloc(tam);
    if (buf == NULL) {
        perror("malloc");
        return 1;
    }
    fd_o = open(argv[1], O_RDONLY);
    if (fd_o < 0) {
        perror("open origen");
        free(buf);
        return 1;
    }

    /* 3. Si el destino ya existe y es el MISMO archivo que el origen (mismo
     *    dispositivo e inodo, aunque tenga otro nombre o sea un enlace duro),
     *    O_TRUNC lo vaciaria antes de leerlo y se perderian los datos. */
    if (fstat(fd_o, &st_o) == 0 && stat(argv[2], &st_d) == 0 &&
        st_o.st_dev == st_d.st_dev && st_o.st_ino == st_d.st_ino) {
        fprintf(stderr, "origen y destino son el mismo archivo\n");
        close(fd_o);
        free(buf);
        return 1;
    }

    fd_d = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_d < 0) {
        perror("open destino");
        close(fd_o);
        free(buf);
        return 1;
    }

    /* 4. Bucle de copia. read devuelve los bytes leidos (pueden ser menos que
     *    tam: lectura parcial), 0 al final del archivo o -1 si hay error. */
    for (;;) {
        ssize_t hecho = 0;
        n = read(fd_o, buf, tam);
        nlect++;
        if (n < 0) {
            if (errno == EINTR)
                continue;               /* interrumpida por una senal: reintentar */
            perror("read");
            rc = 1;
            break;
        }
        if (n == 0)
            break;                      /* fin de archivo */

        /* write tambien puede escribir menos de lo pedido (escritura
         * parcial): se repite con lo que falta hasta escribir los n bytes. */
        while (hecho < n) {
            ssize_t w = write(fd_d, buf + hecho, (size_t)(n - hecho));
            nescr++;
            if (w < 0) {
                if (errno == EINTR)
                    continue;
                perror("write");
                rc = 1;
                break;
            }
            hecho += w;
        }
        if (rc)
            break;
    }

    /* 5. fsync opcional: obliga a llevar al disco los datos que estan en la
     *    cache de paginas y espera a que termine. */
    if (rc == 0 && usar_fsync && fsync(fd_d) < 0) {
        perror("fsync");
        rc = 1;
    }

    /* 6. Liberar recursos; close tambien puede reportar errores de escritura. */
    if (close(fd_d) < 0) {
        perror("close destino");
        rc = 1;
    }
    if (close(fd_o) < 0) {
        perror("close origen");
        rc = 1;
    }
    free(buf);

    /* Numero de llamadas read/write, para la tabla del bono. */
    if (rc == 0)
        fprintf(stderr, "lecturas=%ld escrituras=%ld\n", nlect, nescr);
    return rc;
}
