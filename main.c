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
  int addr;
  int opcode;
  int indirectBit;
  int memPageBit;
  int page;
  int offset;
} registerIR;

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

void loadAllTheThings();
void updatePC(int i);
int getOpCode(int instruction);
void handleIR(int i);

// Initialize a 2-dimensional (32 pages x 128 bytes) simulation of PDP-8 memory

int memory[PAGES][BYTES];

registerPC regPC;
registerAC regAC;
registerIR regIR;

Group1bitfield group1;
Group2bitfield group2;

forTracefile trace;

int regCPMA;

FILE *ifp,*ofp;

int main()
{
  memset(memory, 0, sizeof(int));

  // open the tracefile, make it available to 'r' read
  // open the output file to append each iteration's result
  ofp = fopen("tracefile.din", "a");

  // parse the input file and store instructions/data in memory as directed
  loadAllTheThings();

  // starting from the first address, use assembly file to make tracefile
  int i;
  i = regIR.addr;
  updatePC(i);

  int carryout; // still no idea what this is
 
  // while the instruction is not a halt, follow instructions
  while (group2.hlt != 1)
  {
      countInstr++;

      // input line is an I/O instruction (not handled here)
      if (regIR.opcode == 6)
      {
         countIO++;
         printf("Warning: an I/O instruction was not simulated.\n");
      }

      // input line is a group 1, 2, or 3 microinstruction
      else if (regIR.opcode == 7)
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
           group1.opcode = regIR.opcode;
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
           group2.opcode = regIR.opcode;
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
         // we do not handle group 3 instructions, need clock cycles
         else
         {
           printf("nope"); 
         }
      } // end opcode 7

      // if input line is just wrong
      else if (regIR.opcode > 7)
      {
         printf("opcode %d is greater than 7\n",regIR.opcode);
      }

      // treat input line as a memory reference instruction
      else
      { 
        handleIR(i);
        // calculate the effective address, store in regCPMA
        // zero page, indirection
        if (regIR.memPageBit == 0 && regIR.indirectBit == 1)
        {
          regCPMA = memory[0][regIR.offset];
        }
        // current page, indirection
        else if (regIR.memPageBit == 1 && regIR.indirectBit == 1)
        {
          regCPMA = memory[regIR.page][regIR.offset];
        }
        // current page, no indirection
        else if (regIR.memPageBit == 1 && regIR.indirectBit == 0)
        {
          regCPMA = regIR.page + regIR.offset;
        }
        // zero page, autoindexing (registers 0010o-0017o)
        else if (regIR.memPageBit == 0 && regIR.indirectBit == 0)
        {
          if (0x8 < regIR.offset < 0xf)
          {
            regCPMA = memory[0][regIR.offset]++;
            countClock = countClock + 2;
          }
          else
          {
            regCPMA = memory[0][regIR.offset];
            countClock++;
          }
        }
        else if (regIR.memPageBit > 1 || regIR.indirectBit > 1)
        {
          printf("Something's gone badly wrong.");
        }

        updatePC(regCPMA);

        switch (regIR.opcode)
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

      } // end IR handling

  } // end while loop

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

void handleIR(int i)
{
  regIR.addr = i;
  regIR.page = i >> 7;
  regIR.indirectBit = (i >> 8) & 1;
  regIR.memPageBit = (i >> 7) & 1;
  regIR.offset = i & 0x7f;
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

void loadAllTheThings()
{
  int count;
  int input1;
  int input2;
  int input3;
  int i;
  char input[5];

  ifp = fopen("test.mem", "r");
  while (fscanf(ifp, "%s\n", &input) != EOF)
  {
    // treat input line as an address
    if (input[0] == '@')
    { 
      count++;
      // get the address in hex
      input1 = getHex(input[1]);
      input2 = getHex(input[2]);
      input3 = getHex(input[3]);
      i = input1*256 + input2*16 + input3;
      // the first @ represents the address of the first instruction
      if (count == 1)
      {	
         updatePC(i);
         handleIR(i);
      }
      // the second @ represents the address of the first data line
      else if (count == 2)
      {
         updatePC(i);
      }
      else 
      {
         printf("This is the third @ and doesn't follow form.");
      }
    }
    // treat input line as an instruction or data line
    else 
    {
      // store the instruction at the correct place in memory
      if (count == 1)
      {
         input1 = getHex(input[0]);
         input2 = getHex(input[1]);
         input3 = getHex(input[2]);
         i = input1*256 + input2*16 + input3;
         memory[regPC.memPage][regPC.memOffset] = i;
         updatePC(regPC.addr++);
      }
      // store the data line at the correct place in memory
      // using PC here
      else if (count == 2)
      {
         input1 = getHex(input[0]);
         input2 = getHex(input[1]);
         input3 = getHex(input[2]);
         i = input1*256 + input2*16 + input3;
         memory[regPC.memPage][regPC.memOffset] = i;
         updatePC(regPC.addr++); 
      }
      else 
      {
         printf("The first line of the input file did not contain an @.");
      }
    }
  }
  fclose(ifp);
}


