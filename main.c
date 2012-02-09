/* main.c
 * Jen Hanni
 * 
 * PDP-8 ISA-level simulator
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

#define PAGES 32
#define WORDS 128

// I am a teapot, also a 2-dimensional simulation of PDP-8 memory
// 32 indices, 128 bytes

int memory[PAGES][WORDS];

FILE *ifp,*ofp;

int main()
{
  memset(memory, 0, sizeof(int));

  // open the tracefile, make it available to 'r' read
  // open the output file to make it available to append each iteration's result
  ifp = fopen("test.mem", "r");
  ofp = fopen("tracefile.din", "a");

  // set it up to read line by line, set n and addr accordingly
  char input[5];
  while (fscanf(ifp, "%s\n", &input) != EOF)
  {
    if (input[0] == '@')
       printf("%c\n",input[0]);
    else 
       printf("nope\n");
  }
  return 0; 
}
