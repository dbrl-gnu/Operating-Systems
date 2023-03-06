// Autors: Marc Melià Flexas, Pau Rosado Muñoz, Xavier Vives Marcus
#include <sys/wait.h>
#include <signal.h>
#include "directorios.h"

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_BLUE "\x1b[34m"

#define NUMPROCESOS 100
#define NUMESCRITURAS 50
#define REGMAX  500000
struct REGISTRO{

    time_t fecha;
    pid_t pid;
    int nEscritura;
    int nRegistro;
};

void reaper();