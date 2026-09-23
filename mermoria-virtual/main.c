/* Ejercicio 2 RESUELTO: Paginacion con reemplazo FIFO */
#include <stdio.h>
#include <stdlib.h>

#define TAM_PAGINA   256
#define NUM_PAGINAS  8
#define NUM_MARCOS   4

int tabla_paginas[NUM_PAGINAS];
int cola_marcos[NUM_MARCOS];
int marcos_ocupados = 0;
int siguiente_marco_a_reemplazar = 0;
int contador_fallos = 0;
int contador_aciertos = 0;

void inicializar_tabla_paginas(void) {
    int i;
    for (i = 0; i < NUM_PAGINAS; i++) tabla_paginas[i] = -1;
    for (i = 0; i < NUM_MARCOS; i++) cola_marcos[i] = -1;
    marcos_ocupados = 0;
    siguiente_marco_a_reemplazar = 0;
    contador_fallos = 0;
    contador_aciertos = 0;
}

void mostrar_tabla_paginas(void) {
    int i;
    printf("\n--- Tabla de paginas (proceso) ---\n");
    for (i = 0; i < NUM_PAGINAS; i++) {
        if (tabla_paginas[i] == -1) printf(" Pagina %d -> no cargada\n", i);
        else                        printf(" Pagina %d -> Marco %d\n", i, tabla_paginas[i]);
    }
    printf("--- Marcos fisicos ---\n");
    for (i = 0; i < NUM_MARCOS; i++) {
        if (cola_marcos[i] == -1) printf(" Marco %d -> libre\n", i);
        else                      printf(" Marco %d -> Pagina %d\n", i, cola_marcos[i]);
    }
    printf("Fallos de pagina: %d | Aciertos: %d\n", contador_fallos, contador_aciertos);
}

int cargar_pagina_en_memoria(int pagina) {
    int marco_elegido;

    if (marcos_ocupados < NUM_MARCOS) {
        /* Hay marco libre: se usan en orden 0,1,2,... */
        marco_elegido = marcos_ocupados;
        marcos_ocupados++;
    } else {
        /* Memoria llena: FIFO -> se reemplaza el marco mas antiguo */
        marco_elegido = siguiente_marco_a_reemplazar;
        int pagina_victima = cola_marcos[marco_elegido];
        tabla_paginas[pagina_victima] = -1;   /* la victima deja de estar en memoria */
        printf("   (reemplazo FIFO: sale pagina %d del marco %d)\n", pagina_victima, marco_elegido);
        siguiente_marco_a_reemplazar = (siguiente_marco_a_reemplazar + 1) % NUM_MARCOS;
    }

    cola_marcos[marco_elegido] = pagina;
    tabla_paginas[pagina] = marco_elegido;
    return marco_elegido;
}

int traducir_direccion(int direccion_logica) {
    int numero_pagina, desplazamiento, marco, direccion_fisica;

    if (direccion_logica < 0 || direccion_logica >= TAM_PAGINA * NUM_PAGINAS) {
        printf("Direccion logica %d invalida (rango 0 - %d).\n",
               direccion_logica, TAM_PAGINA * NUM_PAGINAS - 1);
        return -1;
    }

    numero_pagina  = direccion_logica / TAM_PAGINA;
    desplazamiento = direccion_logica % TAM_PAGINA;

    if (tabla_paginas[numero_pagina] != -1) {
        marco = tabla_paginas[numero_pagina];
        contador_aciertos++;
        printf("[ACIERTO] ");
    } else {
        contador_fallos++;
        printf("[FALLO DE PAGINA] ");
        marco = cargar_pagina_en_memoria(numero_pagina);
    }

    direccion_fisica = marco * TAM_PAGINA + desplazamiento;

    printf("Direccion logica %d -> pagina %d, desplazamiento %d -> "
           "marco %d -> direccion fisica %d\n",
           direccion_logica, numero_pagina, desplazamiento, marco, direccion_fisica);

    return direccion_fisica;
}

int main() {
    int secuencia[] = {0, 300, 600, 900, 1200, 1500, 50, 1800, 610, 2000};
    int total = sizeof(secuencia) / sizeof(secuencia[0]);
    int i, opcion, direccion;

    inicializar_tabla_paginas();

    printf("=============================================================\n");
    printf(" SIMULADOR DE MEMORIA VIRTUAL - PAGINACION CON REEMPLAZO FIFO\n");
    printf(" Tamano de pagina: %d bytes | Paginas logicas: %d | Marcos: %d\n",
           TAM_PAGINA, NUM_PAGINAS, NUM_MARCOS);
    printf("=============================================================\n");

    do {
        printf("\n1. Ejecutar secuencia de ejemplo\n2. Traducir una direccion manualmente\n");
        printf("3. Ver tabla de paginas y marcos\n0. Salir\nSeleccione una opcion: ");
        if (scanf("%d", &opcion) != 1) break;

        switch (opcion) {
            case 1:
                for (i = 0; i < total; i++) traducir_direccion(secuencia[i]);
                mostrar_tabla_paginas();
                break;
            case 2:
                printf("Ingrese direccion logica (0 - %d): ", TAM_PAGINA * NUM_PAGINAS - 1);
                scanf("%d", &direccion);
                traducir_direccion(direccion);
                break;
            case 3:
                mostrar_tabla_paginas();
                break;
            case 0:
                printf("Saliendo del simulador...\n");
                break;
            default:
                printf("Opcion invalida.\n");
        }
    } while (opcion != 0);

    return 0;
}