#include "AST.h"
#include "reg_code.h"

void compileExpression(int target, Object *p)
{
    Object *q;
    Symbol *op;
    int r1,r2;
    extern int tmp_counter;

    switch(p->kind){
    case NUM:
        genCode2(LOADI,target,p->val);
	return;
    case SYM:
	compileLoadVar(target,getSymbol(p));
	return;
    case LIST:
	q = getFirst(p);
	if(q->kind != SYM) compile_error("bad function or operator\n");
	op = q->sym;
	if(op == plusSym){
	    r1 = tmp_counter++; r2 = tmp_counter++;
	    compileExpression(r1,getNth(p,1));
	    compileExpression(r2,getNth(p,2));
	    genCode3(ADD,target,r1,r2);
	    return;
	}
	if(op == minusSym){
	    r1 = tmp_counter++; r2 = tmp_counter++;
	    compileExpression(r1,getNth(p,1));
	    compileExpression(r2,getNth(p,2));
	    genCode3(SUB,target,r1,r2);
	    return;
	}
	if(op == mulSym){
	    r1 = tmp_counter++; r2 = tmp_counter++;
	    compileExpression(r1,getNth(p,1));
	    compileExpression(r2,getNth(p,2));
	    genCode3(MUL,target,r1,r2);
	    return;
	}
	if(op == lessSym){
	    r1 = tmp_counter++; r2 = tmp_counter++;
	    compileExpression(r1,getNth(p,1));
	    compileExpression(r2,getNth(p,2));
	    genCode3(LT,target,r1,r2);
	    return;
	}
	if(op == greaterSym){
	    r1 = tmp_counter++; r2 = tmp_counter++;
	    compileExpression(r1,getNth(p,1));
	    compileExpression(r2,getNth(p,2));
	    genCode3(GT,target,r1,r2);
	    return;
	}
	if(op == eqSym){
	    compile_error("assign operator must be statement\n");
	    return;
	}
	compileCallFunc(target,op,getNext(p));
    }
}


