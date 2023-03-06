// Autors: Marc Melià Flexas, Pau Rosado Muñoz, Xavier Vives Marcus
#include "debugging.h"
#include "ficheros_basico.h"
#define ERROR -1

int tamMB(unsigned int nbloques)
{
    int tam = (nbloques / 8) / BLOCKSIZE;
    if ((nbloques / 8) % BLOCKSIZE != 0)
    {
        return (tam + 1);
    }
    return tam;
}

int tamAI(unsigned int ninodos)
{
    int tam = (ninodos * INODOSIZE) / BLOCKSIZE;
    if ((ninodos * INODOSIZE) % BLOCKSIZE != 0)
    {
        return (tam + 1);
    }
    return tam;
}

int initMB()
{
    struct superbloque SB;
    // Declaramos un búfer tan grande como un bloque
    unsigned char *buf = malloc(BLOCKSIZE);
    // Contenido de buffer = todo 0s
    memset(buf, 0, BLOCKSIZE);

    // Leemos el superbloque para obtener la localización del array de inodos.
    if (bread(0, &SB) == -1)
    {

        perror("Error");
        return -1;
    }

    // Ponemos todo el MB a 0s
    for (int i = SB.posPrimerBloqueMB; i < SB.posUltimoBloqueMB; i++)
    {
        if (bwrite(i, buf) == -1)
        {
            perror("Error");
            return -1;
        }
    }
    // Ponemos a 1 los bits pertenecientes a los metadatos (AI, SB y MB)
    actualizarBitsMetadatosMB();

    return 0;
}

int initSB(unsigned int nbloques, unsigned int ninodos)
{
    // Inicializamos el superbloque
    struct superbloque SB;
    SB.posPrimerBloqueMB = posSB + tamSB;
    SB.posUltimoBloqueMB = SB.posPrimerBloqueMB + tamMB(nbloques) - 1;
    SB.posPrimerBloqueAI = SB.posUltimoBloqueMB + 1;
    SB.posUltimoBloqueAI = SB.posPrimerBloqueAI + tamAI(ninodos) - 1;
    SB.posPrimerBloqueDatos = SB.posUltimoBloqueAI + 1;
    SB.posUltimoBloqueDatos = nbloques - 1;
    SB.posInodoRaiz = 0;
    SB.posPrimerInodoLibre = 0;
    SB.cantBloquesLibres = nbloques;
    SB.cantInodosLibres = ninodos;
    SB.totBloques = nbloques;
    SB.totInodos = ninodos;

    if (bwrite(posSB, &SB) == ERROR)
    {
        perror("Error");
        return ERROR;
    }
    return 0;
}

int actualizarBitsMetadatosMB()
{
    struct superbloque SB;
    if (bread(posSB, &SB) == ERROR)
    {
        perror("ERROR: ");
        return ERROR;
    }

    // Número total de bloques que ocupan los metadatos
    int nBloquesTotalMetaDatos = tamAI(SB.totInodos) + tamMB(SB.totBloques) + tamSB;

    // Número total de bloques ENTEROS que hay que poner a 1
    int nBloquesMD = (nBloquesTotalMetaDatos / 8) / BLOCKSIZE;

    // Búfer que usaremos en seguida para escribir
    unsigned char bufferMB[BLOCKSIZE];

    // Escribimos los en el MB bloques enteros que correspondan a los metadatos
    if (nBloquesMD > 0)
    {
        memset(bufferMB, 255, sizeof(bufferMB));
        for (int i = SB.posPrimerBloqueMB; i < (SB.posPrimerBloqueMB + nBloquesMD); i++)
        {
            if (bwrite(i, bufferMB) == ERROR)
            {
                perror("ERROR: ");
                return ERROR;
            }
        }
    }
    // Por ahora hemos escrito todos los bloques enteros que deben estar a 1, falta poner a 1 los que han caído
    // entre y entre

    // búfer auxiliar
    unsigned char bufferAux[BLOCKSIZE];
    // Total de bytes ENTEROS que hay que poner a 1
    int nBytesMD = (nBloquesTotalMetaDatos / 8) % 1024;

    // Total de bits que hay que poner a 1 (será un número del 0-7)
    int nBitsMD = nBloquesTotalMetaDatos % 8;

    // Ponemos a 1 los bytes
    for (int i = 0; i < nBytesMD; i++)
    {
        bufferAux[i] = 255;
    }
    // Ponemos a 1 los bits
    if (nBitsMD != 0)
    {
        // Dependiendo de cuántos bits haya que poner a 1 se seleccionará una máscara u otra
        // 128 = 1000000
        // 192 = 1100000
        // 224 = 1110000
        // ...
        // 254 = 1111110

        // Nótese que no se tiene en cuenta 225 porque entonces tendríamos que poner 8 bits a 1
        // que es lo mismo que poner un byte a 1.
        unsigned char mascaras[] = {128, 192, 224, 240, 248, 252, 254};
        bufferAux[nBytesMD] = mascaras[nBitsMD - 1];
    }
    // Ponemos a 0 todos los otros bits
    for (int i = nBytesMD + 1; i < BLOCKSIZE; i++)
    {
        bufferAux[i] = 0;
    }
    // Escribimos este último bloque en el MB
    if (bwrite(SB.posPrimerBloqueMB + nBloquesMD, bufferAux) == ERROR)
    {
        perror("ERROR: ");
        return ERROR;
    }

    // Actualizamos SB
    SB.cantBloquesLibres = SB.cantBloquesLibres - nBloquesTotalMetaDatos;
    if (bwrite(0, &SB) == ERROR)
    {
        perror("ERROR: ");
        return ERROR;
    }

    return 0;
}

// Esta función se encargará de inicializar la lista de inodos libres.
int initAI()
{
    // Buffer para ir recorriendo el array de inodos
    struct inodo inodos[BLOCKSIZE / INODOSIZE];

    // Leemos el superbloque para obtener la localización del array de inodos.
    struct superbloque SB;
    if (bread(0, &SB) == ERROR)
    {
        perror("Error");
        return ERROR;
    }

    unsigned int contInodos = SB.posPrimerInodoLibre + 1;

    // Para cada bloque del array de inodos.
    for (int i = SB.posPrimerBloqueAI; i <= SB.posUltimoBloqueAI; i++)
    {

        // Leemos el bloque de inodos
        // És necessari llegir es bloque de inodos?
        if (bread(i, inodos) == ERROR)
        {

            perror("Error");
            return ERROR;
        }

        // Para cada inodo del array de inodos
        for (int j = 0; j < BLOCKSIZE / INODOSIZE; j++)
        {

            inodos[j].tipo = 'l'; // Indicamos que el tipo de inodo es libre ('l')

            // Si no hemos llegado al último inodo.
            if (contInodos < SB.totInodos)
            {

                // Enlazamos el inodo con el siguiente.
                inodos[j].punterosDirectos[0] = contInodos;
                contInodos++;
            }
            else
            {

                inodos[j].punterosDirectos[0] = UINT_MAX;
                break;
            }
        }

        // Escribimos el bloque de inodos i en el dispositivo virtual
        if (bwrite(i, inodos) == ERROR)
        {
            perror("Error");
            return ERROR;
        }
    }
    return 0;
}

int escribir_bit(unsigned int nbloque, unsigned int bit)
{
    // Leemos el superbloque y nos lo guardamos paro usarlo más tarde
    struct superbloque SB;
    if (bread(0, &SB) == ERROR)
    {
        perror("Error");
        return ERROR;
    }

    int posbyte = nbloque / 8;
    int posbit = nbloque % 8;

    // Nbloque relativo dentro del mapa de bits
    int nbloqueMB = posbyte / BLOCKSIZE;

    // nbloque real dentro del disco
    int nbloqueabs = SB.posPrimerBloqueMB + nbloqueMB;

    // Leemos el bloque
    unsigned char bufferMB[BLOCKSIZE];
    if (bread(nbloqueabs, bufferMB) == ERROR)
    {
        perror("Error");
        return ERROR;
    }

    // Calculamos el offset con respecto al bloque que hemos leído
    posbyte = posbyte % BLOCKSIZE;

    // Declaramos la máscara
    unsigned char mascara = 128; // 10000000
    // Desplazamos para que el 1 esté situado en la posición posbit-ésima
    mascara >>= posbit;

    // Escribimos el bit
    // Caso 1
    if (bit == 1)
    {
        bufferMB[posbyte] |= mascara;
    }
    // Caso 0
    else if (bit == 0)
    {
        bufferMB[posbyte] &= ~mascara;
    }
    // Caso Error
    else
    {
        perror("ERROR: ");
        return ERROR;
    }

    if (bwrite(nbloqueabs, bufferMB) == ERROR)
    {
        perror("ERROR: ");
        return ERROR;
    }
    return 0;
}

char leer_bit(unsigned int nbloque)
{
    // Leemos el superbloque y nos lo guardamos paro usarlo más tarde
    struct superbloque SB;
    if (bread(0, &SB) == ERROR)
    {
        perror("Error");
        return ERROR;
    }

    int posbyte = nbloque / 8;
    int posbit = nbloque % 8;

    // Nbloque relativo dentro del mapa de bits
    int nbloqueMB = posbyte / BLOCKSIZE;

    // nbloque real dentro del disco
    int nbloqueabs = SB.posPrimerBloqueMB + nbloqueMB;

    // Leemos el bloque
    unsigned char bufferMB[BLOCKSIZE];
    if (bread(nbloqueabs, bufferMB) == ERROR)
    {
        perror("Error");
        return ERROR;
    }
#if DEBUGN3
    printf("[leer_bit(%i)→ posbyte:%i, posbit:%i, nbloqueMB:%i, nbloqueabs:%i)]\n", nbloque, posbyte, posbit, nbloqueMB, nbloqueabs);
#endif
    // Calculamos el offset con respecto al bloque que hemos leído
    posbyte = posbyte % BLOCKSIZE;

    // Declaramos la máscara
    unsigned char mascara = 128; // 10000000

    // Movemos el 1 de la máscara para que esté encima del mismo bit que queremos leer
    mascara >>= posbit;
    // Evaluamos el valor del bit
    mascara &= bufferMB[posbyte];
    // Desplazamos para obtener resultado
    mascara >>= (7 - posbit);

// Print para el DEBUGGING. Perteneciente al debugging del nivel 3
#if DEBUGN3
    printf("leer_bit(%i) = %i\n", nbloque, mascara);
#endif

    // Devolvemos el resultado
    return mascara;
}

/*
La función reservar_bloque ncuentra el primer bloque libre, consultando el MB (primer bit a 0),
lo ocupa y devuelve su posición.
*/
int reservar_bloque()
{
    // Obtenemos el superbloque
    struct superbloque SB;
    if (bread(posSB, &SB) == ERROR)
    {
        perror("ERROR: ");
        return ERROR;
    }
    // CASO EN EL QUE QUEDAN BLOQUES LIBRES
    if (SB.cantBloquesLibres > 0)
    {
        // Búferes que usaremos para comprobar cuál es el primer bloque libre
        unsigned char bufferMB[BLOCKSIZE];
        unsigned char bufferAux[BLOCKSIZE];
        memset(bufferAux, 255, sizeof(bufferAux));

        // Comprobamos todos los bloques pertenecientes al MB
        int posBloqueMB = SB.posPrimerBloqueMB;
        for (;; posBloqueMB++)
        {
            // Leemos un bloque del MB
            if (bread(posBloqueMB, bufferMB) == ERROR)
            {
                perror("ERROR: ");
                return ERROR;
            }
            // Comprobamos si hay algun bit a 0
            if (memcmp(bufferMB, bufferAux, sizeof(bufferAux)) < 0)
            {
                // Si ya hemos encontrado el primer bit a 0, forzamos la salida del bucle for
                break;
            }
        }

        // Busamos el primer byte que tenga un 0
        int posbyte = 0;
        for (;; posbyte++)
        {
            if (bufferMB[posbyte] < 255)
            {
                // Si ya lo hemos encontrado, fuerza la salida del bucle for
                break;
            }
        }

        // Ahora ya tenemos el byte que tiene un 0 en alguna de sus posiciones. Falta saber en cuál de ellas es
        unsigned char mascara = 128;
        int posbit = 0;

        // Encontramos en cuál de las posiciones del byte está un 0
        while (bufferMB[posbyte] & mascara)
        {
            bufferMB[posbyte] <<= 1;

            posbit++;
        }
        // Ya sabemos cuál es el primer bit a 0

        // Obtenemos el número de bloque que representa este bit
        unsigned int nbloque = ((posBloqueMB - SB.posPrimerBloqueMB) * BLOCKSIZE + posbyte) * 8 + posbit;

        // Lo marcamos como reservado
        escribir_bit(nbloque, 1);

        // Actualizamos el SB
        SB.cantBloquesLibres--;
        if (bwrite(posSB, &SB) == ERROR)
        {
            perror("ERROR: ");
            return ERROR;
        }

        // Limpiamos el bloque
        unsigned char bufBloque[BLOCKSIZE];
        memset(bufBloque, 0, BLOCKSIZE);
        if (bwrite(nbloque, &bufBloque) == ERROR)
        {
            perror("ERROR: ");
            return ERROR;
        }
        return nbloque;
    }

    // CASO EN EL QUE NO QUEDAN BLOQUES LIBRES
    else
    {
        return ERROR;
    }
}
// La función liberar_que libera un bloque determinado por nbloque. uep
int liberar_bloque(unsigned int nbloque)
{

    // Ponemos a 0 el bit del MB correspondiente al bloque nbloque
    escribir_bit(nbloque, 0);

    // Leemos el superbloque
    struct superbloque SB;
    if (bread(posSB, &SB) == ERROR)
    {
        perror("Error");
        return ERROR;
    }

    // Incrementamos la cantidad de bloques libres en el superbloque.
    SB.cantBloquesLibres = SB.cantBloquesLibres + 1;

    // Salvamos el SB
    if (bwrite(posSB, &SB) == ERROR)
    {
        perror("Error");
        return ERROR;
    }

    // Devolvemos el nº de bloque liberado, nbloque.
    return nbloque;
}
int escribir_inodo(unsigned int ninodo, struct inodo *inodo)
{
    // Leemos el sjuperbloque
    struct superbloque SB;
    if (bread(0, &SB) == ERROR)
    {
        perror("Error");
        return ERROR;
    }
    // Calculamos el numero de bloque absoluto que le corresponde al inodo que queremos escribir
    unsigned int bloqueabs = ((ninodo * INODOSIZE) / BLOCKSIZE) + SB.posPrimerBloqueAI;

    struct inodo inodos[BLOCKSIZE / INODOSIZE];

    // Leemos el bloque en el que está el inodo
    if (bread(bloqueabs, inodos) == ERROR)
    {
        perror("Error");
        return ERROR;
    }
    // Sustituimos el inodo antiguo por el que queremos escribir
    inodos[ninodo % (BLOCKSIZE / INODOSIZE)] = *inodo;
    // Los escribimos en el dispositivo
    if (bwrite(bloqueabs, &inodos) == ERROR)
    {
        perror("Error: ");
        return ERROR;
    }
    return 0;
}

int leer_inodo(unsigned int ninodo, struct inodo *inodo)
{
    struct superbloque SB;
    if (bread(0, &SB) == ERROR)
    {
        perror("Error");
        return ERROR;
    }

    unsigned int numBloque = ((ninodo * INODOSIZE) / BLOCKSIZE) + SB.posPrimerBloqueAI;

    struct inodo inodos[BLOCKSIZE / INODOSIZE];

    if (bread(numBloque, inodos) == ERROR)
    {
        // Error en la lectura.
        perror("ERROR: ");
        return ERROR;
    }
    // Ponemos el inodo solicitado en el puntero pasado por parámetro
    *inodo = inodos[ninodo % (BLOCKSIZE / INODOSIZE)];
    return 0;
}
int reservar_inodo(unsigned char tipo, unsigned char permisos)
{
    // Leemos el superbloque
    struct superbloque SB;
    if (bread(0, &SB) == ERROR)
    {
        perror("Error");
        return ERROR;
    }
    if (SB.cantInodosLibres > 0)
    {
        // Nos guardamos la posición del inodo que reservamos (tendrá su utilidad más adelante)
        int posInodoReservado = SB.posPrimerInodoLibre;

        // Leemos el primer inodo libre
        struct inodo *reservado = malloc(sizeof(struct inodo));
        if (leer_inodo(posInodoReservado, reservado) == ERROR)
        {
            perror("ERROR: ");
        }
        // Modificamos los datos del inodo
        reservado->tipo = tipo;
        reservado->permisos = permisos;
        reservado->nlinks = 1;
        reservado->tamEnBytesLog = 0;
        reservado->atime = time(NULL);
        reservado->ctime = time(NULL);
        reservado->mtime = time(NULL);
        reservado->numBloquesOcupados = 0;

        // Actualizamos el SB
        SB.posPrimerInodoLibre = reservado->punterosDirectos[0]; // Actualizamos para que el SB ahora apunte al NUEVO primer nodo libre
        SB.cantInodosLibres--;                                   // Puesto que reservamos uno

        // *****Lo escribiremos en el disco un poco más adelante *****

        // Ponemos a 0 todos los punteros
        for (int i = 0; i < 12; i++)
        {
            reservado->punterosDirectos[i] = 0;
        }
        for (int i = 0; i < 3; i++)
        {
            reservado->punterosIndirectos[i] = 0;
        }
        // Escribimos el inodo en el AI
        if (escribir_inodo(posInodoReservado, reservado) == ERROR)
        {
            perror("Error: ");
            return ERROR;
        }

        // Lo escribimos en el disco
        if (bwrite(0, &SB) == ERROR)
        {
            perror("Error: ");
            return ERROR;
        }
        // Retornamos la posición del inodo que acabamos de reservar
        return posInodoReservado;
    }

    // CASO EN EL QUE NO QUEDAN INODOS LIBRES
    else
    {
        printf("No quedan inodos libres\n");
        return ERROR;
    }
}

// NIVEL 4
int obtener_nRangoBL(struct inodo *inodo, unsigned int nblogico, unsigned int *ptr)
{
    if (nblogico < DIRECTOS)
    {
        *ptr = inodo->punterosDirectos[nblogico];
        return 0;
    }
    if (nblogico < INDIRECTOS0)
    {
        *ptr = inodo->punterosIndirectos[0];
        return 1;
    }
    if (nblogico < INDIRECTOS1)
    {
        *ptr = inodo->punterosIndirectos[1];
        return 2;
    }
    if (nblogico < INDIRECTOS2)
    {
        *ptr = inodo->punterosIndirectos[2];
        return 3;
    }
    *ptr = 0;
    perror("Error:");
    return ERROR;
}

int obtener_indice(unsigned int nblogico, int nivel_punteros)
{
    if (nblogico < DIRECTOS)
    {
        return nblogico;
    }
    if (nblogico < INDIRECTOS0)
    {
        return nblogico - DIRECTOS;
    }
    if (nblogico < INDIRECTOS1)
    {
        if (nivel_punteros == 2)
        {
            return (nblogico - INDIRECTOS0) / NPUNTEROS;
        }
        if (nivel_punteros == 1)
        {
            return (nblogico - INDIRECTOS0) % NPUNTEROS;
        }
    }
    if (nblogico < INDIRECTOS2)
    {
        if (nivel_punteros == 3)
        {
            return (nblogico - INDIRECTOS1) / (NPUNTEROS * NPUNTEROS);
        }
        if (nivel_punteros == 2)
        {
            return ((nblogico - INDIRECTOS1) % (NPUNTEROS * NPUNTEROS) / NPUNTEROS);
        }
        if (nivel_punteros == 1)
        {
            return ((nblogico - INDIRECTOS1) % (NPUNTEROS * NPUNTEROS) % NPUNTEROS);
        }
    }
    return ERROR;
}


int traducir_bloque_inodo(unsigned int ninodo, unsigned int nblogico, unsigned char reservar) {
    struct inodo inodo;

    unsigned int ptr,ptr_ant;
    int salvar_inodo,nRangoBL,nivel_punteros,indice;

    unsigned int buffer[NPUNTEROS];

    ptr = ptr_ant = salvar_inodo = 0;

    leer_inodo(ninodo, &inodo);

    nRangoBL = obtener_nRangoBL(&inodo, nblogico, &ptr);// 0:D, 1:I0, 2:I1, 3:I2
    nivel_punteros = nRangoBL;// el nivel_punteros +alto es el que cuelga del inodo

    while (nivel_punteros > 0) {// iterar para cada nivel de punteros indirectos
        if (ptr == 0) { // no cuelgan bloques de punteros
            if (reservar == 0) { // bloque inexistente -> no imprimir nada por pantalla!!!
                return ERROR;
            } else { // reservar bloques de punteros y crear enlaces desde el  inodo hasta el bloque de dato
                salvar_inodo = 1;
                ptr = reservar_bloque();// de punteros
                inodo.numBloquesOcupados++;
                inodo.ctime = time(NULL);// fecha actual

                if (nivel_punteros == nRangoBL) {// el bloque cuelga directamente del inodo
                    inodo.punterosIndirectos[nRangoBL - 1] = ptr;// (imprimirlo para test)
#if DEBUGN4
                    printf("[traducir_bloque_inodo() → inodo.punterosIndirectos[%d]: %d (reservado BF %u para punteros_nivel%u)]\n", nRangoBL - 1, ptr, ptr, nivel_punteros);
#endif                   
                } else {// el bloque cuelga de otro bloque de punteros
                    buffer[indice] = ptr;// salvamos en el dispositivo el buffer de punteros modificado
#if DEBUGN4
                    printf("[traducir_bloque_inodo() → punteros_nivel%d[%d] = %d (reservado BF %u para punteros_nivel%u)]\n", nivel_punteros + 1, indice, ptr, ptr, nivel_punteros);
#endif                    
                    bwrite(ptr_ant, buffer);
                }
                memset(buffer, 0, BLOCKSIZE);
            }
        } else {
            bread(ptr, buffer);// leemos del dispositivo el bloque de punteros ya existente
        }

        indice = obtener_indice(nblogico, nivel_punteros);
        ptr_ant = ptr;         // guardamos el puntero actual
        ptr = buffer[indice]; // y lo desplazamos al siguiente nivel
        nivel_punteros--;
    }
    // al salir de este bucle ya estamos al nivel de datos

    if (ptr == 0) {// no existe bloque de datos
        if (reservar == 0) {// error lectura ∄ bloque
            return ERROR;
        } else {
            salvar_inodo = 1;
            ptr = reservar_bloque(); // de datos
            inodo.numBloquesOcupados++;
            inodo.ctime = time(NULL);

            if (nRangoBL == 0) {
                inodo.punterosDirectos[nblogico] = ptr; // (imprimirlo para test)
#if DEBUGN4
                printf("[traducir_bloque_inodo() → inodo.punterosDirectos[%d] =  %d (reservado BF %u para BL %u)]\n", nblogico, ptr, ptr, nblogico);
#endif
            } else {
                buffer[indice] = ptr; // asignamos la dirección del bloque de datos (imprimirlo para test)
#if DEBUGN4
                printf("[traducir_bloque_inodo() → punteros_nivel%d[%d] = %d (reservado BF %u para BL %u)]\n", nivel_punteros + 1, indice, ptr, ptr, nblogico);
#endif
                bwrite(ptr_ant, buffer);

            }
            // escribimos en el dispositivo el inodo actualizado
            escribir_inodo(ninodo, &inodo);
#if DEBUGN4
            printf("[traducir_bloque_inodo() → punteros_nivel1[1] = %d (reservado BF %u para BL %u)]\n", ptr, ptr, nblogico);
#endif
        }
        // printf("nblogico= %d, ptr= %d\n", nblogico, ptr);
        return ptr;
    }


    if (salvar_inodo == 1) {
        escribir_inodo(ninodo, &inodo);// sólo si lo hemos actualizado
    }
    // nº de bloque físico correspondiente al bloque de datos lógico, nblogico
    return ptr;
}
// Nivel 6

int liberar_inodo(unsigned int ninodo)
{
    struct inodo inodo;
    struct superbloque SB;
    // Leemos el inodo
    if (leer_inodo(ninodo, &inodo) == ERROR)
    {
        perror("ERROR: ");
        return ERROR;
    }
    // Liberamos sus bloques
    int bliberados = liberar_bloques_inodos(0, &inodo);
    // Control de errores
    if (bliberados == ERROR)
    {
        perror("ERROR: ");
        return ERROR;
    }
    // Actualizamos inodo.numBloquesOcupados
    inodo.numBloquesOcupados = inodo.numBloquesOcupados - bliberados;

    if (inodo.numBloquesOcupados == 0)
    {
        // Lo marcamos como vacío y libre
        inodo.tamEnBytesLog = 0;
        inodo.tipo = 'l';

        if (bread(0, &SB) == ERROR)
        {
            perror("ERROR: ");
            return ERROR;
        }
        // Ponemos el inodo que hemos liberado en la lista de libres
        inodo.punterosDirectos[0] = SB.posPrimerInodoLibre;
        SB.posPrimerInodoLibre = ninodo;
        SB.cantInodosLibres++;

        // Lo escribimos en el AI
        if (escribir_inodo(ninodo, &inodo) == ERROR)
        {
            perror("ERROR: ");
            return ERROR;
        }
        // Escribims el SB actualizado
        if (bwrite(0, &SB) == ERROR)
        {
            perror("ERROR: ");
            return ERROR;
        }
        return ninodo;
    }
    else
    {
        printf("[liberar_inodo()-> ERROR! inodo.numBloquesOcupados != 0. inodo.numBloquesOcupados = %u]", inodo.numBloquesOcupados);
        return ERROR;
    }
}

int liberar_bloques_inodos(unsigned int primerBL, struct inodo *inodo){
    unsigned int nivel_punteros, nblog, ultimoBL,indice, ptr;
    unsigned char bufAux_punteros[BLOCKSIZE];
    unsigned int bloques_punteros[3][NPUNTEROS];
    int indices_primerBL[3];// indices del primerBL para cuando se llama desde mi_truncar_f()  
    int liberados = 0;    
    int contador_breads = 0;// para comprobar optimización eficiencia
    int contador_bwrites = 0;// para comprobar optimización eficiencia
    int nRangoBL;
    int ptr_nivel[3];// punteros a los bloques

    if ((inodo->tamEnBytesLog) == 0) {// si el inodo está vacío
        return liberados;
    }

    if (inodo->tamEnBytesLog % BLOCKSIZE == 0) {// calculamos la posición del último BL
        ultimoBL = ((inodo->tamEnBytesLog) / BLOCKSIZE) - 1;
    } else {
        ultimoBL = (inodo->tamEnBytesLog) / BLOCKSIZE;
    }

#if DEBUGN6
    fprintf(stderr, "[liberar_bloques_inodo()→ primer BL: %d, último BL: %d]\n", primerBL, ultimoBL);
#endif

    memset(bufAux_punteros, 0, BLOCKSIZE);
    //Recorremos los bloques del inodo.
    for (nblog = primerBL; nblog <= ultimoBL; nblog++){
        nRangoBL = obtener_nRangoBL(inodo, nblog, &ptr);
        if (nRangoBL < 0){
            fprintf(stderr, "ERROR al obtener el rango del BL\n");
            return ERROR;
        }
        nivel_punteros = nRangoBL; 
        while (ptr > 0 && nivel_punteros > 0) { 
            //Obtenemos el indice.
            indice = obtener_indice(nblog, nivel_punteros);
            if ((indice == 0) || (nblog == primerBL)){
                if (bread(ptr, bloques_punteros[nivel_punteros - 1]) == ERROR){
                    fprintf(stderr, "Error: leer el dispositivo no cargado previamente\n");
                    return ERROR;
                }
                //Como hemos hecho una lectura incrementamos los breads.
                contador_breads++;
            }

            ptr_nivel[nivel_punteros - 1] = ptr;
            indices_primerBL[nivel_punteros - 1] = indice;
            ptr = bloques_punteros[nivel_punteros - 1][indice];
            nivel_punteros--;
        }

        //Si existen bloques entramos.
        if (ptr > 0){
            liberar_bloque(ptr);
            liberados++;
#if DEBUGN6
            fprintf(stderr,"[liberar_bloques_inodo()-> liberado BF %d de datos par a BL %d]\n", ptr, nblog);
#endif
            //nRangoBL es un puntero directo.
            if (nRangoBL == 0){
                inodo->punterosDirectos[nblog] = 0;
            } else {
                nivel_punteros = 1;
                while (nivel_punteros <= nRangoBL){

                    indice = indices_primerBL[nivel_punteros - 1];
                    bloques_punteros[nivel_punteros - 1][indice] = 0;
                    ptr = ptr_nivel[nivel_punteros - 1];

                    if (memcmp(bloques_punteros[nivel_punteros - 1], bufAux_punteros, BLOCKSIZE) == 0){
                        //Debemos liberar el bloque de punteros.
                        liberar_bloque(ptr);
                        //Como lo hemos liberado actualizamos el liberados.
                        liberados++;
#if DEBUGN6
                        fprintf(stderr,"[liberar_bloques_inodo()→ liberado BF %i de punteros_nivel%i correspondiente al BL: %i]\n", ptr, nivel_punteros, nblog);
#endif
                        if (nivel_punteros == nRangoBL){
                            inodo->punterosIndirectos[nRangoBL - 1] = 0;
                        }
                        nivel_punteros++;
                    } else { //escribimos en el dispositivo el bloque de punteros modificado
                        if (bwrite(ptr, bloques_punteros[nivel_punteros - 1]) == ERROR){
                            fprintf(stderr, "Error de escritura\n");
                            return ERROR;
                        }
                        //Como hemos escrito lo actualizamos.
                        contador_bwrites++;
                        //Salimos ya que no es necesario liberar nada mas.
                        nivel_punteros = nRangoBL + 1;
                    }
                }
            }
        }
    }
#if DEBUGN6
    fprintf(stderr,"[liberar_bloques_inodo()-> total bloques liberados: %d,total breads: %d, total bwrites: %d]\n", liberados,contador_breads,contador_bwrites);
#endif
    return liberados;
}