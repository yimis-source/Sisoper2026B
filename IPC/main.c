#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>

#define FIFO_FILE "canal_fifo"
#define FIFO_FILE1 "canal_fifo1"
#define SHM_KEY 1234
#define BUFFER_SIZE 128

typedef struct {
    int a;
    int b;
    int resultado_mul;
    int resultado_sum;
    int listo;
} DatosCompartidos;

int main() {
    int shm_id;
    DatosCompartidos *mem;
    pid_t pid_prod, pid_cons1, pid_cons2;

    // Crear memoria compartida
    shm_id = shmget(SHM_KEY, sizeof(DatosCompartidos), IPC_CREAT | 0666);
    if (shm_id < 0) {
        perror("Error creando memoria compartida");
        exit(1);
    }

    mem = (DatosCompartidos *) shmat(shm_id, NULL, 0);
    if (mem == (DatosCompartidos *) -1) {
        perror("Error asociando memoria compartida");
        exit(1);
    }

    // Inicializar memoria compartida
    mem->a = 0;
    mem->b = 0;
    mem->resultado_mul = 0;
    mem->resultado_sum = 0;
    mem->listo = 0;

    // Crear FIFO si no existe
    if (mkfifo(FIFO_FILE, 0666) == -1 && errno != EEXIST) {
        perror("Error al crear FIFO");
        exit(EXIT_FAILURE);
    }

    // Crear FIFO1 si no existe
    if (mkfifo(FIFO_FILE1, 0666) == -1 && errno != EEXIST) {
        perror("Error al crear FIFO1");
        exit(EXIT_FAILURE);
    }

    pid_prod = fork();

    if (pid_prod == 0) {
        // Proceso Productor
        int fd = open(FIFO_FILE, O_WRONLY);

        if (fd < 0) {
            perror("Error abriendo FIFO para escritura");
            exit(EXIT_FAILURE);
        }

        int fd1 = open(FIFO_FILE1, O_WRONLY);

        if (fd1 < 0) {
            perror("Error abriendo FIFO1 para escritura");
            close(fd);
            exit(EXIT_FAILURE);
        }

        for (int i = 1; i <= 5; i++) {
            int a = i, b = i + 2;
            char buffer[BUFFER_SIZE];
            snprintf(buffer, BUFFER_SIZE, "%d %d", a, b);
            write(fd, buffer, strlen(buffer) + 1);
            write(fd1, buffer, strlen(buffer) + 1);
            printf("[Productor] Enviado: %s\n", buffer);
            sleep(1);
        }

        close(fd);
        close(fd1);
        exit(0);

    } else {

        // PRIMER CONSUMIDOR

        pid_cons1 = fork();

        if (pid_cons1 == 0) {

            int fd = open(FIFO_FILE, O_RDONLY);

            if (fd < 0) {
                perror("Error abriendo FIFO para lectura");
                exit(1);
            }

            int *resultados = malloc(5 * sizeof(int));

            if (resultados == NULL) {
                perror("Error reservando memoria");
                close(fd);
                exit(1);
            }

            for (int i = 0; i < 5; i++) {

                char buffer[BUFFER_SIZE];
                int a, b;

                int bytes = read(fd, buffer, BUFFER_SIZE);

                if (bytes > 0) {

                    sscanf(buffer, "%d %d", &a, &b);

                    while (mem->listo != 0) {
                        usleep(1000);
                    }

                    mem->a = a;
                    mem->b = b;

                    mem->resultado_mul = mem->a * mem->b;

                    resultados[i] = mem->resultado_mul;

                    mem->listo = 1;

                    printf("[Consumidor 1] %d x %d = %d\n",
                           mem->a,
                           mem->b,
                           resultados[i]);

                    mem->listo = 0;
                }
            }

            printf("[Consumidor 1] Resultados: ");

            for (int i = 0; i < 5; i++) {
                printf("%d ", resultados[i]);
            }

            printf("\n");

            free(resultados);
            close(fd);

            exit(0);

        } else {

            // SEGUNDO CONSUMIDOR
            pid_cons2 = fork();

            if (pid_cons2 == 0) {

                int fd = open(FIFO_FILE1, O_RDONLY);

                if (fd < 0) {
                    perror("Error abriendo FIFO para lectura");
                    exit(1);
                }

                int *resultados = malloc(5 * sizeof(int));

                if (resultados == NULL) {
                    perror("Error reservando memoria");
                    close(fd);
                    exit(1);
                }

                for (int i = 0; i < 5; i++) {

                    char buffer[BUFFER_SIZE];
                    int a, b;

                    int bytes = read(fd, buffer, BUFFER_SIZE);

                    if (bytes > 0) {

                        sscanf(buffer, "%d %d", &a, &b);

                        while (mem->listo != 0) {
                            usleep(1000);
                        }

                        mem->a = a;
                        mem->b = b;

                        mem->resultado_sum = mem->a + mem->b;

                        resultados[i] = mem->resultado_sum;

                        mem->listo = 1;

                        printf("[Consumidor 2] %d + %d = %d\n",
                               mem->a,
                               mem->b,
                               resultados[i]);

                        mem->listo = 0;
                    }
                }

                printf("[Consumidor 2] Resultados: ");

                for (int i = 0; i < 5; i++) {
                    printf("%d ", resultados[i]);
                }

                printf("\n");

                free(resultados);
                close(fd);

                exit(0);
            }
        }
    }

    // El proceso padre espera a que todos los hijos terminen
    waitpid(pid_prod, NULL, 0);
    waitpid(pid_cons1, NULL, 0);
    waitpid(pid_cons2, NULL, 0);

    // Liberar memoria compartida y eliminar los FIFO
    shmdt(mem);
    shmctl(shm_id, IPC_RMID, NULL);

    unlink(FIFO_FILE);
    unlink(FIFO_FILE1);

    return 0;
}