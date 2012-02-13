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
int count7;	// number of times Micro was executed

typedef struct {	// holds address of next instruction
  int addr;
  int memPage;
  int memOffset;
} registerPC;

typedef struct {	// struct .linkbit = 0, .accbits = 0-11
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
  int    rev : 1;
  int    osr : 1;
  int    hlt : 1;
  int  bit11 : 1;
} Group2bitfield;

typedef struct {
  int n;
  int addr;
} forTracefile;

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

forTracefile trace;

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
      int input1 = getHex(input[1]);
      int input2 = getHex(input[2]);
      int input3 = getHex(input[3]);
      i = input1*256 + input2*16 + input3;
      updatePC(i);
    }
    // treat input line as an instruction
    else 
    {
      countInstr++;
      int input1 = getHex(input[0]);
      int input2 = getHex(input[1]);
      int input3 = getHex(input[2]);
      i = input1*256 + input2*16 + input3;
      regMRI.opcode = i >> 9;
      // input line is not a memory reference instruction
      if (regMRI.opcode == 6)
      {
         countIO++;
         printf("Warning: an I/O instruction was not simulated.\n");
      }
      else if (regMRI.opcode == 7)
      {
        count7++;
        countClock++;
        int bit11;
        int groupbit;
        groupbit = (i << 8) & 1;
        bit11 = i & 1;
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

           // group 1 microinstructions here

           if (group1.cla == 1)
           {
              regAC.accbits = 0;
           }
           if (group1.cll == 1) 
              regAC.linkbit = 0;
           if (group1.cma == 1)
           {
              /* complement the accumulator */
           }
           if (group1.rar == 1)
           {
              if (group1.rotate == 0)
                 /* rotate right 1 place */
                 { printf("1"); }
              else
                 /* rotate right 2 places */
                 { printf("1"); }
           }
           if (group1.ral == 1)
           {
              if (group1.rotate == 0)
                 /* rotate left 1 place */
              { 
                 printf("1");
                 
              }
              else
                 /* rotate left 2 places */
              { 
                 printf("1");
 
              }
           }
           if (group1.iac == 1)
           {
              regAC.accbits++;
           }
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
           group2.rev = (i << 3) & 1; // reverse for spa, sna, szl
           group2.osr = (i << 2) & 1;
           group2.hlt = (i << 1) & 1;
           group2.bit11 = bit11;

           // group 2 microinstructions here

           
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
          printf("Something's gone badly wrong.");
        }

        switch (regMRI.opcode)
        {
          case 0:
            countAND++;
            countClock = countClock + 2;
            regAC.accbits = regCPMA & regAC.accbits;
            trace.n = 0;
            trace.addr = regCPMA;
            break;
          case 1:
            countTAD++;
            countClock = countClock + 2;
            regCPMA = -regCPMA;
            regAC.accbits = regCPMA & regAC.accbits;
            if (carryout)
               regAC.linkbit = -regAC.linkbit; 
            trace.n = 0;
            trace.addr = regCPMA;
            break;
          case 2:
            countClock = countClock + 2;
            countISZ++;
            if (regCPMA == 0)
            {
               regPC.addr++;
               updatePC(regPC.addr);
            }
            else
               regCPMA++;
            trace.n = 1;
            trace.addr = regCPMA;
            break;
          case 3:
            countClock = countClock + 2;
            countDCA++;
            regCPMA = regAC.accbits;
            regAC.accbits = 0;
            regAC.linkbit = 0;
            trace.n = 1;
            trace.addr = regCPMA;
            break;
          case 4:
            countClock = countClock + 2;
            countJMS++;
            regCPMA = regPC.addr;
            regPC.addr = regCPMA++;
            updatePC(regPC.addr);
            trace.n = 1;
            trace.addr = regCPMA;
            break;
          case 5:
            countJMP++;
            countClock++;
            regPC.addr = regCPMA;
            updatePC(regPC.addr);
            trace.n = 2;
            trace.addr = regCPMA;
            break;
        } // end switch 

        fprintf(ofp,"%d %o\n",trace.n,trace.addr);
        fflush(ofp);

      } // end MRI handling

    } // end instruction handling

  } // end while loop

  fclose(ifp);
  fclose(ofp);

  printf("Total clock cycles: %d\n",countClock);
  printf("Total instructions: %d\n",countInstr);
  printf("Counts: AND, TAD, ISZ, DCA, JMS, JMP, IO, Micro\n"
         "        %-4d %-4d %-4d %-4d %-4d %-4d %-3d %-6d\n",
         countAND,countTAD,countISZ,countDCA,countJMS,countJMP,countIO,count7);

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
  regMRI.indirectBit = (i >> 8) & 1;
  regMRI.memPageBit = (i >> 7) & 1;
  regMRI.offset = i & 0x7f;
}

int getHex(int num)
{
  if (48 <= num && num <= 57)
     return num - 48;
  else if (97 <= num && num <= 102)
     return num - 87;
  else if (65 <= num && num <= 70)
     return num - 55;
  else
     return -1;
}
