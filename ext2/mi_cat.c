// Autors: Marc Melià Flexas, Pau Rosado Muñoz, Xavier Vives Marcus
#include "directorios.h"

// Sintaxis Sintaxis: ./mi_cat <disco> </ruta_fichero>
int main(int argc, char **argv)
{

    // Comprobamos sintaxis
    if (argc != 3)
    {
        printf("Sintaxis: mi_cat <disco> </ruta_fichero>\n");
        return -1;
    }
    // Montamos el dispositivo
    bmount(argv[1]);

    // comprobar que la ruta se corresponda a un fichero

    // Caso en el que se trata de un fichero
    if (argv[2][strlen(argv[2]) - 1] != '/')
    {
        char *ruta = argv[2];
        unsigned int leidos, offset = 0;
        unsigned int t_leidos = 0;
        int tambuffer = 1500;

        char buffer_texto[tambuffer];
        memset(buffer_texto, 0, tambuffer);

        leidos = mi_read(ruta, buffer_texto, offset, tambuffer);
        while (leidos > 0)
        {
            if (leidos == -1)
            {
                printf("ERROR");
                return -1;
            }
            t_leidos += leidos;
            write(1, buffer_texto, leidos);
            memset(buffer_texto, 0, tambuffer);
            offset += tambuffer;
            leidos = mi_read(ruta, buffer_texto, offset, tambuffer);
        }
        fprintf(stderr, "\ntotal_leidos %d\n", t_leidos);
    }
    // Caso en el que no es un fichero
    else
    {
        printf("ERROR: No se trata de un fichero\n");
    }
    // Return 0?
    return 0;
}