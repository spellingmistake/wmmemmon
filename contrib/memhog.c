/* $Id: memhog.c,v 1.1.1.1 2002/10/14 12:23:26 sch Exp $ */

/*
 * memhog.c -- increasinly allocates memory
 */
#include <stdlib.h>
#include <stdio.h>

#define BLOCKSIZE (16*1024*1024) /* 16 megabytes */

int main(int argc, char *argv[]){
  unsigned long  sz = 0;
  char          *m  = (void*)NULL;
  for ( ; ; ) {
    m = (char *)realloc(m, (++sz)*BLOCKSIZE);
    memset(&m[((sz-1)*BLOCKSIZE)], 0xff, BLOCKSIZE);
    printf("%lu total megabytes allocated.\n", (sz*BLOCKSIZE/(1024*1024)));
    sleep(1);
  };
  return 0;
}
