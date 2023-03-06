#include "directorios.h"

// Sintaxis: ./mi_stat <disco> </ruta>
int main(int argc, char **argv)
{
    if (argc != 3)
    {
        printf("Sintaxis: ./mi_stat <disco> </ruta>\n");
        return -1;
    }
    if (bmount(argv[1]) == -1)
    {
        
        return -1;
    }

    struct STAT stat;

    if (mi_stat(argv[2], &stat) < 0)
    {

        return -1;
    }

    if (bumount() == -1)
    {
      
        return -1;
    }

    return 0;
}