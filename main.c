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

struct regPC {		// holds address of next instruction
  uint32_t addr;
  int memPage;
  int memOffset;
};

struct regAC {		// struct .linkbit = 0, .accbits = 0-11
  int linkbit;
  uint32_t accbits;
};

struct regMRI {		// memory instruction register
  int opcode;
  int indirectBit;
  int memPageBit;
  int offset;
};

uint32_t regCPMA;	// holds address to be read/written from/to memory

uint32_t regMemBuff;	// holds value read from memory OR value to be written
uint32_t regSwitch;	// used to load memory from console

int signalRead;		// de-asserted 0, asserted 1
int signalWrite;	// de-asserted 0, asserted 1

uint32_t memPage;	// which of the 32 pages in memory
uint32_t memOffset;	// which 128 12-bit word on that page

int updatePC(uint32_t addr)
{
  regPC.addr = address;
  regPC.memPage = bits 0-4;
  regPC.memOffset = bits 5-11;
  return 0;
}

int getOpCode(uint32_t instruction)
{
  int opcode;
  opcode = bits 0-2;
  return opcode;
}

void handleMRI(uint32_t instruction, int opcode)
{
  regMRI.opcode = opcode;
  regMRI.indirectBit = bit 3;
  regMRI.memPageBit = bit 4;
  regMRI.offset = offset;
}

// Initialize a 2-dimensional (32 pages x 128 bytes) simulation of PDP-8 memory

int memory[PAGES][BYTES];

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
    // treat intput line as an address
    if (input[0] == '@')
       updatePC(input[1],input[2],input[3]) // treat as memory address

    // treat input line as an instruction
    else 
    {
      // input line is not a memory reference instruction
      opcode = getOpCode(input);
      if (opcode == 6 || opcode == 7)
      {
        deal with opcode 6 or 7;
      }

      // input line is just wrong
      else if (opcode > 7)
      {
        print error message to stderror;
      }

      // treat input line as a memory reference instruction
      else
      { 
        handleMRI(input, opcode);
        if (regMRI.memPageBit == 0)
        {
          regCPMA = 00000 + regMRI.offset;
        }
        else if (regMRI.memPageBit == 1 && regMRI.indirectBit == 1)
        {
          regCPMA = regPC.addr & regMRI.offset;
        }
        else if (regMRI.memPageBit == 1 && regMRI.indirectBit == 0)
        {
          regCPMA = mem[regPC.memPage][regMRI.offset];
        }
        else if (regMRI.memPageBit == 0 && regMRI.indirectBit == 0)
        {
          if (0010o < offset < 0017o)
          {
            regCPMA = mem[0][offset]++;
            countClock = countClock + 2;
          }
          else
          {
            regCPMA = mem[0][offset];
            countClock++;
          }
        }
        else if (regMRI.memPageBit > 1 || regMRI.indirectBit > 1)
        {
        print error message to stderror;
        }

        switch (instOpCode)
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
               regPC.addr++;
               updatePC(regPC.addr);
            else
               regCPMA++;
          case 3:
            countDCA++;
            regCPMA = regAC.addr;
            regAC.addr = 0;
            regAC.linkbit = 0;
          case 4:
            countJMS++;
            regCPMA = regPC;
            regPC.addr = regCPMA++;
            updatePC(addr);
          case 5:
            countJMP++;
            regPC.addr = regCPMA;
            updatePC(addr);
        } // end switch 

      } // end MRI handling

    } // end instruction handling

  fclose(ifp,ofp);

  printf("Counter outputs go here.");

  return 0; 
}

