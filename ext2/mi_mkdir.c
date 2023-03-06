// Autors: Marc Melià Flexas, Pau Rosado Muñoz, Xavier Vives Marcus
#include "directorios.h"

// Sintaxis: ./mi_mkdir <disco> <permisos> </ruta>
int main(int argc, char **argv)
{
    if (argc != 4)
    {
        printf("Sintaxis: ./mi_mkdir <nombre_dispositivo> <permisos> </ruta_directorio/>\n");
        return -1;
    }
    if (bmount(argv[1]) == -1)
    {
       
        return -1;
    }
    if ((atoi(argv[2]) < 0) || (atoi(argv[2]) > 7))
    {

        printf("Error: modo inválido: <<%i>>\n",(atoi(argv[2])));
        return -1;
    }
 
   

    if (argv[3][strlen(argv[3]) - 1] == '/')
    {
        // Creamos el directorio con la ruta y los permisos
        if (mi_creat(argv[3],(atoi(argv[2]))) < 0)
        {

         
            return -1;
        }
    }
    else
    {

      
        return -1;
    }

    if (bumount() == -1)
    {
        
        return -1;
    }
}