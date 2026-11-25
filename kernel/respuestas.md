# Respuestas: El kernel y el sistema de archivos

## E1 · Preguntas de análisis (minips)

**1. ¿Qué estado predomina? ¿Por qué casi ninguno está en R?**

En mi salida el estado que más sale es S (191 procesos), después I (145, que son hilos del kernel como los kworker). En R solo vi 2, y uno era el mismo minips. Tiene sentido porque en un núcleo solo puede correr un proceso a la vez, y casi todos los demás están esperando algo (el teclado, la red, un temporizador), así que quedan dormidos hasta que llegue lo que esperan.

**2. ¿Qué procesos tienen más cambios involuntarios y cuáles más voluntarios?**

Los que más involuntarios tienen son Hyprland (12963), zen-bin (9090) y los procesos "Isolated Web Co" del navegador. Son los que más usan CPU (dibujar la pantalla, ejecutar JavaScript), entonces el planificador los saca cuando se les acaba el quantum. Eso es lo típico de un proceso limitado por CPU.

En voluntarios el primero es pipewire-pulse (529866), que es el servidor de audio. Se la pasa bloqueándose para esperar el siguiente bloque de sonido, o sea que es un proceso limitado por E/S. Hyprland y el navegador también tienen muchos voluntarios porque esperan eventos del usuario.

**3. Hijos del laboratorio IPC en el listado**

Mi programa del laboratorio se llama `main` y no `lab_ipc`. Al correrlo aparecieron 4 procesos `main`: el padre (11293) y sus tres hijos (11295, 11296, 11297). El PPID del padre salió 1 porque lo lancé en segundo plano.

```
 11293      1 main             S      1684        1        0
 11295  11293 main             S      1404        5        0
 11296  11293 main             S      1476        4        0
 11297  11293 main             S      1600        5        0
```

Al principio los cuatro estaban en S. El productor duerme con `sleep(1)` entre envíos y los consumidores se bloquean en `read` hasta que llega algo al FIFO. Se ve en que los VOL suben de a uno cada segundo.

Un segundo después dos hijos ya estaban en Z (zombi) con RSS 0:

```
 11296  11293 main             Z         0        6        0
 11297  11293 main             Z         0        7        0
```

Ya habían terminado, pero el padre seguía en `waitpid(pid_prod, ...)` esperando al productor y todavía no había recogido su estado de salida. Cuando el padre llega a los otros `waitpid`, los zombis desaparecen.

**4. ¿Por qué la suma de VmRSS sobrestima la memoria?**

La suma me dio 13 503 684 KiB (unos 13 GB), pero `free -m` marcaba 9400 MiB usados. La diferencia sale de que VmRSS cuenta las páginas compartidas en cada proceso que las usa. libc y las demás bibliotecas están una sola vez en la RAM, pero aparecen en el RSS de todos, y lo mismo pasa con las páginas copy-on-write después de un `fork`. Por ejemplo, el navegador tiene varios procesos "Isolated Web Co" que comparten mucho código y cada uno lo suma.

## Ejemplo 1 · Discusión

**1. ¿Por qué printf salió después de write?**

Cuando redirijo a un archivo, stdout ya no es una terminal y la libc usa búfer completo. `printf` va guardando el texto en memoria y solo lo manda al kernel al final, cuando el programa termina. `write` va directo al kernel, por eso sale primero. En el laboratorio IPC pasaba algo parecido con `fork`: si quedaba texto en el búfer al hacer `fork`, el hijo heredaba una copia y el mensaje salía repetido. Por eso había que usar `setvbuf` o `fflush` antes.

**2. ¿Leer byte por byte o por bloques?**

Por bloques. Lo probé con `copiar`: con búfer de 1 byte, copiar 1 MiB tardó 1.14 s (más de 2 millones de llamadas), y con 4096 bytes copiar 20 MiB tardó 0.02 s. Lo que cuesta es cada cruce al kernel, copiar los bytes es lo de menos.

**3. ¿Quién paga más cruces, monolítico o microkernel?**

El microkernel. Ahí el sistema de archivos, los drivers y la red son procesos en modo usuario, entonces leer un archivo necesita varios mensajes IPC (aplicación → kernel → servidor de archivos → kernel → driver ...) y cada mensaje es otro cruce. En un kernel monolítico como Linux todo eso se resuelve dentro del kernel con una sola entrada.

## Ejemplo 2 · Discusión

**1. ¿Por qué un enlace duro no cruza sistemas de archivos?**

Un enlace duro es solo una entrada de directorio con un número de inodo, y ese número solo tiene sentido dentro de su sistema de archivos. El inodo 2216834 de mi disco no tiene nada que ver con un inodo 2216834 en una USB.

**2. Si otro proceso hace rm, ¿el primero sigue leyendo?**

Sí. `rm` solo borra el nombre y baja el contador de enlaces, pero el proceso sigue teniendo su descriptor apuntando al inodo. Los datos se liberan cuando el contador llega a 0 y además se cierra el último descriptor.

**3. ¿Qué pasaba con canal_fifo sin unlink?**

Se quedaba en el directorio, porque `mkfifo` crea una entrada real en el sistema de archivos y solo `unlink` la quita. Con `stat` aparece como FIFO (en `ls -l` empieza con `p`) y con tamaño 0, porque lo que pasa por un FIFO va por memoria del kernel y nunca se guarda en el disco. En mi código sí hago `unlink` de los dos FIFOs al final.

**4. write y se va la luz antes de fsync**

Los datos se pueden perder. `write` solo los copió a la caché de páginas en RAM, y si se va la luz antes de que el kernel los pase al disco, se borran. El archivo puede quedar vacío, con la versión vieja o a medias. El journaling ayuda a que el sistema de archivos no quede dañado, pero no recupera lo que no alcanzó a escribirse. En mi prueba con 50 MiB, `fsync` subió el tiempo de 0.02 s a 0.05 s, que es lo que cuesta esperar al disco de verdad.

## Autoevaluación

1. La llamada al sistema cambia a modo kernel y el kernel valida los argumentos antes de hacer el trabajo, por eso es mucho más lenta. En mi equipo salió unas 200 veces más cara que una función normal.
2. Linux tiene módulos: código que se puede cargar al kernel mientras está corriendo (`lsmod`, `modinfo`).
3. El inodo guarda el tipo, los permisos, el dueño, el tamaño, las fechas, el contador de enlaces y dónde están los datos. El nombre no lo guarda, ese está en el directorio.
4. Los datos se quedan. El contador baja a 1 y el otro nombre sigue apuntando al mismo inodo.
5. `fsync` garantiza que los datos ya se escribieron en el disco. `write` normalmente solo los deja en la caché.
