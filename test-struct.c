#include <stdio.h>
#include <string.h>

int main(){
    typedef struct{
        int CI;
        char Nombre[24];
        int Edad;
        float Prom;
        char Sexo;
        int Activo;
        char* Grado;
        char* Fecha;
    }Estudiante;

    Estudiante archivo[99];

    strcpy(archivo[1].Nombre, "Jose Angel Teran Alfonzo Josteral");

    printf("hola %s \n", archivo[1].Nombre);
    return 0;
}