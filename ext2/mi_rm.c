// Autors: Marc Melià Flexas, Pau Rosado Muñoz, Xavier Vives Marcus
#include "directorios.h"

// Sintaxis: :/mi_rm disco /ruta
int main(int argc, char **argv)
{

    // Comprobamos sintaxis
    if (argc == 3)
    {
        // Montar dispositivo
        bmount(argv[1]);        

        if (strcmp(argv[2],"/")==0)
        {
            printf("ERROR: No se puede eliminar el inodo raiz\n");
            return EXIT_FAILURE;
        }else

           // Comprobamos si es un fichero
        if (argv[2][strlen(argv[2]) - 1] != '/')
        {
          // Llamada a unlink
          mi_unlink(argv[2]);

        }else{

            printf("ERROR: No es un fichero\n");
            return EXIT_FAILURE;

        }

        // Desmontamos el dispositivo
        bumount();
        return EXIT_SUCCESS;
    }
    else
    {
        printf("Sintaxis: :/mi_rm disco /ruta");
        return -1;
    }
}