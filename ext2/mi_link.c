// Autors: Marc Melià Flexas, Pau Rosado Muñoz, Xavier Vives Marcus
#include "directorios.h"

// Sintaxis: ./mi_link disco /ruta_fichero_original /ruta_enlace

int main(int argc, char **argv)
{
    if (argc != 4)
    {
        printf("Sintaxis: ./mi_link disco /ruta_fichero_original /ruta_enlace");
        exit (-1);
    }
    // Montamos dispositivo
    bmount(argv[1]);
    if(argv[2][strlen(argv[2])-1]=='/' || argv[3][strlen(argv[3])-1]=='/')
    {
        printf("ERROR: ESPECIFICA FICHEROS\n");
    }
    mi_link(argv[2],argv[3]);
    bumount();
}