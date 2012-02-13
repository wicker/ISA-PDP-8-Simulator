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

int countClock;	// total number of clock cycles

int countInstr;	// total number of instructions

int countAND;	// number of times AND was executed
int countTAD;	// number of times TAD was executed
int countISZ; 	// number of times ISZ was executed
int countDCA;	// number of times DCA was executed
int countJMS;	// number of times JMS was executed
int countJMP;	// number of times JMP was executed
int countIO;	// number of times IO was executed
int countMicro;	// number of times Micro was executed

typedef struct {	// holds address of next instruction
  int addr;
  int memPage;
  int memOffset;
} registerPC;

typedef struct {	// struct .linkbit = 0, .accbits = 0-11
  int addr;
  int linkbit;
  int accbits;
} registerAC;

typedef struct {	// memory instruction register
  int opcode;
  int indirectBit;
  int memPageBit;
  int offset;
} registerMRI;

typedef struct {
  int opcode : 3;
  int   bit3 : 1;
  int    cla : 1;
  int    cll : 1;
  int    cma : 1;
  int    cml : 1;
  int    rar : 1; 
  int    ral : 1;
  int rotate : 1;
  int    iac : 1;
} Group1bitfield;

typedef struct {
  int opcode : 3;
  int   bit3 : 1;
  int    cla : 1;
  int    sma : 1;
  int    sza : 1;
  int    snl : 1;
  int rotate : 1;
  int    osr : 1;
  int    hlt : 1;
  int  bit11 : 1;
} Group2bitfield;

void updatePC(int i);
int getOpCode(int instruction);
void handleMRI(int i);

// Initialize a 2-dimensional (32 pages x 128 bytes) simulation of PDP-8 memory

int memory[PAGES][BYTES];

registerPC regPC;
registerAC regAC;
registerMRI regMRI;

Group1bitfield group1;
Group2bitfield group2;

int regCPMA;	// holds address to be read/written from/to memory

int regMemBuff;	// holds value read from memory OR value to be written
int regSwitch;	// used to load memory from console

int signalRead;		// de-asserted 0, asserted 1
int signalWrite;	// de-asserted 0, asserted 1

int memPage;	// which of the 32 pages in memory
int memOffset;	// which 128 12-bit word on that page

FILE *ifp,*ofp;

int main()
{
  memset(memory, 0, sizeof(int));

  // open the tracefile, make it available to 'r' read
  // open the output file to make it available to append each iteration's result
  ifp = fopen("test.mem", "r");
  ofp = fopen("tracefile.din", "a");

  // set it up to read line by line, set n and addr accordingly
  int i;
  int carryout; // need to define/fix this
  char input[5];

  while (fscanf(ifp, "%s\n", &input) != EOF)
  {
    // treat input line as an address
    if (input[0] == '@')
    {
      printf("input[1] = %d\n",input[1]);
      printf("input[2] = %d\n",input[2]);
      printf("input[3] = %d\n",input[3]);
      int input1 = getHex(input[1]);
      printf("input1 = 0x%x\n",input1);
      int input2 = getHex(input[2]);
      printf("input2 = 0x%x\n",input2);
      int input3 = getHex(input[3]);
      printf("input3 = 0x%x\n",input3);
      i = input1*256 + input2*16 + input3;
      printf("i = 0x%x\n",i);
      updatePC(i);
    }
    // treat input line as an instruction
    else 
    {
      i = input[0]*256 + input[1]*16 + input[2];
      printf("input[0] = %d,input[1] = %d,input[2] = %d,input[3] = %d,i = %d\n",
             input[0],input[1],input[2],input[3],i);
      // input line is not a memory reference instruction
      regMRI.opcode = i << 9;
      printf("regMRI.opcode = %d\n",regMRI.opcode);
      if (regMRI.opcode == 6)
      { 
         printf("opcode 6 needs a nop with clock increment");
      }
      else if (regMRI.opcode == 7)
      {
        int bit11;
        int groupbit;
        groupbit = (i << 8) & 1;
        bit11 = i & 1;
        printf("groupbit = %d\n",groupbit);
        // dealing with a group 1 microinstruction
        if (groupbit == 0)
        {
           group1.opcode = regMRI.opcode;
           group1.bit3 = groupbit;
           group1.cla = (i << 7) & 1;
           group1.cll = (i << 6) & 1;
           group1.cma = (i << 5) & 1;
           group1.cml = (i << 4) & 1;
           group1.rar = (i << 3) & 1;
           group1.ral = (i << 2) & 1;
           group1.rotate = (i << 1) & 1;
           group1.iac = i & 1;

           // goup 1 micorinstructions here
        } 
        // dealing with a group 2 microinstruction
        else if (groupbit == 1 && bit11 == 0)
        {
           group2.opcode = regMRI.opcode;
           group2.bit3 = groupbit;
           group2.cla = (i << 7) & 1;
           group2.sma = (i << 6) & 1;
           group2.sza = (i << 5) & 1;
           group2.snl = (i << 4) & 1;
           group2.rotate = (i << 3) & 1;
           group2.osr = (i << 2) & 1;
           group2.hlt = (i << 1) & 1;
           group2.bit11 = bit11;

           // goup 2 micorinstructions here
        }
        // this is a nop, add in clock cycles as appropriate
        else
        {
          printf("nope"); 
        }
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
        countInstr++;
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

  printf("Total clock cycles: %d\n",countClock);
  printf("Total instructions: %d\n",countInstr);
  printf("Counts: AND, TAD, ISZ, DCA, JMS, JMP\n"
         "        %-4d %-4d %-4d %-4d %-4d %-4d\n",
         countAND,countTAD,countISZ,countDCA,countJMS,countJMP);

  return 0; 
}

// these functions are all need testing, testing for compiling right now

void updatePC(int i)
{
  regPC.addr = i;
  regPC.memPage = i >> 7;
  regPC.memOffset = i & 0x7f;
}

int getOpCode(int instruction)
{
  int opcode;
  opcode = 0;
  return opcode;
}

void handleMRI(int i)
{
  regMRI.indirectBit = i & 0x100;
  regMRI.memPageBit = i & 0x80;
  regMRI.offset = i & 0x7f;
}

void handleGroup1(int i)
{

}

void handleGroup2(int i)
{

}

int getHex(int num)
{
  printf("num: 0x%x\n",num);
  if (48 <= num && num <= 57)
     return num - 48;
  else if (97 <= num && num <= 102)
     return num - 87;
  else if (65 <= num && num <= 70)
     return num - 55;
  else
     return -1;
}
