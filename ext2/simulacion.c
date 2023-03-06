// Autors: Marc Melià Flexas, Pau Rosado Muñoz, Xavier Vives Marcus
#include "simulacion.h"
static int acabados=0;


int main(int argc, char **argv)
{
    // Variables para dar nombre a nuestro directorio
    char directorioSimulacion[30];
    time_t tiempo = time(NULL);
    struct tm *tm = localtime(&tiempo);

    // Asociamos señal de final de proceso a la función enterrador
    signal(SIGCHLD, reaper);

    if (argc != 2)
    {
        printf("Sintaxis: ./simulacion <disco>");
        return 0;
    }
    // Montar el dispositivo
    bmount(argv[1]);

    // Creamos el nombre del dispositivo
    sprintf(directorioSimulacion, "/simul_%d%02d%02d%02d%02d%02d/", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

    // Creamos directirio de simulacion
    mi_creat(directorioSimulacion, 6);

    for (int proceso = 1; proceso <= NUMPROCESOS; proceso++)
    {
        pid_t pid = fork();

        // Caso hijo
        if (pid == 0)
        {
            bmount(argv[1]);
            char directorioProceso[50];
            char ficheroProceso[60];

            // Creamos nombre del directorio i fichero para el proceso i-ésimo
            sprintf(directorioProceso, "%sproceso_%d/", directorioSimulacion, getpid());
            sprintf(ficheroProceso, "%sprueba.dat", directorioProceso);

            // Creamos el directorio
            mi_creat(directorioProceso, 6);
            // Creamos el fichero
            mi_creat(ficheroProceso, 6);

            // Inicializamos semilla para el proceso i-ésimo
            srand(time(NULL) + getpid());

            struct REGISTRO registro;
            memset(&registro, 0, sizeof(struct REGISTRO));

            int nescritura;
            for (nescritura = 1; nescritura <= NUMESCRITURAS; nescritura++)
            {
                registro.fecha = time(NULL);
                registro.pid = getpid();
                registro.nEscritura = nescritura;
                registro.nRegistro = rand() % REGMAX;

                mi_write(ficheroProceso, &registro, registro.nRegistro * sizeof(struct REGISTRO), sizeof(struct REGISTRO));
                printf(ANSI_COLOR_BLUE "[simulacion.c -> Escritura %d en %s]\n",nescritura,ficheroProceso);
                
                // Esperamos 0,05 segundos para la siguiente escritura
                usleep(50000);
            }
            printf(ANSI_COLOR_RED "[Proceso %d: Completadas %d escrituras en %sprueba.dat] \n", proceso, (nescritura-1), directorioProceso);
            // Desmontamos dispositivo
            bumount();
            // Para enviar la señal SIGCHLD
            exit(0);
        }
        // Esperamos 0,15 segundos para lanzar el siguiente proceso
        usleep(150000);
    }

    // Para que el padre espere a que se entierren sus hijos
    while(acabados<NUMPROCESOS){
        pause();
    }

    bumount();
    return EXIT_SUCCESS;
}

void reaper()
{
    pid_t ended;
    signal(SIGCHLD, reaper);
    while ((ended = waitpid(-1, NULL, WNOHANG)) > 0)
    {
        acabados++;
    }
}