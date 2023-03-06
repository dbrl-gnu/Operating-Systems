// Autors: Marc Melià Flexas, Pau Rosado Muñoz, Xavier Vives Marcus
// Programa ficticio para probar las funcionalidades de cambio de permisos

#include "ficheros.h"

// Sintaxis : permitir <nombre_dispositivo> <ninodo> <permisos>
int main(int argc, char **argv)
{

    // Validación de la sintaxis
    if (argc == 4) {
     
        // Montar dispositivo
        if (bmount(argv[1]) == -1) {
			
            perror("ERROR");
			return -1;
	    }   

        // Obtenemos los parámetros ninodo y permisos
        unsigned int ninodo = atoi(argv[2]);
        unsigned char permisos = atoi(argv[3]);

        // Llamada a mi_chmod_f() con los argumentos recibidos, convertidos a entero
        if(mi_chmod_f(ninodo,permisos)==-1){

            perror("ERROR");
			return -1;
        }

        // Desmontar dispositivo
        if (bumount() == -1) {
			
            perror("ERROR");
			return -1;
	    }  

    }else{

        // Mala Sintaxi
        printf("Sintaxis: permitir <nombre_dispositivo> <ninodo> <permisos>\n");
	    return -1;

    }

    return 0;

}