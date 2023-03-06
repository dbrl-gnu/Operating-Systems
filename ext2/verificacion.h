// Autors: Marc Melià Flexas, Pau Rosado Muñoz, Xavier Vives Marcus
#include "simulacion.h"

struct INFORMACION{
    int pid;
    unsigned int nEscrituras;
    struct REGISTRO PrimeraEscritura;
    struct REGISTRO UltimaEscritura;
    struct REGISTRO MenorPosicion;
    struct REGISTRO MayorPosicion;
};