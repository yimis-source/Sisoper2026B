#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  void *datos;
  size_t size;
  size_t capacity;
  size_t elem_size
} ArregloDinamico;

void arreglo_iniciar(ArregloDinamico *arr, size_t capacidad_inicial , size_t elem_size) {
  arr->datos = (int *)malloc(capacidad_inicial * elem_size);
  arr->elem_size= elem_size;

  if (arr->datos == NULL) {
    fprintf(stderr, "No se pudo reservar la memoria.\n");
    exit(EXIT_FAILURE);
  }

  arr->size = 0;
  arr->capacity = capacidad_inicial;

  printf("Arreglo iniciado, hay espacio para %zu elementos.\n",
         capacidad_inicial);
}

void arreglo_agregar(ArregloDinamico *arr, void *valor ) {
  if (arr->size == arr->capacity) {
    size_t nueva_capacidad = arr->capacity * 2;

    printf("Ya no queda espacio (%zu/%zu). Vamos a pedir mas memoria con "
           "realloc, hasta %zu elementos.\n",
           arr->size, arr->capacity, nueva_capacidad);

    int *tmp = (int *)realloc(arr->datos, nueva_capacidad * arr->elem_size);

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

  arr->datos[arr->size] = valor;
  arr->size++;
}

void arreglo_liberar(ArregloDinamico *arr) {
  free(arr->datos);

  arr->datos = NULL;
  arr->size = 0;
  arr->capacity = 0;

  printf("Ya se libero la memoria correctamente.\n");
}

void arreglo_insertar(ArregloDinamico *arr, size_t indice, int valor) {

  if (indice >= arr->capacity) {
    size_t nueva_capacidad = arr->capacity;
    while (nueva_capacidad < indice) {
      nueva_capacidad *= 2;
    }
    int *tmp = (int *)realloc(arr->datos, nueva_capacidad * sizeof(int));

    if (tmp == NULL) {
      fprintf(stderr, "No se pudo ampliar la memoria con realloc.\n");
      free(arr->datos);
      exit(EXIT_FAILURE);
    }
  }

  if (indice > arr->size) {
    fprintf(stderr, "No se pudo insertar en el índice %zu.\n", indice);
    free(arr->datos);
    exit(EXIT_FAILURE);
  }
  for (size_t i = arr->capacity; i >= indice; i--) {
    arr->datos[i] = arr->datos[i - 1];
  }
  arr->datos[indice] =;
  arr->size++;
}

void eliminar_arreglo(ArregloDinamico *arr, size_t indice) {
  if (indice >= arr->size) {
    fprintf(stderr, "No se pudo eliminar el elemento en el índice %zu.\n",
            indice);
    return;
  }

  for (size_t i = indice; i <= arr->size; i++) {
    arr->datos[i] = arr->datos[i + 1];
  }
  arr->size--;
}

int arreglo_buscar(ArregloDinamico *arr, int valor) {

  for (size_t i = 0; i < arr->size; i++)
    if (arr->datos[i] == valor) {
      return i;
    }
  return -1;
}

int main(void) {
  ArregloDinamico arr;

  arreglo_iniciar(&arr, 2, sizeof(int));
  for (int i = 1; i <= 20; i++) {
      int valor= i*10;
      
    arreglo_agregar(&arr, &valor);
  }

  printf("\nContenido final del arreglo (%zu elementos, capacidad %zu):\n",
         arr.size, arr.capacity);

  for (size_t i = 0; i < arr.size; i++) {
    printf("%d ", arr.datos[i]);
  }

  printf("\n");

  arreglo_insertar(&arr, 5, 2);

  for (size_t i = 0; i < arr.size; i++) {
    printf("%d ", arr.datos[i]);
  }
  printf("\n");

  eliminar_arreglo(&arr, 4);

  for (size_t i = 0; i < arr.size; i++) {
    printf("%d ", arr.datos[i]);
  }
  printf("\n");

  printf("el numero esta la posicion  %i ", arreglo_buscar(&arr, 2));
  
  printf("\n"); 
  
  arreglo_liberar(&arr);

  return 0;
}
