// Autors: Marc Melià Flexas, Pau Rosado Muñoz, Xavier Vives Marcus
#include "ficheros.h"
int main(int argc, char **argv)
{
    // Comprobamos que se haya introducido el comando correctamente
    if (argc == 4)
    {
        struct tm *ts;
        char atime[80];
        char mtime[80];
        char ctime[80];
        char *camino = argv[1];
        int ninodo = atoi(argv[2]);
        int nbytes = atoi(argv[3]);
        struct STAT p_stat;
        struct superbloque SB;
        if (bmount(camino) == -1)
        {
            perror("ERROR");
            return -1;
        }
        if (bread(0, &SB) != -1)
        {
            // Inicializamos los bloques del dispositivo todo a 0
            if(nbytes == 0) {
                liberar_inodo(ninodo);
            } else {
                mi_truncar_f(ninodo,nbytes);
            }
            mi_stat_f(ninodo, &p_stat);
            printf("\n");
            printf("DATOS INODO %u:\n",ninodo);
            printf("tipo = %c\n",p_stat.tipo);
            printf("permisos = %u\n",p_stat.permisos);
            ts = localtime(&p_stat.atime);
             strftime(atime, sizeof(atime), "%a %Y-%m-%d %H:%M:%S", ts);
            ts = localtime(&p_stat.mtime);
            strftime(mtime, sizeof(mtime), "%a %Y-%m-%d %H:%M:%S", ts);
            ts = localtime(&p_stat.ctime);
            strftime(ctime, sizeof(ctime), "%a %Y-%m-%d %H:%M:%S", ts);
            printf("atime %s:\n",atime);
            printf("ctime %s:\n",ctime);
            printf("mtime %s:\n",mtime);
            printf("nlinks = %u\n", p_stat.nlinks);
            printf("tamEnBytesLog = %u\n",p_stat.tamEnBytesLog);
            printf("numBloquesOcupados = %u\n",p_stat.numBloquesOcupados);
            printf("\n");
        }else
        {
            perror("ERROR");
            return -1;
        }
        if(bumount() == -1)
        {
            perror("ERROR");
            return -1;
        }
    }else{
        printf("Sintaxis: truncar <nombre_dispositivo><ninodo><nbytes>\n");
        return -1;
    }
}
