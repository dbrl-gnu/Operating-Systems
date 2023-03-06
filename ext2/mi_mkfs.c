// Autors: Marc Melià Flexas, Pau Rosado Muñoz, Xavier Vives Marcus
#include "ficheros_basico.h"
int main(int argc, char **argv)
{
    // Comprobamos que se haya introducido el comando correctamente
    if (argc == 3)
    {
        char *camino = argv[1];
        int nbloques = atoi(argv[2]);
        void *buf=malloc(BLOCKSIZE);
        int fd;

        memset(buf, 0, BLOCKSIZE);

    fd=(bmount(camino));
    if(fd!=-1){
        // Inicializamos los bloques del dispositivo todo a 0
            for (int i = 0; i < nbloques; i++)
            {
                bwrite(i,buf);
            }
            initSB(nbloques, nbloques/4);
            initMB();
            initAI();
            reservar_inodo('d',7);
            if(bumount(fd) == -1){
                perror("ERROR");
            }
        }
    }else{
        perror("ERROR");
    }
}
