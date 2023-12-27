/*
 * Project 4
 * EECS 370 LC-2K Instruction-level simulator
 *
 * Make sure *not* to modify printState or any of the associated functions
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Machine Definitions
#define NUMMEMORY 65536 /* maximum number of words in memory */
#define NUMREGS 8 /* number of machine registers */

// File
#define MAXLINELENGTH 1000 /* MAXLINELENGTH is the max number of characters we read */

typedef struct stateStruct {
    int pc;
    int mem[NUMMEMORY];
    int reg[NUMREGS];
    int numMemory;
} stateType;

extern void cache_init(int blockSize, int numSets, int blocksPerSet);
extern int cache_access(int addr, int write_flag, int write_data);
extern void printStats(void);
static stateType state;
static int num_mem_accesses = 0;
int mem_access(int addr, int write_flag, int write_data) {
    ++num_mem_accesses;
    if (write_flag) {
        state.mem[addr] = write_data;
    }
    return state.mem[addr];
}

int get_num_mem_accesses(void){
    return num_mem_accesses;
}

void printState(stateType *);

int convertNum(int);

void translateMC(int *, int);

void repBin(char *, int);

void setFields(int *, char *);

int subBin(char *, int, int);

int runInst(int *, stateType *);

int nor(int, int);

// MAIN

int main(int argc, char *argv[]){
    freopen("p4spec.1.1.1.rahulu.out", "w", stdout);
    char line[MAXLINELENGTH];
    FILE *filePtr;

    if (argc != 5) {
        printf("error: usage: %s <machine-code file> <blockSizeInWords> <numberOfSets> <blocksPerSet> \n", argv[0]);
        exit(1);
    }

    filePtr = fopen(argv[1], "r");
    if (filePtr == NULL) {
        printf("error: can't open file %s", argv[1]);
        perror("fopen");
        exit(1);
    }
    
    cache_init(atoi(argv[2]),atoi(argv[3]), atoi(argv[4]));

    /* read the entire machine-code file into memory */
    for (state.numMemory = 0; fgets(line, MAXLINELENGTH, filePtr) != NULL; state.numMemory++) {
        if (sscanf(line, "%d", state.mem+state.numMemory) != 1) {
            printf("error in reading address %d\n", state.numMemory);
            exit(1);
        }
        // printf("memory[%d]=%d\n", state.numMemory, state.mem[state.numMemory]);
    }

    state.pc = 0; // start pc counter at 0
    
    for(int i = 0; i < NUMREGS; i++){ // initialize all regs to 0
        state.reg[i] = 0;
    }
    
    /*
     opcodes: add, nor, lw, sw, beq, jalr, halt, [fill]
     
     */
    
    int instructions[state.numMemory][5]; //opcode, arg0, arg1, arg2, [.fill]
    
    int halted = 0;
    
    while(halted == 0){
        translateMC(instructions[state.pc], cache_access(state.pc, 0, 0));
        halted = runInst(instructions[state.pc], &state); // run instructions until program is halted
    }
    
    printState(&state);
    printf("Main memory words accessed: %d\n", get_num_mem_accesses());
    printStats();
    
    return(0);
}

void printState(stateType *statePtr)
{
    int i;
    printf("\n@@@\nstate:\n");
    printf("\tpc %d\n", statePtr->pc);
    printf("\tmemory:\n");
    for (i=0; i<statePtr->numMemory; i++) {
              printf("\t\tmem[ %d ] %d\n", i, statePtr->mem[i]);
    }
    printf("\tregisters:\n");
    for (i=0; i<NUMREGS; i++) {
              printf("\t\treg[ %d ] %d\n", i, statePtr->reg[i]);
    }
    printf("end state\n");
}

int convertNum(int num)
{
    /* convert a 16-bit number into a 32-bit Linux integer */
    if (num & (1<<15) ) {
        num -= (1<<16);
    }
    return(num);
}

void translateMC(int *instArr, int mCode){
    char binRep[] = "00000000000000000000000000000000";
    repBin(binRep, mCode); // translate current line of memory to binary
    instArr[0] = subBin(binRep, 22, 24); // set opcode field of instructions based on binRep
    setFields(instArr, binRep); // set arg fields of instructions based on binRep
}

void repBin(char *binaryInitial, int decimal){ // converts a decimal number to binary
    for(int i = 0; i < 32; i++){
        if((decimal >> i) & 1){ // if iTh bit is 1, set bit of string to 1
            binaryInitial[31-i] = '1';
        }
    }
}

void setFields(int *instArr, char *binary){
    if(instArr[0] < 2){ // if R type instruction
        instArr[1] = subBin(binary, 19, 21);
        instArr[2] = subBin(binary, 16, 18);
        instArr[3] = subBin(binary, 0, 2);
    }
    else if(instArr[0] < 5){ // if I type instruction
        instArr[1] = subBin(binary, 19, 21);
        instArr[2] = subBin(binary, 16, 18);
        instArr[3] = subBin(binary, 0, 15);
        
        if(instArr[3] > 32767){
            instArr[3] = convertNum(instArr[3]);
        }
    }
    else if(instArr[0] < 6){ // if J type instruction
        instArr[1] = subBin(binary, 19, 21);
        instArr[2] = subBin(binary, 16, 18);
        instArr[3] = instArr[4] = 0;
    }
    else{ // if O type instruction
        instArr[1] = instArr[2] = instArr[3] = instArr[4] = 0;
    }
    return;
}

int subBin(char *binary, int k, int m){ // translates bits [k,m] to decimal
    int dec = 0;
    int j = 0;
    
    for(int i = (31 - k); i >= (31 - m); i--){
        if(binary[i] == '1'){
            dec += (1 << j);
        }
        j++;
    }
    return dec;
}

int runInst(int *inArr, stateType* statePtr){
    if(inArr[0] == 6){ // halt
        statePtr->pc++; // increase pc
        return 1; // halted
    }
    else if(inArr[0] == 7){ // noop
        statePtr->pc++;
    }
    else if(inArr[0] == 0){ // add
        statePtr->pc++;
        statePtr->reg[inArr[3]] =  statePtr->reg[inArr[1]] +  statePtr->reg[inArr[2]];
    }
    else if(inArr[0] == 1){ // nor
        statePtr->pc++;
        statePtr->reg[inArr[3]] =  nor(statePtr->reg[inArr[1]], statePtr->reg[inArr[2]]);
    }
    else if(inArr[0] == 2){ // lw
        statePtr->pc++;
        statePtr->reg[inArr[2]] = cache_access(((statePtr->reg[inArr[1]]) + inArr[3]), 0, 0);
    }
    else if(inArr[0] == 3){ // sw
        statePtr->pc++;
        if(statePtr->numMemory <= ((statePtr->reg[inArr[1]]) + inArr[3])){ //if we are trying to store something on the stack
            statePtr->numMemory = ((statePtr->reg[inArr[1]]) + inArr[3] + 1);
        }
        cache_access(((statePtr->reg[inArr[1]]) + inArr[3]), 1, statePtr->reg[inArr[2]]);
    }
    else if(inArr[0] == 4){ // beq
        statePtr->pc++;
        if(statePtr->reg[inArr[1]] == statePtr->reg[inArr[2]]){
            statePtr->pc += inArr[3];
        }
    }
    else if(inArr[0] == 5){ // jalr
        statePtr->pc++;
        statePtr->reg[inArr[2]] = statePtr->pc;
        statePtr->pc = statePtr->reg[inArr[1]];
    }
    
    
    return 0; // only return 1 if halt opcode;
}

int nor(int a, int b){
    char aBin[] = "00000000000000000000000000000000";
    char bBin[] = "00000000000000000000000000000000";
    char norBin[] = "00000000000000000000000000000000";
    repBin(aBin, a);
    repBin(bBin, b);
    
    for(int i = 31; i >= 0; i--){
        if((aBin[i] == '0') && (bBin[i] == '0')){
            norBin[i] = '1';
        }
    }
    
    return(subBin(norBin, 0, 31));
}
