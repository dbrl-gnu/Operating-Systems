// Autors: Marc Melià Flexas, Pau Rosado Muñoz, Xavier Vives Marcus
#include "ficheros.h"

int main(int argc, char **argv)
{
    if (argc != 4)
    {
        printf("Sintaxis: escribir <nombre_dispositivo><'$(cat fichero)'><diferentes_inodos>\n");
        printf("Offsets: 9000,209000,30725000,409605000,480000000\n");
        printf("Si diferentes_inodos = 0 se reserva un solo inodo para los offets\n");
        return -1;
    }
    struct STAT p_stat;
    struct superbloque SB;
    char *camino = argv[1];
    int diferentes_inodos = atoi(argv[3]);
    long unsigned int offsets[] = {9000,209000,30725000,409605000,480000000};
    unsigned int ninodo, bytes_es;
    unsigned int nbytes = strlen(argv[2]);
    char *buf_original = malloc(nbytes);
    strcpy(buf_original, argv[2]);
    if (bmount(camino) == -1)
    {
        perror("ERROR");
        return -1;
    }
    printf("longitud texto: %u\n", nbytes);
    if (bread(0, &SB) != -1)
    {
        if (diferentes_inodos == 0)
        {
            ninodo = reservar_inodo('f',6);
            for (int i = 0; i < 5; i++){
                printf("\n");
                printf("Nº inodo reservado: %u\n", ninodo);
                printf("offset: %lu\n", offsets[i]);
                bytes_es = mi_write_f(ninodo, buf_original, offsets[i], nbytes);
                printf("Bytes escritos: %u\n" ,bytes_es);
                mi_stat_f(ninodo, &p_stat);
                printf("stat.tamEnBytesLog = %u\n",p_stat.tamEnBytesLog);
                printf("stat.numBloquesOcupados = %u\n",p_stat.numBloquesOcupados);
            }
        } else {
            for (int i = 0; i < 5; i++){
                ninodo = reservar_inodo('f',6);
                printf("\n");
                printf("Nº inodo reservado: %u\n", ninodo);
                printf("offset: %lu\n", offsets[i]);
                bytes_es = mi_write_f(ninodo, buf_original, offsets[i], nbytes);
                printf("Bytes escritos: %u\n" ,bytes_es);
                mi_stat_f(ninodo, &p_stat);
                printf("stat.tamEnBytesLog = %u\n",p_stat.tamEnBytesLog);
                printf("stat.numBloquesOcupados = %u\n",p_stat.numBloquesOcupados);
            }
        }
    }else
    {
        perror("ERROR");
        return -1;
    }
    if (bumount() == -1)
    {
        perror("ERROR");
        return -1;
    }
    return 0;
}
