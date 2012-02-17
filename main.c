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
  int page;
  int offset;
} registerPC;

typedef struct {	// struct .link = 0, .addr = 0-11
  int link;
  int addr;
} registerAC;

typedef struct {	// memory instruction register
  int addr;
  int opcode;
  int indirectBit;
  int pageBit;
  int page;
  int offset;
} registerIR;

typedef struct {
  int opcode;
  int   bit3;
  int    cla;
  int    cll;
  int    cma;
  int    cml;
  int    rar; 
  int    ral;
  int rotate;
  int    iac;
} Group1bitfield;

typedef struct {
  int opcode;
  int   bit3;
  int    cla;
  int    sma;
  int    spa;
  int    sna;
  int    szl;
  int    sza;
  int    snl;
  int    rev;
  int    skp;
  int    osr;
  int    hlt;
  int  bit11;
} Group2bitfield;

typedef struct {
  int n;
  int addr;
} forTracefile;

int loadAllTheThings();
int updateCPMA(int);
int updatePC(int);
int handleIR(int);
int getEffAddr();
int group1If(int,int,int);
int group2If(int,int,int,int);
void displayGroup1();
void displayGroup2();
void displayMemory();

// Initialize a 2-dimensional (32 pages x 128 bytes) simulation of PDP-8 memory

int memory[PAGES][BYTES];

registerPC regPC;
registerAC regAC;
registerIR regIR;
registerPC regCPMA;

Group1bitfield group1;
Group2bitfield group2;

forTracefile trace;

FILE *ifp,*ofp;

int main()
{
  memset(memory, 0, sizeof(int));

  // parse the input file and store instructions/data in memory as directed
  loadAllTheThings();

  // starting from the first address, use assembly file to make tracefile
  int i;
  updatePC(regIR.addr); // PC now has the address of the first instruction
  int carryout; // still no idea what this is
 
  // while the instruction is not a halt, follow instructions
  int c;
  int opcode;
//  for (c = 0;c < 7; c++) // testing
  while (group2.hlt != 1)
  {
      // open the output file to append each iteration's result
      ofp = fopen("tracefile.din", "a");

      countInstr++;

      i = memory[regPC.page][regPC.offset];
      opcode = i >> 9;
      // treat input line as a memory reference instruction
      if (opcode < 6)
      {
        handleIR(i);
        getEffAddr();
        switch (regIR.opcode)
        {
          case 0:
            countAND++;
            countClock = countClock + 2;
            regAC.addr = regAC.addr & memory[regCPMA.page][regCPMA.offset];
            updatePC(regPC.addr + 1);
            trace.n = 0;
            trace.addr = regCPMA.addr;
            break;
          case 1:
            countTAD++;
            countClock = countClock + 2;
            regAC.addr = regAC.addr + -memory[regCPMA.page][regCPMA.offset];
            if (carryout) //!!! carryout?
               regAC.link = -regAC.link;
            updatePC(regPC.addr + 1); 
            trace.n = 0;
            trace.addr = regCPMA.addr;
            break;
          case 2:
            countClock = countClock + 2;
            countISZ++;
            memory[regCPMA.page][regCPMA.offset] = memory[regCPMA.page][regCPMA.offset] + 1;
            if (memory[regCPMA.page][regCPMA.offset] == 0)
            {
               regPC.addr = regPC.addr + 1;
               updatePC(regPC.addr);
            }
            regPC.addr = regPC.addr;
            trace.n = 1;
            trace.addr = regCPMA.addr;
            break;
          case 3:
            countClock = countClock + 2;
            countDCA++;
            memory[regCPMA.page][regCPMA.offset] = regAC.addr;
            regAC.addr = 0;
            regAC.link = 0;
            updatePC(regPC.addr + 1);
            trace.n = 1;
            trace.addr = regCPMA.addr;
            break;
          case 4:
            countClock = countClock + 2;
            countJMS++;
            updateCPMA(regPC.addr);
            regPC.addr = regCPMA.addr + 1;
            updatePC(regPC.addr);
            trace.n = 1;
            trace.addr = regCPMA.addr;
            break;
          case 5:
            countJMP++;
            countClock++;
            regPC.addr = regCPMA.addr;
            updatePC(regPC.addr);
            trace.n = 2;
            trace.addr = regCPMA.addr;
            break;
        } // end switch 

      }

      // input line is an I/O instruction (not handled here)
      else if (opcode == 6)
      {
         countIO++;
         printf("Warning: an I/O instruction was not simulated.\n");
      }

      // input line is a group 1, 2, or 3 microinstruction
      else if (opcode == 7)
      {
         count7++;
         countClock++;
         int bit11;
         int groupbit;
         groupbit = (i >> 8) & 1;
         bit11 = i & 1;
         // dealing with a group 1 microinstruction
         if (groupbit == 0)
            group1If(i, opcode, groupbit);
         // dealing with a group 2 microinstruction
         else if (groupbit == 1 && bit11 == 0)
            group2If(i, opcode, groupbit, bit11);
         // we do not handle group 3 instructions, need clock cycles
         else
           printf("Warning: Group 3 instructions not supported\n");
      } // end opcode 7

      // if input line is just wrong
      else if (opcode > 7)
         printf("Warning: Opcode %d is greater than 7\n",regIR.opcode);

      if (group2.hlt != 1)
      {
         fprintf(ofp,"%d %o\n",trace.n,trace.addr);
         fflush(ofp);
      }

  } // end while loop

  fclose(ofp);

  printf("\n_______________ OUTPUT ____________________\n");
  printf("\nTotal clock cycles: %d\n",countClock);
  printf("Total instructions: %d\n",countInstr);
  printf("Counts: AND, TAD, ISZ, DCA, JMS, JMP, IO, Micro\n\n"
         "        %-4d %-4d %-4d %-4d %-4d %-4d %-3d %-6d\n\n",
         countAND,countTAD,countISZ,countDCA,countJMS,countJMP,countIO,count7);

  return 0; 
}

// these functions are all need testing, testing for compiling right now

int group1If(int i, int opcode, int groupbit)
{
   group1.opcode = opcode;
   group1.bit3 = groupbit;
   group1.cla = (i >> 7) & 1;
   group1.cll = (i >> 6) & 1;
   group1.cma = (i >> 5) & 1;
   group1.cml = (i >> 4) & 1;
   group1.rar = (i >> 3) & 1;
   group1.ral = (i >> 2) & 1;
   group1.rotate = (i >> 1) & 1;
   group1.iac = i & 1;

   /* displayGroup1(); */

   // group 1 microinstructions
   if (group1.cla == 1) /* clear the accumulator */
      regAC.addr = 0;
   if (group1.cll == 1) /* clear the link bit */
      regAC.link = 0;
   if (group1.cma == 1) /* complement the accumulator */
      regAC.addr = ~regAC.addr;
   if (group1.cml == 1) /* complement the link  */
      regAC.link = ~regAC.link;
   if (group1.iac == 1) /* increment the accumulator */
      regAC.addr = regAC.addr + 1;
   if (group1.rar == 1 && group1.rotate == 0) /* rotate right 1 place */
      regAC.addr = (0xfff & (regAC.addr >> 1)) || (regAC.addr << 11);
   if (group1.rar == 1 && group1.rotate == 1) /* rotate right 2 places */
      regAC.addr = (0xfff & (regAC.addr >> 2)) || (regAC.addr << 10);
   if (group1.ral == 1 && group1.rotate == 0) /* rotate left 1 place */
      regAC.addr = (0xfff & (regAC.addr << 1)) || (regAC.addr >> 11);
   if (group1.ral == 1 && group1.rotate == 1) /* rotate left 2 places */
      regAC.addr = (0xfff & (regAC.addr << 2)) || (regAC.addr >> 10);
   else // no clock cycle for this: no operation
      countClock--;
   updatePC(regPC.addr + 1);
   return 0;
}

int group2If(int i, int opcode, int groupbit, int bit11)
{
   group2.opcode = opcode;
   group2.bit3 = groupbit;
   group2.cla = (i >> 7) & 1;
   group2.sma = (i >> 6) & 1;
   group2.sza = (i >> 5) & 1;
   group2.snl = (i >> 4) & 1;
   group2.rev = (i >> 3) & 1; // reverse for spa, sna, szl
   group2.osr = (i >> 2) & 1;
   group2.hlt = (i >> 1) & 1;
   group2.bit11 = bit11;

   /* displayGroup2(); */
 
   // group 2 microinstructions here
   if (group2.sma == 1) // skip if regAC < 0
   {
      if (regAC.addr < 0)
         updatePC(regPC.addr + 1);
   }
   if (group2.sza == 1) // skip if regAC == 0
   {
      if (regAC.addr == 0)
         updatePC(regPC.addr + 1);
   }
   if (group2.snl == 1) // skip if link nonzero
  {
      if (regAC.link != 0)
         updatePC(regPC.addr + 1);
   }
   if (group2.spa == 1) // skip if regAC > 0
   {
      if (regAC.addr > 0)
         updatePC(regPC.addr + 1);
   }
   if (group2.sna == 1) // skip if regAC == 1
   {
      if (regAC.addr != 0)
         updatePC(regPC.addr + 1);
   }
   if (group2.szl == 1) // skip if link == 0
   {
      if (regAC.link == 0)
          updatePC(regPC.addr + 1);
   }
   if (group2.skp == 1) // skip no matter what
      updatePC(regPC.addr + 1);
   if (group2.cla == 1) // clear accumulator
      updatePC(regPC.addr + 1);
   if (group2.osr == 1) // we don't support this
   {
      printf("Warning: Group 2 instruction ORs the SR with the AC; nop.");
   }
   updatePC(regPC.addr + 1);
   return 0;
}

int updatePC(int i)
{
  regPC.addr = i;
  regPC.page = i >> 7;
  regPC.offset = i & 0x7f;
}

int updateCPMA(int i)
{
  regCPMA.addr = i;
  regCPMA.page = i >> 7;
  regCPMA.offset = i & 0x7f;
}

int handleIR(int i)
{
  regIR.addr = i;
  regIR.page = i >> 7;
  regIR.indirectBit = (i >> 8) & 1;
  regIR.pageBit = (i >> 7) & 1;
  regIR.offset = i & 0x7f;
  regIR.opcode = i >> 9;
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

int loadAllTheThings()
{
  int count;
  int input1;
  int input2;
  int input3;
  int i;
  char input[5];
  count = 0;

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
      // this is saved in the IR for use at the start of the program
      // the second @ represents the address of the first data line
      updatePC(i);
      if (count == 1)
      {	
        handleIR(i);
      }
      else if (count > 2)
      {
         printf("Warning: Input file lines after the third @ were ignored;\n "
                          "    the program will now execute.\n\n");
         break;
      }
    } // end address store
    // treat input line as an instruction or data line
    else 
    {
      // store the instruction or data at the correct place in memory
         if (count == 0)
         {
            printf("Warning: The first line of the input file did not "
                             "contain an @. Halting program.\n");
	    break;
         }
         input1 = getHex(input[0]);
         input2 = getHex(input[1]);
         input3 = getHex(input[2]);
         i = input1*256 + input2*16 + input3;
         memory[regPC.page][regPC.offset] = i;
         updatePC(regPC.addr + 1);

    } // end instruction store
  }
  /* optionally display memory pages for testing */
  /* displayMemory(); */

  fclose(ifp);
  return 0;
}

int getEffAddr()
{
        // calculate the effective address, store in regCPMA
        // zero page, indirection
        if (regIR.pageBit == 0 && regIR.indirectBit == 1)
        {
          regCPMA.addr = memory[0][regIR.offset];
          countClock++;
        }
        // current page, indirection
        else if (regIR.pageBit == 1 && regIR.indirectBit == 1)
        {
          regCPMA.addr = memory[regIR.page][regIR.offset];
          countClock++;
        }
        // current page, no indirection
        else if (regIR.pageBit == 1 && regIR.indirectBit == 0)
        {
          regCPMA.addr = regIR.page + regIR.offset;
        }
        // zero page, autoindexing (registers 0010o-0017o)
        else if (regIR.pageBit == 0 && regIR.indirectBit == 0)
        {
          if (0x8 < regIR.offset < 0xf)
          {
            regCPMA.addr = memory[0][regIR.offset]++;
            countClock = countClock + 2;
          }
          else
          {
            regCPMA.addr = memory[0][regIR.offset];
            countClock++;
          }
        }
        else if (regIR.pageBit > 1 || regIR.indirectBit > 1)
        {
          printf("Something's gone badly wrong.");
        }
	return 0;
}

void displayMemory()
{
  printf("Contents of the first pages of memory:\n");
  int p; 
  int y;
  for (p = 0; p < 3; p++)
  {
    for (y = 0; y < 120; y++)
    { 
      printf("%x ",memory[p][y]);
    }
    printf("\n\n");
  }
}

void displayGroup1()
{
  printf("grp1: op bit3 cla cll cma cml rar ral rot iac\n"
         "      %-3d %-4d %-3d %-3d %-3d %-3d %-3d %-3d %-3d %-3d\n",
         group1.opcode, group1.bit3, group1.cla, group1.cll,
         group1.cma, group1.cml, group1.rar, group1.ral,
         group1.rotate, group1.iac);
}

void displayGroup2()
{
  printf("grp2: op bit3 cla sma sza snl rev osr hlt bit11\n"
         "      %-3d %-4d %-3d %-3d %-3d %-3d %-3d %-3d %-3d %-5d\n",
         group2.opcode, group2.bit3, group2.cla, group2.sma,
         group2.sza, group2.snl, group2.rev, group2.osr,
         group2.hlt, group2.bit11);
}
