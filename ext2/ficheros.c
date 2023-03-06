// Autors: Marc Melià Flexas, Pau Rosado Muñoz, Xavier Vives Marcus
#include "ficheros.h"
#include "debugging.h"

int mi_write_f(unsigned int ninodo, const void *buf_original, unsigned int offset, unsigned int nbytes)
{
    // Variable de retorno
    int nbytesEscritos = 0;
    // Este búfer lo usaremos más adelante
    char buf_bloque[BLOCKSIZE];

    // Leemos el inodo apuntado por ninodo
    struct inodo inodo;
    if (leer_inodo(ninodo, &inodo) == ERROR)
    {
        perror("ERROR: ");
        return ERROR;
    }

    // Comprobamos si tiene permisos de escritura
    if ((inodo.permisos & 2) != 2)
    {
        fprintf(stderr,"No hay permisos de lectura\n");
        return nbytesEscritos;
    }
    else
    {
        // Averiguamos el rango de bloques lógicos que habrá que escribir
        int primerBL = offset / BLOCKSIZE;
        int ultimoBL = (offset + nbytes - 1) / BLOCKSIZE;

        // Averiguamos los depslazamientos dentro del mismo bloque. Es decir, un offset puede ser (por ejemplo) 300. Por tanto
        // habrá que empezar a escribir a partir del byte 300 del primer BL.
        // Lo mismo pasa para el último BL que escribamos. No necesariamente habrá que escribir un bloque entero
        int desp1 = offset % BLOCKSIZE;
        int desp2 = (offset + nbytes - 1) % BLOCKSIZE;

        // Caso en el que primerBL==ultimoBL
        if (primerBL == ultimoBL)
        {
            mi_waitSem();
            // Obtenemos el num de bloque físico
            int nbfisico = traducir_bloque_inodo(ninodo, primerBL, 1);

            mi_signalSem();
            // Leemos el bloque
            if (bread(nbfisico, buf_bloque) == ERROR)
            {
                perror("ERROR: ");
                return ERROR;
            }

            // Copiamos el contenido que había que escribir
            memcpy(buf_bloque + desp1, buf_original, nbytes);

            // Lo escribimos en el dispositivo
            if (bwrite(nbfisico, buf_bloque) == ERROR)
            {
                perror("ERROR: ");
                return ERROR;
            }
            nbytesEscritos = nbytes;
        }
        // Caso en el que la operación de escritura afecta a más de un bloque
        else
        {
            mi_waitSem();
            // Leemos el primer bloque lógico
            int nbfisico = traducir_bloque_inodo(ninodo, primerBL, 1);
            mi_signalSem();
            if (bread(nbfisico, buf_bloque) == ERROR)
            {
                perror("ERROR: ");
                return ERROR;
            }
            // Copiamos lo que faltaba del primer BL
            memcpy(buf_bloque + desp1, buf_original, BLOCKSIZE - desp1);
            if (bwrite(nbfisico, buf_bloque) == ERROR)
            {
                perror("ERROR: ");
                return ERROR;
            }
            nbytesEscritos = BLOCKSIZE - desp1;

            // Escribimos los bloques intermedios
            for (int i = primerBL + 1; i < ultimoBL; i++)
            {
                mi_waitSem();
                // Obtenemos nbfisico apuntado por en i-ésimo bloque lógico
                nbfisico = traducir_bloque_inodo(ninodo, i, 1);
                mi_signalSem();
                bwrite(nbfisico, buf_original + (BLOCKSIZE - desp1) + (i - primerBL - 1) * BLOCKSIZE);
                // Actualizamos nbytesEscritos
                nbytesEscritos += BLOCKSIZE;
            }
            mi_waitSem();
            // Último bloque lógico
            nbfisico = traducir_bloque_inodo(ninodo, ultimoBL, 1);
            mi_signalSem();
            // Leemos lo que había antes
            if (bread(nbfisico, buf_bloque) == ERROR)
            {
                perror("ERROR: ");
                return ERROR;
            }
            // Sobreescribimos la parte que queremos
            memcpy(buf_bloque, buf_original + (nbytes - desp2 - 1), desp2 + 1);

            // Lo escribimos en el dispositivo
            if (bwrite(nbfisico, &buf_bloque) == ERROR)
            {
                perror("ERROR: ");
                return ERROR;
            }
            nbytesEscritos += desp2 + 1;
        }
    }
    mi_waitSem();
    // Actualizamos los atributos del inodo
    if (leer_inodo(ninodo, &inodo) == ERROR)
    {
        perror("ERROR: ");
        return ERROR;
    }
    // Si hemos escrito al menos 1 byte, actualizamos el mtime
    if (nbytesEscritos > 0)
    {
        inodo.mtime = time(NULL);
    }

    // Actualizamos tamEnBytesLog y ctime si hemos escrito más allá del final de fichero
    if (offset + nbytesEscritos > inodo.tamEnBytesLog)
    {
        inodo.tamEnBytesLog = offset + nbytesEscritos;
        inodo.ctime = time(NULL);
    }

    // Escribimos el inodo ahora actualizado
    if (escribir_inodo(ninodo, &inodo) == ERROR)
    {
        perror("ERROR: ");
        return ERROR;
    }
    mi_signalSem();
    // Devolvemos la cantidad de bytes que hemos escrito
    return nbytesEscritos;
}

int mi_read_f(unsigned int ninodo, void *buf_original, unsigned int offset, unsigned int nbytes)
{
    struct inodo inodo;
    int nbytesLeidos = 0;
    char buf_bloque[BLOCKSIZE];
    int nbfisico;
    
    mi_waitSem();
    // Leemos el inodo
    if (leer_inodo(ninodo, &inodo) == ERROR)
    {
        mi_signalSem();
        perror("ERROR: ");
        return ERROR;
    }
    // Puesto que hemos accedido al inodo, actualizamos el atime
    inodo.atime = time(NULL);
    if ((escribir_inodo(ninodo, &inodo)) == ERROR)
    {
        mi_signalSem();
        perror("ERROR: ");
        return ERROR;
    }
    mi_signalSem();
    // Comprobamos que tenga permisos de lectura
    if ((inodo.permisos & 4) != 4)
    {
        fprintf(stderr,"No hay permisos de lectura\n");
        return nbytesLeidos;
    }
    else
    {
        // Comprobamos que no queramos leer más allá del EOF
        if (offset >= inodo.tamEnBytesLog)
        {
            return nbytesLeidos;
        }
        // Comprobamos  si pretende leer más allá del EOF
	else if ((offset + nbytes) >= inodo.tamEnBytesLog)
        {
            nbytes = inodo.tamEnBytesLog - offset;
        }

        // Limpiamos el buf_original
        memset(buf_original, 0, nbytes);

        // Averiguamos el rango de bloques lógicos que habrá que leer
        int primerBL = offset / BLOCKSIZE;
        int ultimoBL = (offset + nbytes - 1) / BLOCKSIZE;

        // Averiguamos los depslazamientos dentro del mismo bloque. Es decir, un offset puede ser (por ejemplo) 300. Por tanto
        // habrá que empezar a leere a partir del byte 300 del primer BL.
        // Lo mismo pasa para el último BL que escribamos. No necesariamente habrá que leer un bloque entero
        int desp1 = offset % BLOCKSIZE;
        int desp2 = (offset + nbytes - 1) % BLOCKSIZE;

        // Caso en el que solo tenemos que leer un bloque o parte de un bloque
        if (primerBL == ultimoBL)
        {
            // Obtenemos el num de bloque físico
            nbfisico = traducir_bloque_inodo(ninodo, primerBL, 0);

            // Nos aseguramos de que el inodo tenga un BF para dicho BL
            if (nbfisico != ERROR)
            {
                // Leemos el contenido
                if (bread(nbfisico, buf_bloque) == ERROR)
                {
                    perror("ERROR: ");
                    return ERROR;
                }
                // Copiamos lo que nos interesa
                memcpy(buf_original, buf_bloque + desp1, nbytes);
            }
            nbytesLeidos = nbytes;
            return nbytesLeidos;
        }
        // Caso en el que toca leer más de un bloque
        else
        {
            nbfisico = traducir_bloque_inodo(ninodo, primerBL, 0);

            // Comprobamos si existe un BF para dicho BL
            if (nbfisico != ERROR)
            {
                // Leemos el bloque
                if (bread(nbfisico, buf_bloque) == ERROR)
                {
                    perror("ERROR: ");
                    return ERROR;
                }
                // Copiamos lo que nos interesa
                memcpy(buf_original, buf_bloque + desp1, BLOCKSIZE - desp1);
            }
            nbytesLeidos += BLOCKSIZE - desp1;

            // Para los bloques intermedios entre primerBL y ultimoBL
            for (int i = primerBL + 1; i < ultimoBL; i++)
            {
                // Obtenemos direccion de nbfisco para dicho BL
                nbfisico = traducir_bloque_inodo(ninodo, i, 0);

                // Comprobamos si tiene asignado dicho BF
                if (nbfisico != ERROR)
                {
                    // Leemos dicho bloque
                    if (bread(nbfisico, buf_bloque) == ERROR)
                    {
                        perror("ERROR: ");
                        return ERROR;
                    }
                    memcpy(buf_original + (BLOCKSIZE - desp1) + (i - primerBL - 1) * BLOCKSIZE, buf_bloque, BLOCKSIZE);
                }
                nbytesLeidos += BLOCKSIZE;
            }
            // Para el último bloque o fragmento de bloque que tengamos que leer
            nbfisico = traducir_bloque_inodo(ninodo, ultimoBL, 0);

            // Comprobamos si existe un BF asignado para tal BL
            if (nbfisico != ERROR)
            {
                // Leemos el bloque
                if (bread(nbfisico, buf_bloque) == ERROR)
                {
                    perror("ERROR: ");
                    return ERROR;
                }

                memcpy(buf_original + (nbytes - desp2 - 1), buf_bloque, desp2 + 1);
            }
            // Actualizamos el número de bytes leídos
            nbytesLeidos += desp2 + 1;
            return nbytesLeidos;
        }
    }
    // Por si acaso
    return ERROR;
}

int mi_stat_f(unsigned int ninodo, struct STAT *p_stat)
{

    struct inodo inodo;
    if (leer_inodo(ninodo, &inodo) < 0)
    {
        perror("Error");
        return -1;
    }

    p_stat->tipo = inodo.tipo;
    p_stat->permisos = inodo.permisos;

    p_stat->atime = inodo.atime;
    p_stat->mtime = inodo.mtime;
    p_stat->ctime = inodo.ctime;

    p_stat->nlinks = inodo.nlinks;
    p_stat->tamEnBytesLog = inodo.tamEnBytesLog;
    p_stat->numBloquesOcupados = inodo.numBloquesOcupados;

    return 0;
}

int mi_chmod_f(unsigned int ninodo, unsigned char permisos)
{

    struct inodo inodo;
    mi_waitSem();
    // Leemos el inodo del correspondiente nº de inodo
    if (leer_inodo(ninodo, &inodo) < 0)
    {
        perror("Error");
        return -1;
    }

    // Cambiamos los permisos
    inodo.permisos = permisos;

    // Actualizamos ctime
    inodo.ctime = time(NULL);

    if (escribir_inodo(ninodo, &inodo) < 0)
    {
        perror("Error");
        return -1;
    }
    mi_signalSem();
    return 0;
}
int mi_truncar_f(unsigned int ninodo, unsigned int nbytes)
{
    // Variable que retornaremos
    unsigned int bliberados = 0;
    // Leemos el inodo
    struct inodo inodo;
    if (leer_inodo(ninodo, &inodo) == ERROR)
    {
        perror("ERROR: ");
        return ERROR;
    }
    // Comprobamos que tenga permisos de escritura
    if ((inodo.permisos & 2) != 2)
    {
        printf("[truncar_f(): ERROR DE PERMISOS]");
        return bliberados;
    }
    else
    {

        // Si la cantidad de bytes lógicos que hemos de liberar no cabe dentro de un número entero de bloques
        // (nbytes%BLOCKSIZE !=0), liberaremos a partir del primer bloque completo que tengamos que liberar.
        // Es decir, los bloques a medias los dejamos como estaban
        int primerBL;
        if (nbytes % BLOCKSIZE == 0)
        {
            primerBL = nbytes / BLOCKSIZE;
        }
        else
        {
            primerBL = nbytes / BLOCKSIZE + 1;
        }

        // Liberamos desde primerBL
        bliberados = liberar_bloques_inodos(primerBL, &inodo);

        if (bliberados == ERROR)
        {
            perror("ERROR: ");
            return ERROR;
        }

        // Actualizamos el inodo
        inodo.tamEnBytesLog = nbytes;
        inodo.ctime = time(NULL);
        inodo.mtime = time(NULL);
        inodo.numBloquesOcupados = inodo.numBloquesOcupados - bliberados;

        // Escribimos el inodo ya actualizado
        if (escribir_inodo(ninodo, &inodo) == ERROR)
        {
            perror("ERROR: ");
            return ERROR;
        }

        return bliberados;
    }
}
