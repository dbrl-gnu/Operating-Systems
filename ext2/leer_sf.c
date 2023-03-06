// Autors: Marc Melià Flexas, Pau Rosado Muñoz, Xavier Vives Marcus
#include "directorios.h"
#include "debugging.h"

void mostrar_buscar_entrada(char *camino, char reservar);

void mostrar_buscar_entrada(char *camino, char reservar)
{
    unsigned int p_inodo_dir = 0;
    unsigned int p_inodo = 0;
    unsigned int p_entrada = 0;
    int error;
    printf("\ncamino: %s, reservar: %d\n", camino, reservar);
    if ((error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, reservar, 6)) < 0)
    {
        mostrar_error_buscar_entrada(error);
    }
    printf("*******************************************\n");
    return;
}


int main(int argc, char **argv)
{
    struct superbloque SB;
    char *camino = argv[1];
    if (argc != 2)
    {
        printf("ERROR: ");
    }
    if (bmount(camino) == ERROR)
    {
        return ERROR;
    }
    if (bread(0, &SB) != ERROR)
    {

        // mostrar el superbloque

        printf("DATOS DEL SUPERBLOQUE\n");
        printf("posPrimerBloqueMB: %u\n", SB.posPrimerBloqueMB);
        printf("posUltimoBloqueMB: %u\n", SB.posUltimoBloqueMB);
        printf("posPrimerBloqueAI: %u\n", SB.posPrimerBloqueAI);
        printf("posUltimoBloqueMB: %u\n", SB.posUltimoBloqueAI);
        printf("posPrimerBloqueDatos: %u\n", SB.posPrimerBloqueDatos);
        printf("posUltimoBloqueDatos: %u\n", SB.posUltimoBloqueDatos);
        printf("posInodoRaiz: %u\n", SB.posInodoRaiz);
        printf("posPrimerInodoLibre: %u\n", SB.posPrimerInodoLibre);
        printf("cantBloquesLibres: %u\n", SB.cantBloquesLibres);
        printf("cantInodosLibres: %u\n", SB.cantInodosLibres);
        printf("totBloques: %u\n", SB.totBloques);
        printf("totInodos: %u\n\n", SB.totInodos);

#if DEBUGN2
        printf("sizeof struc superbloque: %lu\n", sizeof(struct superbloque));
        printf("sizeof struc inodo is: %lu\n", sizeof(struct inodo));
        struct inodo inodos[BLOCKSIZE / INODOSIZE];
        for (int i = SB.posPrimerInodoLibre; i < SB.cantInodosLibres; i++)
        {
            if (i % (BLOCKSIZE / INODOSIZE) == 0)
            {
                if (bread(SB.posPrimerBloqueAI + i / (BLOCKSIZE / INODOSIZE), inodos) == ERROR)
                {
                    perror("ERROR: ");
                    return ERROR;
                }
            }
            printf("%d ", inodos[i % (BLOCKSIZE / INODOSIZE)].punterosDirectos[0]);
        }
#endif

#if DEBUGN3
        printf("RESERVAMOS UN BLOQUE Y LUEGO LO LIBERAMOS\n");
        int nbloque = reservar_bloque();
        // Actualizamos datos del SB local
        if (bread(0, &SB) == ERROR)
        {
            perror("ERROR: ");
            return ERROR;
        }

        printf("Se ha reservado el bloque físico nº %i que era el %iº libre indicado por el MB\n", nbloque,
               ((nbloque - SB.posPrimerBloqueDatos) + 1));

        printf("SB.cantBloquesLibres = %i\n", SB.cantBloquesLibres);
        if (liberar_bloque(nbloque) == ERROR)
        {
            perror("ERROR: ");
            return ERROR;
        }
        // Actualizamos los datos del SB local
        if (bread(0, &SB) == ERROR)
        {
            perror("ERROR: ");
            return ERROR;
        }
        printf("Liberamos ese bloque y después SB.cantBloquesLibres = %i\n", SB.cantBloquesLibres);
        // mostrar el MB (y así comprobar el funcionamiento de escribir_bit() y leer_bit()).
        printf("MAPA DE BITS CON BLOQUES DE METADATOS OCUPADOS\n");
        for (int i = 0; i < SB.totBloques; i++)
        {
            leer_bit(i);
        }

        // Imprimir los datos del directorio raíz
        struct inodo dirRaiz;
        leer_inodo(SB.posInodoRaiz, &dirRaiz);

        printf("\n\nDATOS DEL DIRECTORIO RAIZ\n");
        printf("tipo: %c\n", dirRaiz.tipo);
        printf("permisos: %u\n", dirRaiz.permisos);
        ts = localtime(&dirRaiz.atime);
        strftime(atime, sizeof(atime), "%a %Y-%m-%d %H:%M:%S", ts);
        ts = localtime(&dirRaiz.mtime);
        strftime(mtime, sizeof(mtime), "%a %Y-%m-%d %H:%M:%S", ts);
        ts = localtime(&dirRaiz.ctime);
        strftime(ctime, sizeof(ctime), "%a %Y-%m-%d %H:%M:%S", ts);
        printf("ID: %d ATIME: %s MTIME: %s CTIME: %s\n", SB.posInodoRaiz, atime, mtime, ctime);
        printf("nlinks: %u\n", dirRaiz.nlinks);
        printf("tamEnBytesLog: %u\n", dirRaiz.tamEnBytesLog);
        printf("numBloquesOcupados: %u\n", dirRaiz.numBloquesOcupados);
#endif

#if DEBUGN4
        struct tm *ts;
        char atime[80];
        char mtime[80];
        char ctime[80];

        struct inodo inodoN4;
        unsigned int ninodoN4 = reservar_inodo('f', 6);

        if (ninodoN4 < 0)
        {
            perror("ERROR: ");
            return ERROR;
        }
        if (leer_inodo(ninodoN4, &inodoN4) == ERROR)
        {
            perror("ERROR: ");
            return ERROR;
        }

        printf("INODO %u. TRADUCCION DE LOS BLOQUES LOGICOS 8, 204, 30.004, 400.004 Y 468.750\n", ninodoN4);
        traducir_bloque_inodo(ninodoN4, 8, 1);
        traducir_bloque_inodo(ninodoN4, 204, 1);
        traducir_bloque_inodo(ninodoN4, 30004, 1);
        traducir_bloque_inodo(ninodoN4, 400004, 1);
        traducir_bloque_inodo(ninodoN4, 468750, 1);
        if (leer_inodo(ninodoN4, &inodoN4) == ERROR)
        {
            perror("ERROR: ");
            return ERROR;
        }
        printf("\n\nDATOS DEL INODO RESERVADO %u\n", ninodoN4);
        printf("tipo: %c\n", inodoN4.tipo);
        printf("permisos: %u\n", inodoN4.permisos);
        ts = localtime(&inodoN4.atime);
        strftime(atime, sizeof(atime), "%a %Y-%m-%d %H:%M:%S", ts);
        ts = localtime(&inodoN4.mtime);
        strftime(mtime, sizeof(mtime), "%a %Y-%m-%d %H:%M:%S", ts);
        ts = localtime(&inodoN4.ctime);
        strftime(ctime, sizeof(ctime), "%a %Y-%m-%d %H:%M:%S", ts);
        printf("atime: %s\nmtime: %s\nctime: %s\n", atime, mtime, ctime);
        printf("nlinks: %u\n", inodoN4.nlinks);
        printf("tamEnBytesLog: %u\n", inodoN4.tamEnBytesLog);
        printf("numBloquesOcupados: %u\n", inodoN4.numBloquesOcupados);

        if (bread(0, &SB) == ERROR)
        {
            perror("ERROR: ");
            return ERROR;
        }
        printf("\n\nSB.posPrimerInodoLibre = %u\n",SB.posPrimerInodoLibre);
#endif
#if DEBUGN7
        // Mostrar creación directorios y errores
        mostrar_buscar_entrada("pruebas/", 1);           // ERROR_CAMINO_INCORRECTO
        mostrar_buscar_entrada("/pruebas/", 0);          // ERROR_NO_EXISTE_ENTRADA_CONSULTA
        mostrar_buscar_entrada("/pruebas/docs/", 1);     // ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO
        mostrar_buscar_entrada("/pruebas/", 1);          // creamos /pruebas/
        mostrar_buscar_entrada("/pruebas/docs/", 1);     // creamos /pruebas/docs/
        mostrar_buscar_entrada("/pruebas/docs/doc1", 1); // creamos /pruebas/docs/doc1
        mostrar_buscar_entrada("/pruebas/docs/doc1/doc11", 1);
        // ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_UN_FICHERO
        mostrar_buscar_entrada("/pruebas/", 1);          // ERROR_ENTRADA_YA_EXISTENTE
        mostrar_buscar_entrada("/pruebas/docs/doc1", 0); // consultamos /pruebas/docs/doc1
        mostrar_buscar_entrada("/pruebas/docs/doc1", 1); // creamos /pruebas/docs/doc1
        mostrar_buscar_entrada("/pruebas/casos/", 1);    // creamos /pruebas/casos/
        mostrar_buscar_entrada("/pruebas/docs/doc2", 1); // creamos /pruebas/docs/doc2
#endif
    }
    else
    {
        perror("ERROR");
        return ERROR;
    }
    if (bumount() == ERROR)
    {
        return ERROR;
    }

    return 0;
}