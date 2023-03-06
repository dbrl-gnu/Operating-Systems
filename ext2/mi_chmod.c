// Autors: Marc Melià Flexas, Pau Rosado Muñoz, Xavier Vives Marcus
#include "directorios.h"

// Sintaxis: ./mi_chmod <disco> <permisos> </ruta>
int main(int argc, char **argv)
{
    if (argc != 4)
    {
        printf("Sintaxis: ./mi_chmod <disco> <permisos> </ruta>\n");
        return -1;
    }
    if (bmount(argv[1]) == -1)
    {
       
        return -1;
    }
    if ((atoi(argv[2]) < 0) || (atoi(argv[2]) > 7))
    {

        
        return -1;
    }

    // Creamos el directorio con la ruta y los permisos
    if (mi_chmod(argv[3],(atoi(argv[2]))) < 0)
    {

        
        return -1;
    }
 
    if (bumount() == -1)
    {
        
        return -1;
    }

    return 0;
}