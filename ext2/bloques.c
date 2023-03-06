// Autors: Marc Melià Flexas, Pau Rosado Muñoz, Xavier Vives Marcus
#include "bloques.h"
#include "debugging.h"
#include "semaforo_mutex_posix.h"

int descriptor;
static sem_t *mutex;
static unsigned int inside_sc = 0;

int bmount(const char *camino)
{
    if (descriptor > 0)
    {
        close(descriptor);
    }
    umask(000);
    if ((descriptor = open(camino, O_RDWR | O_CREAT, 0666)) == -1)
    {
        perror("ERROR: ");
        return ERROR;
    }
    if (!mutex)
    { // el semáforo es único en el sistema y sólo se ha de inicializar 1 vez (padre)
        mutex = initSem();
        if (mutex == SEM_FAILED)
        {
            return -1;
        }
    }
    return descriptor;
}
int bumount()
{
    descriptor = close(descriptor);
    if (descriptor == -1)
    {
        perror("ERROR: ");
        return ERROR;
    }
    deleteSem();
    return 0;
}
int bwrite(unsigned int nbloque, const void *buf)
{
    int rw;
    if (lseek(descriptor, nbloque * BLOCKSIZE, SEEK_SET) == -1)
    {
        perror("ERROR: ");
        return ERROR;
    }
    if ((rw = write(descriptor, buf, BLOCKSIZE)) == -1)
    {
        perror("ERROR: ");
        return ERROR;
    }
    return rw;
}
int bread(unsigned int nbloque, void *buf)
{
    int wr;
    if (lseek(descriptor, nbloque * BLOCKSIZE, SEEK_SET) == -1)
    {
        perror("ERROR: ");
        return ERROR;
    }
    if ((wr = read(descriptor, buf, BLOCKSIZE)) == -1)
    {
        perror("ERROR: ");
        return ERROR;
    }
    return wr;
}
void mi_waitSem()
{
    if (!inside_sc)
    { // inside_sc==0
        waitSem(mutex);
    }
    inside_sc++;
}

void mi_signalSem()
{
    inside_sc--;
    if (!inside_sc)
    {
        signalSem(mutex);
    }
}