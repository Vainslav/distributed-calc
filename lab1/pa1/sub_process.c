#include "sub_process.h"
#include "ipc.h"

typedef struct ProcessCommunication
{
    int reads[],
    int writes[]
} ProcessCommunication;


void sub_process_routine(int id, int reads[], int writes[], int size) {
    
}