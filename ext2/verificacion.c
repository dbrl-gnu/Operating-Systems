// Autors: Marc Melià Flexas, Pau Rosado Muñoz, Xavier Vives Marcus

#include "verificacion.h"

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        printf("Sintaxis: ./verificacion <nombre_dispositivo> <directorio_simulacion>");
        return EXIT_FAILURE;
    }
    // Montamos dispositivo
    bmount(argv[1]);
    unsigned int p_inodo_dir = 0;
    unsigned int p_inodo = 0;
    unsigned int p_entrada = 0;
    struct inodo inodo;

    int error;
    // Obtenemos inodo correspondiente a la ruta
    if ((error = buscar_entrada(argv[2], &p_inodo_dir, &p_inodo, &p_entrada, 0, 0)) < 0)
    {
        mostrar_error_buscar_entrada(error);
        return EXIT_FAILURE;
    }
    // Leemos inodo correspondiente
    leer_inodo(p_inodo, &inodo);
    // Comprobamos permisos
    if ((inodo.permisos & 4) != 4)
    {
        printf("[verificacion.c -> Error de permisos de lectura");
        return EXIT_FAILURE;
    }

    unsigned int numentradas = (inodo.tamEnBytesLog / sizeof(struct entrada));

    if (numentradas != NUMPROCESOS)
    {
        printf("[verificacion.c -> ERROR: La cantidad de entradas no coincide con NUMPROCESOS]\n");
        return EXIT_FAILURE;
    }
    char rutaVerificacion[80];
    strcpy(rutaVerificacion, argv[2]);
    strcat(rutaVerificacion, "informe.txt");
    // Creamos fichero informe.txt
    mi_creat(rutaVerificacion, 6);
    // Inicializamos array de entradas
    struct entrada entradas[NUMPROCESOS * sizeof(struct entrada)];
    memset(entradas, 0, sizeof(entradas));
    mi_read(argv[2], entradas, 0, NUMPROCESOS * sizeof(struct entrada));

    int offsetInforme = 0;

    char *pidbarra;
    unsigned int pid;
    int cant_registros_buffer_escrituras = 256;
    struct REGISTRO buffer_escrituras[cant_registros_buffer_escrituras];
    struct INFORMACION info;

    for (int i = 0; i < NUMPROCESOS; i++)
    {
        pidbarra = strchr(entradas[i].nombre, '_');
        // Desplazamos puntero para sacar el PID
        pidbarra++;
        pid = atoi(pidbarra);

        // Inicializamos buffer
        memset(buffer_escrituras, 0, sizeof(buffer_escrituras));
        int offset = 0;

        // Creamos string del fichero que vamos a leer
        char rutaProceso[100];
        sprintf(rutaProceso, "%sproceso_%d/prueba.dat", argv[2], pid);

        // Inicialitzamos struct info
        info.pid = pid;
        info.nEscrituras = 0;
        info.PrimeraEscritura.fecha = time(NULL);
        info.PrimeraEscritura.nEscritura = NUMESCRITURAS;
        info.PrimeraEscritura.nRegistro = 0;
        info.UltimaEscritura.fecha = 0;
        info.UltimaEscritura.nEscritura = 0;
        info.UltimaEscritura.nRegistro = 0;
        info.MenorPosicion.fecha = time(NULL);
        info.MenorPosicion.nEscritura = 0;
        info.MenorPosicion.nRegistro = REGMAX;
        info.MayorPosicion.fecha = time(NULL);
        info.MayorPosicion.nEscritura = 0;
        info.MayorPosicion.nRegistro = 0;

        // Bucle que recorre el fichero
        while ((mi_read(rutaProceso, buffer_escrituras, offset, sizeof(buffer_escrituras)) > 0))
        {
            // Recorrem todo el buffer de lectura.
            for (int i = 0; i < cant_registros_buffer_escrituras; i++)
            {
                // Comporbamos que sea válida
                if (buffer_escrituras[i].pid == info.pid)
                {

                    // Primera escritura (menor posición)
                    if (info.MenorPosicion.nEscritura == 0)
                    {
                        info.MenorPosicion.nEscritura = buffer_escrituras[i].nEscritura;
                        info.MenorPosicion.nRegistro = buffer_escrituras[i].nRegistro;
                        info.MenorPosicion.fecha = buffer_escrituras[i].fecha;
                    }
                    else
                    {
                        // Comprobamos si la que hemos leído es la primera
                        if (buffer_escrituras[i].nEscritura < info.PrimeraEscritura.nEscritura)
                        {
                            info.PrimeraEscritura.nEscritura = buffer_escrituras[i].nEscritura;
                            info.PrimeraEscritura.nRegistro = buffer_escrituras[i].nRegistro;
                            info.PrimeraEscritura.fecha = buffer_escrituras[i].fecha;
                        }
                        // Comprobamos si la que hemos leído es la última
                        else if (buffer_escrituras[i].nEscritura > info.UltimaEscritura.nEscritura)
                        {
                            info.UltimaEscritura.nEscritura = buffer_escrituras[i].nEscritura;
                            info.UltimaEscritura.nRegistro = buffer_escrituras[i].nRegistro;
                            info.UltimaEscritura.fecha = buffer_escrituras[i].fecha;
                        }
                    }
                    // Aumentam en 1 la cantidad de entradas válidas
                    info.nEscrituras++;
                    // Obtenemos información sobre escritura de mayor posición
                    info.MayorPosicion.nEscritura = buffer_escrituras[i].nEscritura;
                    info.MayorPosicion.nRegistro = buffer_escrituras[i].nRegistro;
                    info.MayorPosicion.fecha = buffer_escrituras[i].fecha;
                }
            }
            // Volvemos a poner el búffer a 0 para la siguiente iteración
            memset(buffer_escrituras, 0, sizeof(buffer_escrituras));
            // Augmentamos offset para leer el siguiente trozo del fichero
            offset += sizeof(buffer_escrituras);
        }
        printf("%d escrituras validadas en %s \n", info.nEscrituras, rutaProceso);
        // Escribimos los datos del struct info en el fichero informe.txt

        char buffer1[500];
        char buffer2[100];
        memset(buffer1, 0, 500);
        memset(buffer2, 0, 100);
        sprintf(buffer1, "PID: %d\n", info.pid);
        sprintf(buffer2, "Numero escrituras: %d\n", info.nEscrituras);
        strcat(buffer1, buffer2);
        memset(buffer2, 0, 100);
        sprintf(buffer2, "Primera escritura\t%d\t%d\t%s", info.PrimeraEscritura.nEscritura, info.PrimeraEscritura.nRegistro, asctime(localtime(&info.PrimeraEscritura.fecha)));
        strcat(buffer1, buffer2);
        memset(buffer2, 0, 100);
        sprintf(buffer2, "Ultima escritura\t%d\t%d\t%s", info.UltimaEscritura.nEscritura, info.UltimaEscritura.nRegistro, asctime(localtime(&info.UltimaEscritura.fecha)));
        strcat(buffer1, buffer2);
        memset(buffer2, 0, 100);
        sprintf(buffer2, "Menor Posicion\t\t%d\t%d\t%s", info.MenorPosicion.nEscritura, info.MenorPosicion.nRegistro, asctime(localtime(&info.MenorPosicion.fecha)));
        strcat(buffer1, buffer2);
        memset(buffer2, 0, 100);
        sprintf(buffer2, "Mayor Posicion\t\t%d\t%d\t%s\n", info.MayorPosicion.nEscritura, info.MayorPosicion.nRegistro, asctime(localtime(&info.MayorPosicion.fecha)));
        strcat(buffer1, buffer2);
        // Introduim la informació recollida dins el fitxer.
        offsetInforme += mi_write(rutaVerificacion, buffer1, offsetInforme, strlen(buffer1));
    }
}
