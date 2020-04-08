#include <stdio.h>
#include "reg_code.h"

#define MAX_CODE 100

struct _code {
    int opcode;
    int operand1,operand2,operand3;
    char *s_operand;
} Codes[MAX_CODE];

int n_code;


void initGenCode()
{
    n_code = 0;
}

void genCode1(int opcode,int operand1)
{
    Codes[n_code].operand1 = operand1;
    Codes[n_code++].opcode = opcode;
}

void genCode2(int opcode,int operand1, int operand2)
{
    Codes[n_code].operand1 = operand1;
    Codes[n_code].operand2 = operand2;
    Codes[n_code++].opcode = opcode;
}

void genCode3(int opcode,int operand1, int operand2, int operand3)
{
    Codes[n_code].operand1 = operand1;
    Codes[n_code].operand2 = operand2;
    Codes[n_code].operand3 = operand3;
    Codes[n_code++].opcode = opcode;
}

void genCodeS(int opcode,int operand1, int operand2, char *s)
{
    Codes[n_code].operand1 = operand1;
    Codes[n_code].operand2 = operand2;
    Codes[n_code].s_operand = s;
    Codes[n_code++].opcode = opcode;
}

/* 
 *  code generator for x86
 */ 

#define N_REG 4
#define N_SAVE 4

#define TMP_OFF(i) 	-((i+1)+1)*4
#define LOCAL_VAR_OFF(i) 	-(N_SAVE+1+(i+1))*4
#define ARG_OFF(i)	((i)+2)*4

#define REG_AX 0
#define REG_BX 1
#define REG_CX 2
#define REG_DX 3

#define MAX_ARGS 6 /* for Darwin */

char *tmpRegName[N_REG] = { "eax", "ebx", "ecx", "edx" };
int tmpRegState[N_REG];
int tmpRegSave[N_SAVE];

void initTmpReg();
int getReg(int r);
void assignReg(int r, int reg);
int useReg(int r);
void freeReg(int reg);
void saveReg(int reg);
void saveAllRegs();

/*
 * sample register allocation
 */
void initTmpReg()
{
    int i;
    for(i = 0; i < N_REG; i++) tmpRegState[i] = -1;
    for(i = 0; i < N_SAVE; i++) tmpRegSave[i] = -1;
}

/* getReg: get free register */
int getReg(int r)
{
    int i;
    for(i = 0; i < N_REG; i++){
	if(tmpRegState[i] < 0){
	    tmpRegState[i] = r;
	    return i;
	}
    }
    error("no temp reg");
}

/* assign r to reg */
void assignReg(int r, int reg)
{
    if(tmpRegState[reg] == r) return;
    saveReg(reg);
    tmpRegState[reg] = r;
}

/* load r into reg */
int useReg(int r)
{
    int i,rr;

    for(i = 0; i < N_REG; i++){
	if(tmpRegState[i] == r) return i;
    }
    /* not found in register, then restore from save area. */
    for(i = 0; i < N_SAVE; i++){
	if(tmpRegSave[i] == r){
	    rr = getReg(r);
	    tmpRegSave[i] = -1;
	    /* load into regsiter */
	    printf("\tmov\t[ebp%+d],%s\n",tmpRegName[rr],TMP_OFF(i));
	    return rr;
	}
    }
    error("reg is not found");
}

void freeReg(int reg)
{
    tmpRegState[reg] = -1;
}

void saveReg(int reg)
{
    int i;

    if(tmpRegState[reg] < 0) return;
    for(i = 0; i < N_SAVE; i++){
	if(tmpRegSave[i] < 0){
	    printf("\tmov \t[ebp%+d],%s\n",TMP_OFF(reg),tmpRegName[reg]);
	    tmpRegSave[i] = tmpRegState[reg];
	    tmpRegState[reg] = -1;
	    return;
	}
    }
    error("no temp save");
}

void saveAllRegs()
{
    int i;
    for(i = 0; i < N_REG; i++) saveReg(i);
}

/*
 * Code generation
 */
extern int label_counter;

void genFuncCode(char *entry_name, int n_local)
{
    int i;
    int opd1,opd2,opd3;
    int r,r1,r2;
    char *opds;
    int ret_lab,l1,l2;
    int frame_size;

    /* function header */
    puts("\n");       	             /* 一个空行      */
    puts("section .text");       	             /* .text         */
    printf("\textern _printf, _println\n");    /* .globl <name> */
    printf("\tglobal\t_%s\n", entry_name);    /* .globl <name> */
    
    printf("_%s:\n", entry_name);             /* 用户自己定义的函数名字<name>:              */
    
    printf("\tpush\tebp\n");
    printf("\tmov \tebp,esp\n");

    frame_size = -LOCAL_VAR_OFF(n_local);
    ret_lab = label_counter++;

    printf("\t;ret_lab=%d\n",ret_lab);
    printf("\tsub \tesp,%d\n",frame_size);
    printf("\tmov \tebx,[ebp-4]\n");
    
    initTmpReg();
    
    for(i = 0; i < n_code; i++){
	/*debug*//* printf("%s %d %d %d\n",code_name(Codes[i].opcode),
	       Codes[i].operand1,Codes[i].operand2,Codes[i].operand3); */
	opd1 = Codes[i].operand1;
	opd2 = Codes[i].operand2;
	opd3 = Codes[i].operand3;
	opds = Codes[i].s_operand;

    switch(Codes[i].opcode){
	case LOADI:
	    if(opd1 < 0) break;
	    r = getReg(opd1);
	    printf("\tmov \t%s,%d\n",tmpRegName[r],opd2);//Jason
	    break;
	case LOADA:	/* load arg */
	    if(opd1 < 0) break;
	    r = getReg(opd1);
	    printf("\tmov \t%s,[ebp%+d]\n",tmpRegName[r],ARG_OFF(opd2));
	    break;
	case LOADL:	/* load local */
	    if(opd1 < 0) break;
	    r = getReg(opd1);
	    printf("\tmov \t%s,[ebp%+d]\n",tmpRegName[r],LOCAL_VAR_OFF(opd2));
	    break;
	case STOREA:	/* store arg */
	    r = useReg(opd1); freeReg(r);
	    printf("\tmov \t[ebp%+d],%s\n",ARG_OFF(opd2),tmpRegName[r]);
	    break;
	case STOREL:	/* store local */
	    r = useReg(opd1); freeReg(r);
	    printf("\tmov \t[ebp%+d],%s\n",LOCAL_VAR_OFF(opd2),tmpRegName[r]);
	    break;
	case BEQ0:	/* conditional branch */
	    r = useReg(opd1); freeReg(r);
	    printf("\tcmp \t%s,0\n",tmpRegName[r]);
	    printf("\tje\t.L%d\n",opd2);
	    break;
	case LABEL:
	    printf(".L%d:\n",Codes[i].operand1);
	    break;
	case JUMP:
	    printf("\tjmp\t.L%d\n",Codes[i].operand1);
	    break;

	case CALL:
	    saveAllRegs();
	    printf("\tcall\t_%s\n",opds);
	    if(opd1 < 0) break;
	    assignReg(opd1,REG_AX);
	    printf("\tadd esp, %d\n",opd2*4);
	    break;
	case ARG:
	    r = useReg(opd1); freeReg(r);
	    printf("\tpush \t%s\n",tmpRegName[r]); //Jason
	    break;
	case RET:
	    r = useReg(opd1); freeReg(r);
	    if(r != REG_AX) printf("\tmov\teax,%s\n",tmpRegName[r]);
	    printf("\tjmp .L%d\n",ret_lab);
	    break;

	case ADD:
	    r1 = useReg(opd2); r2 = useReg(opd3);
	    freeReg(r1); freeReg(r2);
	    if(opd1 < 0) break;
	    assignReg(opd1,r1);
	    printf("\tadd \t%s,%s\n",tmpRegName[r1],tmpRegName[r2]);
	    break;
	case SUB:
	    r1 = useReg(opd2); r2 = useReg(opd3);
	    freeReg(r1); freeReg(r2);
	    if(opd1 < 0) break;
	    assignReg(opd1,r1);
	    printf("\tsub \t%s,%s\n",tmpRegName[r1],tmpRegName[r2]);
	    break;
	case MUL:
	    r1 = useReg(opd2); r2 = useReg(opd3);
	    freeReg(r1); freeReg(r2);
	    if(opd1 < 0) break;
	    assignReg(opd1,REG_AX);
	    saveReg(REG_DX);
	    if(r1 != REG_AX) 
		printf("\tmov %s,%s\n",tmpRegName[REG_AX],tmpRegName[r1]);
	    printf("\timul \t%s,%s\n",tmpRegName[REG_AX],tmpRegName[r2]);
	    break;
	case LT:
	    r1 = useReg(opd2); r2 = useReg(opd3);
	    freeReg(r1); freeReg(r2);
	    if(opd1 < 0) break;
	    r = getReg(opd1);
	    l1 = label_counter++;
	    l2 = label_counter++;
	    printf("\tcmp\t%s,%s\n",tmpRegName[r1],tmpRegName[r2]);
	    printf("\tjl .L%d\n",l1);
	    printf("\tmov\t%s,0\n",tmpRegName[r]);
	    printf("\tjmp .L%d\n",l2);
	    printf(".L%d:\tmov\t%s,1\n",l1,tmpRegName[r]);
	    printf(".L%d:",l2);
	    break;
	case GT://jason
	    r1 = useReg(opd2); r2 = useReg(opd3);
	    freeReg(r1); freeReg(r2);
	    if(opd1 < 0) break;
	    r = getReg(opd1);
	    l1 = label_counter++;
	    l2 = label_counter++;
	    printf("\tcmp\t%s,%s\n",tmpRegName[r1],tmpRegName[r2]);
	    printf("\tjg .L%d\n",l1);
	    printf("\tmov \t%s,0\n",tmpRegName[r]);
	    printf("\tjmp .L%d\n",l2);
	    printf(".L%d:\tmov\t%s,1\n",l1,tmpRegName[r]);
	    printf(".L%d:",l2);
	    break;

	case PRINTLN:
	    r = useReg(opd1); freeReg(r);
	    printf("\tpush\t%s;把数值参数压栈，第2个参数\n",tmpRegName[r]);//jason
	    printf("\tlea\t%s,[LC%d];取得字符串的地址来传送\n", tmpRegName[r],opd2);
	    printf("\tpush\t%s;把msg消息字符串的地址压栈，第1个参数\n",tmpRegName[r]);
	    saveAllRegs();
	    printf("\tcall\t_println\n");
	    printf("\tadd\tesp,8\n");
	    
	    break;
	}
    }
    
    /* return sequence */
    //printf(";-----zzzz.L%d\n",ret_lab);
    printf(".L%d:\tmov\tebx, [ebp-4]\n",ret_lab);
    printf("\tleave\n");
    printf("\tret\n");
}

int genString(char *s)
{
    int l;
    l = label_counter++;
    
    printf("\tsection\t.data\n");
    printf("LC%d:\n",l);
    printf("\tdb \"%s\"\n",s);
    
    return l;
}

