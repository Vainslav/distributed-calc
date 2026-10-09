#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pa1.h"

int main(int argc, char *argv[])
{
   int n = 1;
   if (argc == 3 && strcmp(argv[1], "-P") == 0)
   {
      n = (int) strtol(argv[2], NULL, 10);
   } else {
      return -1;
   }

   int reads[n][n];
   int writes[n][n];
   
   return 0;
}
