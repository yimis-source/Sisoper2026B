
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>


static int escribir_todo(int fd, const char *buf, size_t n, long *llamadas)
{
    size_t hecho = 0;

    while (hecho < n) {
        ssize_t w = write(fd, buf + hecho, n - hecho);
        (*llamadas)++;
        if (w == -1) {
            if (errno == EINTR)
                continue;           
            return -1;
        }
        hecho += (size_t)w;        
    }
    return 0;
}

int main(int argc, char *argv[])
{
    const char *origen, *destino;
    char *fin_num;
    long tam;
    int usar_fsync = 0;
    int fd_in = -1, fd_out = -1;
    char *buf = NULL;
    long n_read = 0, n_write = 0;
    long long total = 0;
    int estado = EXIT_FAILURE;
    struct stat st_in, st_out;

    
    if (argc < 4 || argc > 5 || (argc == 5 && strcmp(argv[4], "fsync") != 0)) {
        fprintf(stderr, "Uso: %s origen destino tam_buffer [fsync]\n", argv[0]);
        return EXIT_FAILURE;
    }
    origen = argv[1];
    destino = argv[2];
    usar_fsync = (argc == 5);

    errno = 0;
    tam = strtol(argv[3], &fin_num, 10);
    if (errno != 0 || fin_num == argv[3] || *fin_num != '\0' || tam <= 0) {
        fprintf(stderr, "tam_buffer invalido: '%s' (debe ser un entero > 0)\n", argv[3]);
        return EXIT_FAILURE;
    }

    
    fd_in = open(origen, O_RDONLY);
    if (fd_in == -1) {
        perror(origen);
        return EXIT_FAILURE;
    }

    
    if (stat(origen, &st_in) == 0 && stat(destino, &st_out) == 0 &&
        st_in.st_dev == st_out.st_dev && st_in.st_ino == st_out.st_ino) {
        fprintf(stderr, "'%s' y '%s' son el mismo archivo (inodo %lu)\n",
                origen, destino, (unsigned long)st_in.st_ino);
        goto fin;
    }

    fd_out = open(destino, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_out == -1) {
        perror(destino);
        goto fin;
    }

    
    buf = malloc((size_t)tam);
    if (buf == NULL) {
        perror("malloc");
        goto fin;
    }

    
    for (;;) {
        ssize_t r = read(fd_in, buf, (size_t)tam);
        n_read++;
        if (r == -1) {
            if (errno == EINTR)
                continue;
            perror("read");
            goto fin;
        }
        if (r == 0)
            break;                              
        if (escribir_todo(fd_out, buf, (size_t)r, &n_write) == -1) {
            perror("write");
            goto fin;
        }
        total += r;
    }

    
    if (usar_fsync && fsync(fd_out) == -1) {
        perror("fsync");
        goto fin;
    }

    estado = EXIT_SUCCESS;

fin:
    
    free(buf);
    if (fd_out != -1 && close(fd_out) == -1) {
        perror("close destino");
        estado = EXIT_FAILURE;
    }
    if (fd_in != -1 && close(fd_in) == -1) {
        perror("close origen");
        estado = EXIT_FAILURE;
    }

    if (estado == EXIT_SUCCESS)
        fprintf(stderr, "bytes=%lld buffer=%ld read()=%ld write()=%ld fsync=%s\n",
                total, tam, n_read, n_write, usar_fsync ? "si" : "no");
    return estado;
}

/* =====================================================================
 * RESPUESTAS A TODAS LAS PREGUNTAS DE LA GUIA
 * =====================================================================
 *
 * ---------------------------------------------------------------------
 * E1 - PREGUNTAS DE ANALISIS (minips)
 * ---------------------------------------------------------------------
 *
 * 1. ¿Que estado predomina? ¿Por que casi ninguno esta en R?
 *
 *    En mi salida el estado que mas sale es S (191 procesos), despues I
 *    (145, que son hilos del kernel como los kworker). En R solo vi 2, y
 *    uno era el mismo minips. Tiene sentido porque en un nucleo solo puede
 *    correr un proceso a la vez, y casi todos los demas estan esperando
 *    algo (el teclado, la red, un temporizador), asi que quedan dormidos
 *    hasta que llegue lo que esperan.
 *
 * 2. ¿Que procesos tienen mas cambios involuntarios y cuales mas
 *    voluntarios?
 *
 *    Los que mas involuntarios tienen son Hyprland (12963), zen-bin (9090)
 *    y los procesos "Isolated Web Co" del navegador. Son los que mas usan
 *    CPU (dibujar la pantalla, ejecutar JavaScript), entonces el
 *    planificador los saca cuando se les acaba el quantum. Eso es lo tipico
 *    de un proceso limitado por CPU.
 *
 *    En voluntarios el primero es pipewire-pulse (529866), que es el
 *    servidor de audio. Se la pasa bloqueandose para esperar el siguiente
 *    bloque de sonido, o sea que es un proceso limitado por E/S. Hyprland y
 *    el navegador tambien tienen muchos voluntarios porque esperan eventos
 *    del usuario.
 *
 * 3. Hijos del laboratorio IPC en el listado
 *
 *    Mi programa del laboratorio se llama "main" y no "lab_ipc". Al
 *    correrlo aparecieron 4 procesos main: el padre (11293) y sus tres
 *    hijos (11295, 11296, 11297). El PPID del padre salio 1 porque lo lance
 *    en segundo plano.
 *
 *     11293      1 main             S      1684        1        0
 *     11295  11293 main             S      1404        5        0
 *     11296  11293 main             S      1476        4        0
 *     11297  11293 main             S      1600        5        0
 *
 *    Al principio los cuatro estaban en S. El productor duerme con sleep(1)
 *    entre envios y los consumidores se bloquean en read hasta que llega
 *    algo al FIFO. Se ve en que los VOL suben de a uno cada segundo.
 *
 *    Un segundo despues dos hijos ya estaban en Z (zombi) con RSS 0:
 *
 *     11296  11293 main             Z         0        6        0
 *     11297  11293 main             Z         0        7        0
 *
 *    Ya habian terminado, pero el padre seguia en waitpid(pid_prod, ...)
 *    esperando al productor y todavia no habia recogido su estado de
 *    salida. Cuando el padre llega a los otros waitpid, los zombis
 *    desaparecen.
 *
 * 4. ¿Por que la suma de VmRSS sobrestima la memoria?
 *
 *    La suma me dio 13 503 684 KiB (unos 13 GB), pero "free -m" marcaba
 *    9400 MiB usados. La diferencia sale de que VmRSS cuenta las paginas
 *    compartidas en cada proceso que las usa. libc y las demas bibliotecas
 *    estan una sola vez en la RAM, pero aparecen en el RSS de todos, y lo
 *    mismo pasa con las paginas copy-on-write despues de un fork. Por
 *    ejemplo, el navegador tiene varios procesos "Isolated Web Co" que
 *    comparten mucho codigo y cada uno lo suma.
 *
 * ---------------------------------------------------------------------
 * EJEMPLO 1 - PREGUNTAS DE DISCUSION
 * ---------------------------------------------------------------------
 *
 * 1. ¿Por que printf salio despues de write?
 *
 *    Cuando redirijo a un archivo, stdout ya no es una terminal y la libc
 *    usa bufer completo. printf va guardando el texto en memoria y solo lo
 *    manda al kernel al final, cuando el programa termina. write va directo
 *    al kernel, por eso sale primero. En el laboratorio IPC pasaba algo
 *    parecido con fork: si quedaba texto en el bufer al hacer fork, el hijo
 *    heredaba una copia y el mensaje salia repetido. Por eso habia que usar
 *    setvbuf o fflush antes.
 *
 * 2. ¿Leer byte por byte o por bloques?
 *
 *    Por bloques. Lo probe con copiar: con bufer de 1 byte, copiar 1 MiB
 *    tardo 1.14 s (mas de 2 millones de llamadas), y con 4096 bytes copiar
 *    20 MiB tardo 0.02 s. Lo que cuesta es cada cruce al kernel, copiar los
 *    bytes es lo de menos.
 *
 * 3. ¿Quien paga mas cruces, monolitico o microkernel?
 *
 *    El microkernel. Ahi el sistema de archivos, los drivers y la red son
 *    procesos en modo usuario, entonces leer un archivo necesita varios
 *    mensajes IPC (aplicacion -> kernel -> servidor de archivos -> kernel
 *    -> driver ...) y cada mensaje es otro cruce. En un kernel monolitico
 *    como Linux todo eso se resuelve dentro del kernel con una sola entrada.
 *
 * ---------------------------------------------------------------------
 * EJEMPLO 2 - PREGUNTAS DE DISCUSION
 * ---------------------------------------------------------------------
 *
 * 1. ¿Por que un enlace duro no cruza sistemas de archivos?
 *
 *    Un enlace duro es solo una entrada de directorio con un numero de
 *    inodo, y ese numero solo tiene sentido dentro de su sistema de
 *    archivos. El inodo 2216834 de mi disco no tiene nada que ver con un
 *    inodo 2216834 en una USB.
 *
 * 2. Si otro proceso hace rm, ¿el primero sigue leyendo?
 *
 *    Si. rm solo borra el nombre y baja el contador de enlaces, pero el
 *    proceso sigue teniendo su descriptor apuntando al inodo. Los datos se
 *    liberan cuando el contador llega a 0 y ademas se cierra el ultimo
 *    descriptor.
 *
 * 3. ¿Que pasaba con canal_fifo sin unlink?
 *
 *    Se quedaba en el directorio, porque mkfifo crea una entrada real en el
 *    sistema de archivos y solo unlink la quita. Con stat aparece como FIFO
 *    (en ls -l empieza con p) y con tamano 0, porque lo que pasa por un
 *    FIFO va por memoria del kernel y nunca se guarda en el disco. En mi
 *    codigo si hago unlink de los dos FIFOs al final.
 *
 * 4. write y se va la luz antes de fsync
 *
 *    Los datos se pueden perder. write solo los copio a la cache de
 *    paginas en RAM, y si se va la luz antes de que el kernel los pase al
 *    disco, se borran. El archivo puede quedar vacio, con la version vieja
 *    o a medias. El journaling ayuda a que el sistema de archivos no quede
 *    danado, pero no recupera lo que no alcanzo a escribirse. En mi prueba
 *    con 50 MiB (ver copiar.c), fsync subio el tiempo de 0.02 s a 0.05 s,
 *    que es lo que cuesta esperar al disco de verdad.
 *
 * ---------------------------------------------------------------------
 * BONO E2 - RESULTADOS (copiar)
 * ---------------------------------------------------------------------
 *
 *    bufer      archivo   llamadas read+write   tiempo
 *    1 B        1 MiB     ~2 100 000            1.14 s
 *    64 B       20 MiB    ~655 000              0.36 s
 *    4096 B     20 MiB    ~10 000               0.02 s
 *    65536 B    20 MiB    ~640                  0.02 s
 *    1 MiB      20 MiB    ~41                   0.02 s
 *
 *    En todos los casos "cmp origen destino" dijo que la copia era
 *    identica. El tiempo baja muchisimo de 1 byte a 4 KiB y despues ya no
 *    cambia, porque lo caro es el numero de llamadas al sistema.
 *
 *    Con fsync (50 MiB, bufer de 64 KiB, en el disco): sin fsync 0.02 s,
 *    con fsync 0.05 s. Sin fsync el programa termina apenas los datos estan
 *    en la cache de paginas; con fsync espera a que lleguen al disco.
 *
 * ---------------------------------------------------------------------
 * AUTOEVALUACION
 * ---------------------------------------------------------------------
 *
 * 1. La llamada al sistema cambia a modo kernel y el kernel valida los
 *    argumentos antes de hacer el trabajo, por eso es mucho mas lenta. En
 *    mi equipo salio unas 200 veces mas cara que una funcion normal.
 * 2. Linux tiene modulos: codigo que se puede cargar al kernel mientras
 *    esta corriendo (lsmod, modinfo).
 * 3. El inodo guarda el tipo, los permisos, el dueno, el tamano, las
 *    fechas, el contador de enlaces y donde estan los datos. El nombre no
 *    lo guarda, ese esta en el directorio.
 * 4. Los datos se quedan. El contador baja a 1 y el otro nombre sigue
 *    apuntando al mismo inodo.
 * 5. fsync garantiza que los datos ya se escribieron en el disco. write
 *    normalmente solo los deja en la cache.
 */
