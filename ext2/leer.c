// Autors: Marc Melià Flexas, Pau Rosado Muñoz, Xavier Vives Marcus
#include "ficheros.h"
int main(int argc, char **argv)
{
    if (argc != 3)
    {
        printf("Sintaxis: leer <nombre_dispositivo><ninodos>\n");
        return -1;
    } 
    struct superbloque SB;
    struct STAT p_stat;
    char string[128];
    char *camino = argv[1];
    unsigned int ninodo = atoi(argv[2]);
    unsigned int leidos, offset = 0;
    unsigned int t_leidos = 0;
    int tambuffer = 1500;

    void *buffer_texto = malloc(tambuffer);
    memset(buffer_texto, 0, tambuffer);
    if (bmount(camino) == -1)
    {
        return -1;
    }
    if (bread(0, &SB) != -1)
    {
        leidos = mi_read_f(ninodo, buffer_texto, offset, tambuffer);
        while (leidos > 0)
        {
            write(1, buffer_texto ,leidos);
            offset = offset + tambuffer;
            memset(buffer_texto, 0, tambuffer);
            t_leidos  += leidos;
            leidos = mi_read_f(ninodo, buffer_texto, offset, tambuffer);
        }
        sprintf(string, "\ntotal_leidos %d\n", t_leidos);
        write(2, string, strlen(string));
        mi_stat_f(ninodo, &p_stat);
        sprintf(string, "tamEnBytesLog %d\n", p_stat.tamEnBytesLog);
        write(2, string, strlen(string));
    }else
    {
        perror("ERROR");
        return -1;
    }
    if (bumount() == -1)
    {
        return -1;
    }
    return 0;
}