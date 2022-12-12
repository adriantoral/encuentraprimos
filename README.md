# Getting started
En este proyecto vamos a desarrollar una aplicación para encontrar números primos paralelizando la tarea entre varias instancias de unmismo proceso.

Recordamos que un número primo es un número natural mayor que 1 que solo es divisible por sí mismo y por la unidad.

Hay numerosos algoritmos para comprobar si un número es primo, pero para este usaremos uno de fuerza bruta, para poder comprobar el efecto de la paralelización.

Un número K es primo si resulta divisible por cualquier cifra entre 2 y K/2. El algoritmo simplemente prueba a dividir entre 2 y K/2 hasta encontrar un número por el que es divisible (no es primo) o finalizar el bucle sin conseguirlo (es primo).

# Collaborators
- [Adrian Toral](https://github.com/tory1103)
- Miguel Gracia

