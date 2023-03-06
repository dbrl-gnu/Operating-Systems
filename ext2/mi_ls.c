// Autors: Marc Melià Flexas, Pau Rosado Muñoz, Xavier Vives Marcus
#include "directorios.h"

// Lista el contenido de un directorios empleando mi_dir()
int main(int argc, char **argv)
{
    if (argc != 3)
    {
        printf("Sintaxis: ./mi_ls <disco> </ruta_directorio>\n");
        return -1;
    }

    char *camino = argv[2];
    char buffer[TAMBUFFER];

    if (bmount(argv[1]) == -1)
    {
       
        return -1;
    }

    // Es un directorio
    if (camino[strlen(camino) - 1] == '/')
    {
        if (mi_dir(argv[2], buffer, 'd') < 0)
        {
           
            return -1;
        }
    }
    else
    {
        // Es un fichero
        if (mi_dir(argv[2], buffer, 'f') < 0)
        {
           
            return -1;
        }
    }

    printf("%s\n", buffer);


    if (bumount() == -1)
    {
      
        return -1;
    }
    return 0;
}