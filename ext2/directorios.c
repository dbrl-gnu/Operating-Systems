// Autors: Marc Melià Flexas, Pau Rosado Muñoz, Xavier Vives Marcus
#include "directorios.h"
#include "debugging.h"
#include <string.h>

struct UltimaEntrada UltimaEntrada[2];

int extraer_camino(const char *camino, char *inicial, char *final, char *tipo)
{

    // Nos aseguramos de que empieze correctamente
    if (camino[0] != '/' || camino == NULL)
    {
        return ERROR;
    }
    // Caso en el que empieza correctamente
    else
    {
        // Copiamos el contenido del camino en un string auxiliar para poder modificarlo
        char straux[strlen(camino)];
        strcpy(straux, camino);

        // Dividimos el string auxiliar en dos tókens divididos por el carácter "/".
        // La primera parte irá al puntero inicial y straux contendrá el resto del string
        strcpy(inicial, strtok(straux, "/"));

        // Comprobamos si se trata de un fichero o de un directorio
        // Caso directorio
        if (camino[strlen(inicial) + 1] == '/')
        {
            // Ponemos el valor correspondiente a tipo
            *tipo = 'd';
            // Copiamos el resto de la ruta en el puntero final
            strcpy(final, camino + strlen(inicial) + 1);
        }
        // Caso fichero
        else
        {
            // Ponemos el valor correspondiente a tipo
            *tipo = 'f';
            // Ponemos "" en final pues no le podemos asignar valor NULL pues luego quedaría apuntando a NULL
            strcpy(final, "");
            // Si es un fichero lo ponemos sin el "/" inicial
            strcpy(inicial, camino + 1);
        }
    }
    return 0;
}

int buscar_entrada(const char *camino_parcial, unsigned int *p_inodo_dir, unsigned int *p_inodo, unsigned int *p_entrada,
                   char reservar, unsigned char permisos)
{
    struct superbloque SB;
    struct entrada entrada;
    struct inodo inodo_dir;
    char inicial[sizeof(entrada.nombre)];
    char final[strlen(camino_parcial)];
    char tipo;
    int cant_entradas_inodo;
    int num_entrada_inodo;
    int reservado;

    // Leemos superbloque
    if (bread(0, &SB) == ERROR)
    {
        perror("ERROR");
        return ERROR;
    }

    if (strcmp(camino_parcial, "/") == 0)
    {
        *p_inodo = SB.posInodoRaiz;
        *p_entrada = 0;
        return 0;
    }

    if (extraer_camino(camino_parcial, inicial, final, &tipo) == ERROR)
    {
        return ERROR_CAMINO_INCORRECTO;
    }
#if DEBUGN7
    fprintf(stdout, "[buscar_entrada()-> inicial:%s final:%s,reservar: %i]\n", inicial, final, reservar);
#endif
    // buscamos la entrada cuyo nombre se encuentra en inicial
    if (leer_inodo(*p_inodo_dir, &inodo_dir) == ERROR)
    {
        perror("ERROR: ");
        return ERROR;
    }
    // Comprobamos si tiene permisos de lectura
    if ((inodo_dir.permisos & 4) != 4)
    {
        fprintf(stderr, "[buscar_entrada()→ El inodo %d no tiene permisos de lectura\n", *p_inodo_dir);
        return ERROR_PERMISO_LECTURA;
    }
    memset(&entrada, 0, sizeof(entrada));
    // Calculamos cuantas entradas tiene el inodo_dir
    cant_entradas_inodo = inodo_dir.tamEnBytesLog / sizeof(entrada);
    // Num de entrada inicial
    num_entrada_inodo = 0;
    // Variable que usaremos para iterar
    int offset = 0;

    if (cant_entradas_inodo > 0)
    {
        if (mi_read_f(*p_inodo_dir, &entrada, offset, sizeof(entrada)) == ERROR)
        {
            perror("ERROR: ");
            return ERROR;
        }
        while ((num_entrada_inodo < cant_entradas_inodo) && (strcmp(inicial, entrada.nombre) != 0))
        {
            num_entrada_inodo++;
            offset += sizeof(entrada);
            memset(&entrada, 0, sizeof(entrada));
            if (mi_read_f(*p_inodo_dir, &entrada, offset, sizeof(entrada)) == ERROR)
            {
                perror("ERROR: ");
                return ERROR;
            }
        }
    }
    if (strcmp(inicial, entrada.nombre) != 0)
    {
        switch (reservar)
        {
        case 0:
            return ERROR_NO_EXISTE_ENTRADA_CONSULTA;
            break;

        case 1:
            if (inodo_dir.tipo == 'f')
            {
                return ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_UN_FICHERO;
            }
            if ((inodo_dir.permisos & 2) != 2)
            {
                return ERROR_PERMISO_ESCRITURA;
            }
            else
            {
                strcpy(entrada.nombre, inicial);
                if (tipo == 'd')
                {
                    if (strcmp(final, "/") == 0)
                    {
                        reservado = reservar_inodo('d', permisos);
                        if (reservado == ERROR)
                        {
                            perror("ERROR: ");
                            return ERROR;
                        }
#if DEBUGN7
                        fprintf(stdout, "[buscar_entrada()-> reservado inodo %i tipo %c con permisos %i para %s]\n", reservado, tipo, permisos, inicial);
#endif
                        entrada.ninodo = reservado;
                    }
                    else
                    {
                        return ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO;
                    }
                }
                else
                {
                    reservado = reservar_inodo('f', permisos);
                    if (reservado == ERROR)
                    {
                        perror("ERROR: ");
                        return ERROR;
                    }
#if DEBUGN7
                    fprintf(stdout, "[buscar_entrada()-> reservado inodo %i tipo %c con permisos %i para %s]\n", reservado, tipo, permisos, inicial);
#endif
                    entrada.ninodo = reservado;
                }
                if (mi_write_f(*p_inodo_dir, &entrada, inodo_dir.tamEnBytesLog, sizeof(entrada)) == ERROR)
                {
                    if (entrada.ninodo != -1)
                    {
                        if (liberar_inodo(entrada.ninodo) == ERROR)
                        {
                            perror("ERROR: ");
                            return ERROR;
                        }
                    }
                    return EXIT_FAILURE;
                }
#if DEBUGN7
                fprintf(stdout, "[buscar_entrada()-> creada entrada %s, %i]\n", entrada.nombre, entrada.ninodo);
#endif
            }
            // break;
        }
    }

    if ((strcmp(final, "/") == 0) || (strcmp(final, "") == 0))
    {
        if ((num_entrada_inodo < cant_entradas_inodo) && (reservar == 1))
        {
            return ERROR_ENTRADA_YA_EXISTENTE;
        }
        *p_inodo = entrada.ninodo;
        *p_entrada = num_entrada_inodo;
        return EXIT_SUCCESS;
    }
    else
    {
        *p_inodo_dir = entrada.ninodo;
        return (buscar_entrada(final, p_inodo_dir, p_inodo, p_entrada, reservar, permisos));
    }
    return EXIT_SUCCESS;
}

void mostrar_error_buscar_entrada(int error)
{
    // fprintf(stderr, "Error: %d\n", error);
    switch (error)
    {
    case -1:
        fprintf(stderr, "Error: Camino incorrecto.\n");
        break;
    case -2:
        fprintf(stderr, "Error: Permiso denegado de lectura.\n");
        break;
    case -3:
        fprintf(stderr, "Errorsito: No existe el archivo o el directorio.\n");
        break;
    case -4:
        fprintf(stderr, "Error: No existe algún directorio intermedio.\n");
        break;
    case -5:
        fprintf(stderr, "Error: Permiso denegado de escritura.\n");
        break;
    case -6:
        fprintf(stderr, "Error: El archivo ya existe.\n");
        break;
    case -7:
        fprintf(stderr, "Error: No es un directorio.\n");
        break;
    }
}

// Nivel 8

//  Crea un fichero/directorio y su entrada de directorio.
int mi_creat(const char *camino, unsigned char permisos)
{

    mi_waitSem();
    int error;
    unsigned int p_inodo_dir, p_inodo, p_entrada;
    p_inodo_dir = p_inodo = p_entrada = 0;

    error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 1, permisos);

    if (error < 0)
    {
        mostrar_error_buscar_entrada(error);
        mi_signalSem();
        return ERROR;
    }
    mi_signalSem();
    return 0;
}

int mi_dir(const char *camino, char *buffer, char tipo)
{

    // Variables necesarias para buscar entrada
    unsigned int p_inodo_dir, p_inodo, p_entrada;
    p_inodo_dir = p_inodo = p_entrada = 0;

    int error;
    struct inodo inodo;

    error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);

    if (error < 0)
    {
        mostrar_error_buscar_entrada(error);
        return ERROR;
    }

    if (leer_inodo(p_inodo, &inodo) == -1)
    {
        // Error lectura del inodo
        return ERROR;
    }
    // Comprobamos permisos de lectura
    if ((inodo.permisos & 4) != 4)
    {
        // Error directorio no tiene permisos de lectura.
        return -1;
    }
    if (inodo.tipo != tipo)
    {
        // Error: la sintaxis no concuerda con el tipo”
        return -1;
    }

    struct entrada entrada;
    struct tm *tm;
    char aux[30];
    

    // Número de entradas
    unsigned int num_entradas;

    // Si es un directorio tenemos que poner el contenido de sus entradas en el buffer
    if (inodo.tipo == 'd')
    {

        // Calculamos el número de entradas del directorio
        num_entradas = inodo.tamEnBytesLog / sizeof(entrada);
        sprintf(aux, "Total: %i\n", num_entradas);
        if (num_entradas > 0)
        {

            strcat(buffer, aux);
            strcat(buffer, "Tipo \tModo \tmTime \t\t\tTamaño \tNombre\n----------------------------------------------------------------\n");
        }

        // Para cada entrada
        for (int i = 0; i < num_entradas; i++)
        {
            memset(&entrada, 0, sizeof(entrada));
            // Obtenemos la entrada i
            if (mi_read_f(p_inodo, &entrada, i * sizeof(entrada), sizeof(entrada)) < 0)
            {

                return ERROR;
            }
            // Leemos el inodo de la entrada
            if (leer_inodo(entrada.ninodo, &inodo) < 0)
            {

                return ERROR;
            }

            sprintf(aux, "%c\t", inodo.tipo);

            strcat(buffer, aux);
            if (inodo.permisos & 4)
                strcat(buffer, "r");
            else
                strcat(buffer, "-");
            if (inodo.permisos & 2)
                strcat(buffer, "w");
            else
                strcat(buffer, "-");
            if (inodo.permisos & 1)
                strcat(buffer, "x");
            else
                strcat(buffer, "-");

            strcat(buffer, "\t");

            tm = localtime(&inodo.mtime);
            sprintf(aux, "%d-%02d-%02d %02d:%02d:%02d\t", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
            strcat(buffer, aux);

            if (inodo.tipo == 'd')
            {

                sprintf(aux, "%d\t" AZUL_T, inodo.tamEnBytesLog);
            }
            else
            {
                sprintf(aux, "%d\t" VERDE_T, inodo.tamEnBytesLog);
            }

            strcat(buffer, aux);

            strcat(buffer, entrada.nombre);
            strcat(buffer, RESET_COLOR "\n");
        }

        return num_entradas;
    }
    else // si es un fichero tenemos que poner su información en el buffer
    {
        strcat(buffer, "Tipo \tModo \tmTime \t\t\tTamaño \tNombre\n----------------------------------------------------------------\n");
        int offset = p_entrada * sizeof(entrada);

        memset(&entrada, 0, sizeof(entrada));
        // Obtenemos la entrada del indodo
        if (mi_read_f(p_inodo_dir, &entrada, offset, sizeof(entrada)) < 0)
        {

            return ERROR;
        }

        sprintf(aux, "%c\t", inodo.tipo);
        strcat(buffer, aux);
        if (inodo.permisos & 4)
            strcat(buffer, "r");
        else
            strcat(buffer, "-");
        if (inodo.permisos & 2)
            strcat(buffer, "w");
        else
            strcat(buffer, "-");
        if (inodo.permisos & 1)
            strcat(buffer, "x");
        else
            strcat(buffer, "-");

        strcat(buffer, "\t");

        tm = localtime(&inodo.mtime);
        sprintf(aux, "%d-%02d-%02d %02d:%02d:%02d\t", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
        strcat(buffer, aux);
        sprintf(aux, "%d\t", inodo.tamEnBytesLog);
        strcat(buffer, aux);

        strcat(buffer, entrada.nombre);
        strcat(buffer, "\n");

        return 0;
    }
}

int mi_chmod(const char *camino, unsigned char permisos)
{
    unsigned int p_inodo_dir, p_inodo, p_entrada;
    p_inodo_dir = p_inodo = p_entrada = 0;
    int error;

    error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, permisos);

    if (error < 0)
    {
        mostrar_error_buscar_entrada(error);
        return ERROR;
    }

    if (mi_chmod_f(p_inodo, permisos) < 0)
    {

        printf("Error:  mi_chmod: error al cambiar los permisos");
        return ERROR;
    }

    return 0;
}

int mi_stat(const char *camino, struct STAT *p_stat)
{

    unsigned int p_inodo_dir, p_inodo, p_entrada;
    p_inodo_dir = 0;
    int error;

    // Obtenemos en número de inodo
    if ((error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0)) < 0)
    {
        // En cas d'error avisam al usuari.
        mostrar_error_buscar_entrada(error);
        return ERROR;
    }

    if (mi_stat_f(p_inodo, p_stat) < 0)
    {

        printf("Error:  mi_stat_f: error al obtener los datos del inodo");
        return ERROR;
    }

    // Definimos el formato para imprimir el tiempo
    struct tm *tm;
    char atime[80];
    char mtime[80];
    char ctime[80];
    tm = localtime(&p_stat->atime);
    strftime(atime, sizeof(atime), "%a %Y-%m-%d %H:%M:%S", tm);
    tm = localtime(&p_stat->mtime);
    strftime(mtime, sizeof(mtime), "%a %Y-%m-%d %H:%M:%S", tm);
    tm = localtime(&p_stat->ctime);
    strftime(ctime, sizeof(ctime), "%a %Y-%m-%d %H:%M:%S", tm);

    // Imprimimos las variables necesarias.
    printf("Nº de inodo: %d \n", p_inodo);
    printf("Tipo: %c\n", p_stat->tipo);
    printf("Permisos: %i\n", p_stat->permisos);
    printf("atime: %s\n", atime);
    printf("ctime: %s\n", ctime);
    printf("mtime: %s\n", mtime);
    printf("nlinks: %d\n", p_stat->nlinks);
    printf("tamEnBytesLog: %d\n", p_stat->tamEnBytesLog);
    printf("numBloquesOcupados: %d\n\n", p_stat->numBloquesOcupados);

    return 0;
}

// Nivell 9

int mi_write(const char *camino, const void *buf, unsigned int offset, unsigned int nbytes)
{
    int nbytesEscritos = 0;
    unsigned int p_inodo;

    // Comprobamos si coincide con la ultima entrada
    if (strcmp(UltimaEntrada[0].camino, camino) == 0)
    {
        p_inodo = UltimaEntrada[0].p_inodo;
#if DEBUGN9
        printf("[mi_write()--> Utilizamos la caché de escritura en vez de llamar a buscar_entrada()]\n");
#endif
    }
    // Caso en el que no coincide con la ultima entrada y tendremos que llamar a buscar_entrada()
    else
    {
        unsigned int p_inodo_dir = 0;
        unsigned int p_entrada = 0;
        int error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);

        // Caso en el que ha ocurrido algun error
        if (error < 0)
        {
            mostrar_error_buscar_entrada(error);
            // return ERROR ????
            return -1;
        }
#if DEBUGN9
        printf("[mi_write() --> Actualizamos la caché de escritura]\n");
#endif
        strcpy(UltimaEntrada[0].camino, camino);
        UltimaEntrada[0].p_inodo = p_inodo;
    }

    nbytesEscritos = mi_write_f(p_inodo, buf, offset, nbytes);

    return nbytesEscritos;
}

int mi_read(const char *camino, void *buf, unsigned int offset, unsigned int nbytes)
{
    int nBytesLeidos = 0;
    unsigned int p_inodo;

    // Comprobamos si coincide con la ultima entrada
    if (strcmp(UltimaEntrada[1].camino, camino) == 0)
    {
        p_inodo = UltimaEntrada[1].p_inodo;
#if DEBUGN9
        printf("[mi_read()--> Utilizamos la caché de escritura en vez de llamar a buscar_entrada()]\n");
#endif
    } // Caso en el que no coincide con la ultima entrada y tendremos que llamar a buscar_entrada()
    else
    {
        unsigned int p_inodo_dir = 0;
        unsigned int p_entrada = 0;
        int error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);

        // Caso en el que ha ocurrido algun error
        if (error < 0)
        {
            mostrar_error_buscar_entrada(error);
            // return ERROR ????
            return -1;
        }
#if DEBUGN9
        printf("[mi_read() --> Actualizamos la caché de escritura]\n");
#endif

        // Actualizamos la última entrada
        strcpy(UltimaEntrada[1].camino, camino);
        UltimaEntrada[1].p_inodo = p_inodo;
    }

    nBytesLeidos = mi_read_f(p_inodo, buf, offset, nbytes);
    // fwrite(buf, sizeof(char), nBytesLeidos, stdout);
    return nBytesLeidos;
}
// Nivell 10

int mi_link(const char *camino1, const char *camino2)
{
    mi_waitSem();
    struct inodo inodo1;
    unsigned int p_inodo1 = 0;
    unsigned int p_inodo_dir1 = 0;
    unsigned int p_entrada1 = 0;
    unsigned int p_inodo2 = 0;
    unsigned int p_inodo_dir2 = 0;
    unsigned int p_entrada2 = 0;
    int error;
    if ((error = buscar_entrada(camino1, &p_inodo_dir1, &p_inodo1, &p_entrada1, 0, 0)) < 0)
    {
        mostrar_error_buscar_entrada(error);
        mi_signalSem();
        return ERROR;
    }
    // Leemos inodo
    if(leer_inodo(p_inodo1, &inodo1)<0){
        mi_signalSem();
        return ERROR;

    }
    // Comprobamos permisos del inodo
    if ((inodo1.permisos & 4) != 4)
    {
        printf("[mi_link() -> Error de permisos]");
        mi_signalSem();
        return ERROR;
    }

    // Comprobamos que camino 2 no exista
    if ((error = buscar_entrada(camino2, &p_inodo_dir2, &p_inodo2, &p_entrada2, 1, 6)) < 0)
    {
        mostrar_error_buscar_entrada(error);
        mi_signalSem();
        return ERROR;
    }
    // Leemos la entrada creada en camino2
    struct entrada entrada;
    int tamentrada = sizeof(entrada);
    memset(&entrada, 0, tamentrada);
    if (mi_read_f(p_inodo_dir2, &entrada, (p_entrada2 * tamentrada), tamentrada)<0){//pau
        mi_signalSem();
        return ERROR;
    }

    // Escribimos la entrada modificada
    entrada.ninodo = p_inodo1;
    if(mi_write_f(p_inodo_dir2, &entrada, (p_entrada2 * tamentrada), tamentrada)<0){
        mi_signalSem();
        return ERROR;
    }

    // Liberamos inodo que se ha reservado asociado a la entrada (inodo2)
    if(liberar_inodo(p_inodo2)<0){
        mi_signalSem();
        return ERROR;
    }

    // Actualizamos datos del inodo
    inodo1.ctime = time(NULL);
    inodo1.nlinks++;
    // Aztualizamos los cambios en el dispositivo
    if(escribir_inodo(p_inodo1, &inodo1)<0){
        mi_signalSem();
        return ERROR;
    }
    mi_signalSem();
    return 0;
}

int mi_unlink(const char *camino)
{
    mi_waitSem();
    // Variables para buscar entrada
    unsigned int p_inodo_dir, p_inodo, p_entrada;
    p_inodo_dir = p_inodo = p_entrada = 0;
    int error;

    // Variables para leer inodos
    struct inodo inodo;
    struct inodo inodo_dir;

    // Variabels para leer la entrada
    struct entrada entrada;

    // Comprobamos que existe la entrada
    error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);//ultimo 0 por 4

    if (error < 0)
    {
        mostrar_error_buscar_entrada(error);
        mi_signalSem();
        return ERROR;
    }

    // Obtenemos la información del inodo del camino
    if (leer_inodo(p_inodo, &inodo) == -1)
    {
        // Error lectura del inodo
        mi_signalSem();
        return ERROR;
    }

    // Si se trata de un directorio y no está vacío entonces no se puede borrar
    if (inodo.tipo == 'd' && inodo.tamEnBytesLog > 0)
    {

        printf("El directorio no está vacío, no se puede borrar \n");
        mi_signalSem();
        return -1;
    }

    //  Leemos el inodo asociado al directorio que contiene la entrada que queremos eliminar
    if (leer_inodo(p_inodo_dir, &inodo_dir) == -1)
    {
        mi_signalSem();
        // Error lectura del inodo
        return ERROR;
    }

    // Obtenemos el número de entradas de directorio
    int nEntradas = inodo_dir.tamEnBytesLog / sizeof(entrada);

    // Si no es la última entrada
    if (p_entrada != (nEntradas - 1))
    {
        memset(&entrada, 0, sizeof(entrada));
        //  Leemos la última entrada
        if (mi_read_f(p_inodo_dir, &entrada, (nEntradas - 1) * sizeof(entrada), sizeof(entrada)) < 0)
        {
            mi_signalSem();
            return ERROR;
        }

        // La escribimos en la posición de la entrada que queremos eliminar
        if (mi_write_f(p_inodo_dir, &entrada, p_entrada * sizeof(entrada), sizeof(entrada)) < 0)
        {
            mi_signalSem();
            return ERROR;
        }
    }

    // Truncamos el inodo a su tamaño menos el tamaño de una entrada
    if (mi_truncar_f(p_inodo_dir, (inodo_dir.tamEnBytesLog - sizeof(entrada))) < 0)
    {
        mi_signalSem();
        return ERROR;
    }

    // Decrementamos el número de links del inodo
    inodo.nlinks--;

    // Si no quedan enlaces
    if (inodo.nlinks == 0)
    {

        // Liberaremos el inodo
        if (liberar_inodo(p_inodo) < 0)
        {
            mi_signalSem();
            return ERROR;
        }
    }
    else
    {

        // Escribimos el inodo
        inodo.ctime = time(NULL);
        if (escribir_inodo(p_inodo, &inodo) < 0)
        {
            mi_signalSem();
            return ERROR;
        }
    }
    mi_signalSem();
    return 0;
}