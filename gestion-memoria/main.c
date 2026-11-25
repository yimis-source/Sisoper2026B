#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  void *datos;
  size_t size;
  size_t capacity;
  size_t elem_size;
} ArregloDinamico;

/* Devuelve la direccion del elemento i. Como datos es void *, no se puede
 * indexar directamente: se convierte a char * (1 byte) y se avanza
 * i * elem_size bytes. */
static void *elemento(ArregloDinamico *arr, size_t i) {
  return (char *)arr->datos + i * arr->elem_size;
}

/* Duplica la capacidad hasta que quepan al menos minimo elementos. */
static void asegurar_capacidad(ArregloDinamico *arr, size_t minimo) {
  if (minimo <= arr->capacity) {
    return;
  }

  size_t nueva_capacidad = arr->capacity;
  while (nueva_capacidad < minimo) {
    nueva_capacidad *= 2;
  }

  printf("Ya no queda espacio (%zu/%zu). Vamos a pedir mas memoria con "
         "realloc, hasta %zu elementos.\n",
         arr->size, arr->capacity, nueva_capacidad);

  void *tmp = realloc(arr->datos, nueva_capacidad * arr->elem_size);

  if (tmp == NULL) {
    fprintf(stderr, "No se pudo ampliar la memoria con realloc.\n");
    free(arr->datos);
    exit(EXIT_FAILURE);
  }

  arr->datos = tmp;
  arr->capacity = nueva_capacidad;

  printf("Listo, realloc funciono. Ahora hay espacio para %zu elementos.\n",
         arr->capacity);
}

void arreglo_iniciar(ArregloDinamico *arr, size_t capacidad_inicial,
                     size_t elem_size) {
  arr->datos = malloc(capacidad_inicial * elem_size);
  arr->elem_size = elem_size;

  if (arr->datos == NULL) {
    fprintf(stderr, "No se pudo reservar la memoria.\n");
    exit(EXIT_FAILURE);
  }

  arr->size = 0;
  arr->capacity = capacidad_inicial;

  printf("Arreglo iniciado, hay espacio para %zu elementos.\n",
         capacidad_inicial);
}

void arreglo_agregar(ArregloDinamico *arr, const void *valor) {
  asegurar_capacidad(arr, arr->size + 1);

  /* Se copia el contenido de valor (elem_size bytes), no el puntero. */
  memcpy(elemento(arr, arr->size), valor, arr->elem_size);
  arr->size++;
}

void arreglo_liberar(ArregloDinamico *arr) {
  free(arr->datos);

  arr->datos = NULL;
  arr->size = 0;
  arr->capacity = 0;

  printf("Ya se libero la memoria correctamente.\n");
}

void arreglo_insertar(ArregloDinamico *arr, size_t indice, const void *valor) {
  if (indice > arr->size) {
    fprintf(stderr, "No se pudo insertar en el índice %zu.\n", indice);
    free(arr->datos);
    exit(EXIT_FAILURE);
  }

  asegurar_capacidad(arr, arr->size + 1);

  /* Corre una posicion a la derecha los elementos desde indice hasta el
   * final. memmove sirve aunque origen y destino se solapen. */
  memmove(elemento(arr, indice + 1), elemento(arr, indice),
          (arr->size - indice) * arr->elem_size);
  memcpy(elemento(arr, indice), valor, arr->elem_size);
  arr->size++;
}

void eliminar_arreglo(ArregloDinamico *arr, size_t indice) {
  if (indice >= arr->size) {
    fprintf(stderr, "No se pudo eliminar el elemento en el índice %zu.\n",
            indice);
    return;
  }

  /* Corre una posicion a la izquierda los elementos que estan despues de
   * indice. */
  memmove(elemento(arr, indice), elemento(arr, indice + 1),
          (arr->size - indice - 1) * arr->elem_size);
  arr->size--;
}

int arreglo_buscar(ArregloDinamico *arr, const void *valor) {
  for (size_t i = 0; i < arr->size; i++) {
    if (memcmp(elemento(arr, i), valor, arr->elem_size) == 0) {
      return (int)i;
    }
  }
  return -1;
}

static void imprimir_enteros(ArregloDinamico *arr) {
  for (size_t i = 0; i < arr->size; i++) {
    printf("%d ", *(int *)elemento(arr, i));
  }
  printf("\n");
}

int main(void) {
  ArregloDinamico arr;

  arreglo_iniciar(&arr, 2, sizeof(int));
  for (int i = 1; i <= 20; i++) {
    int valor = i * 10;
    arreglo_agregar(&arr, &valor);
  }

  printf("\nContenido final del arreglo (%zu elementos, capacidad %zu):\n",
         arr.size, arr.capacity);
  imprimir_enteros(&arr);

  int nuevo = 2;
  arreglo_insertar(&arr, 5, &nuevo);
  imprimir_enteros(&arr);

  eliminar_arreglo(&arr, 4);
  imprimir_enteros(&arr);

  printf("el numero esta la posicion  %i ", arreglo_buscar(&arr, &nuevo));

  printf("\n");

  arreglo_liberar(&arr);

  return 0;
}
