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
#define BYTES 128

uint32_t countClock;	// total number of clock cycles

uint32_t countInstr;	// total number of instructions

uint32_t countAND;	// number of times AND was executed
uint32_t countTAD;	// number of times TAD was executed
uint32_t countISZ; 	// number of times ISZ was executed
uint32_t countDCA;	// number of times DCA was executed
uint32_t countJMS;	// number of times JMS was executed
uint32_t countJMP;	// number of times JMP was executed
uint32_t countIO;	// number of times IO was executed
uint32_t countMicro;	// number of times Micro was executed

typedef struct {	// holds address of next instruction
  uint32_t addr;
  int memPage;
  int memOffset;
} registerPC;

typedef struct {	// struct .linkbit = 0, .accbits = 0-11
  uint32_t addr;
  int linkbit;
  uint32_t accbits;
} registerAC;

typedef struct {	// memory instruction register
  int opcode;
  int indirectBit;
  int memPageBit;
  int offset;
} registerMRI;

void updatePC(uint32_t i);
int getOpCode(uint32_t instruction);
void handleMRI(uint32_t i);

// Initialize a 2-dimensional (32 pages x 128 bytes) simulation of PDP-8 memory

int memory[PAGES][BYTES];

registerPC regPC;
registerAC regAC;
registerMRI regMRI;

uint32_t regCPMA;	// holds address to be read/written from/to memory

uint32_t regMemBuff;	// holds value read from memory OR value to be written
uint32_t regSwitch;	// used to load memory from console

int signalRead;		// de-asserted 0, asserted 1
int signalWrite;	// de-asserted 0, asserted 1

uint32_t memPage;	// which of the 32 pages in memory
uint32_t memOffset;	// which 128 12-bit word on that page

FILE *ifp,*ofp;

int main()
{
  memset(memory, 0, sizeof(int));

  // open the tracefile, make it available to 'r' read
  // open the output file to make it available to append each iteration's result
  ifp = fopen("test.mem", "r");
  ofp = fopen("tracefile.din", "a");

  // set it up to read line by line, set n and addr accordingly
  uint32_t i;
  int carryout; // need to define/fix this
  char input[5];

  while (fscanf(ifp, "%s\n", &input) != EOF)
  {
    // treat intput line as an address
    if (input[0] == '@')
    {
      i = input[1]*256 + input[2]*16 + input[3];
      updatePC(i);
    }
    // treat input line as an instruction
    else 
    {
      i = input[0]*256 + input[1]*16 + input[2];
      // input line is not a memory reference instruction
      regMRI.opcode = i >> 9;
      if (regMRI.opcode == 6 || regMRI.opcode == 7)
      {
        printf("deal with opcode 6 or 7\n");
      }

      // input line is just wrong
      else if (regMRI.opcode > 7)
      {
        printf("opcode %d is greater than 7\n",regMRI.opcode);
      }

      // treat input line as a memory reference instruction
      else
      { 
        handleMRI(i);
        if (regMRI.memPageBit == 0 && regMRI.indirectBit == 1)
        {
          regCPMA = 0x0 + regMRI.offset;
        }
        else if (regMRI.memPageBit == 1 && regMRI.indirectBit == 1)
        {
          regCPMA = regPC.addr & regMRI.offset;
        }
        else if (regMRI.memPageBit == 1 && regMRI.indirectBit == 0)
        {
          regCPMA = memory[regPC.memPage][regMRI.offset];
        }
        else if (regMRI.memPageBit == 0 && regMRI.indirectBit == 0)
        {
          if (0x8 < regMRI.offset < 0xf)
          {
            regCPMA = memory[0][regMRI.offset]++;
            countClock = countClock + 2;
          }
          else
          {
            regCPMA = memory[0][regMRI.offset];
            countClock++;
          }
        }
        else if (regMRI.memPageBit > 1 || regMRI.indirectBit > 1)
        {
          printf("this is not a possible case.\n");
        }

        switch (regMRI.opcode)
        {
          case 0:
            countAND++;
            regAC.addr = regCPMA & regAC.addr;
          case 1:
            countTAD++;
            regAC.addr = regCPMA & regAC.addr;
            if (carryout && regAC.linkbit == 0)
               regAC.linkbit = 1;
            else if (carryout && regAC.linkbit == 1)
               regAC.linkbit = 0; 
          case 2:
            countISZ++;
            if (regCPMA == 0)
            {
               regPC.addr++;
               updatePC(regPC.addr);
            }
            else
               regCPMA++;
          case 3:
            countDCA++;
            regCPMA = regAC.addr;
            regAC.addr = 0;
            regAC.linkbit = 0;
          case 4:
            countJMS++;
            regCPMA = regPC.addr;
            regPC.addr = regCPMA++;
            updatePC(regPC.addr);
          case 5:
            countJMP++;
            regPC.addr = regCPMA;
            updatePC(regPC.addr);
        } // end switch 

      } // end MRI handling

    } // end instruction handling

  } // end while loop

  fclose(ifp);
  fclose(ofp);

  printf("Counter outputs go here.\n");

  return 0; 
}

// these functions are all need testing, testing for compiling right now

void updatePC(uint32_t i)
{
  regPC.addr = i;
  regPC.memPage = i >> 7;
  regPC.memOffset = i & 0x7f;
}

int getOpCode(uint32_t instruction)
{
  int opcode;
  opcode = 0;
  return opcode;
}

void handleMRI(uint32_t i)
{
  regMRI.indirectBit = i & 0x100;
  regMRI.memPageBit = i & 0x80;
  regMRI.offset = i & 0x7f;
}
