#ifndef TG_INDICAOTR_INDICATORS_H
#define TG_INDICAOTR_INDICATORS_H

#include <stdint.h>

namespace tg {

static const char TYPE_INT = 1;
static const char TYPE_DOUBLE = 2;
static const char TYPE_ARRAY = 3;

struct Value {
	bool isOwnMem;
	char type;
	union {
		int64_t i;
		double f;
		double *values;
	};
	int size; /* 大小 */
	int capacity; /* 容量 */
	/* values数组中最后一个元素的编号,即最新元素的编号
	 * 编号从1开始，使用编号来标识元素，原因在：例如size=5000，no可以到10000 */
	int no;
};

int isValueValid(double f); /* 值是否有效 */

struct Quote {
	Value *open;
	Value *high;
	Value *low;
	Value *close;
};

/* 注册变量，例如OPEN,CLOSE等;
 * 注册函数，例如MA，SMA等 */
typedef Value *(*ValueFn)(void *parser, int argc, const Value **args, Value *R);
int registerVariable(const char *name, ValueFn fn);
ValueFn findVariable(const char *name);

int registerFunction(const char *name, ValueFn fn);
ValueFn findFunction(const char *name);


const Value *OPEN(void *parser);
const Value *HIGH(void *parser);
const Value *LOW(void *parser);
const Value *CLOSE(void *parser);

/* 问题:生成新Value的时候,内存分配多大???
 * 例如对于
 * 	RSI
		LC:=REF(CLOSE,1);
		RSI1:SMA(MAX(CLOSE-LC,0),N1,1)/SMA(ABS(CLOSE-LC),N1,1)*100;
 * 如何使得内存分配最小??? 
 */

/* 算术运算:加减乘除 */
Value *ADD(const Value *X, const Value *Y, Value *R);
Value *SUB(const Value *X, const Value *Y, Value *R);
Value *MUL(const Value *X, const Value *Y, Value *R);
Value *DIV(const Value *X, const Value *Y, Value *R);

/* R:=REF(X,N); */
Value *REF(const Value *X, int N, Value *R);

/* R:=MAX(X,M) */
Value *MAX(const Value *X, double M, Value *R);

/* R:=ABS(X) */
Value *ABS(const Value *X, Value *R);

/* R:=HHV(X, N) */
Value *HHV(const Value *X, int N, Value *R);

/* R:=LLV(X, N) */
Value *LLV(const Value *X, int N, Value *R);

/* MA
	返回简单移动平均
	用法：MA(X,M)：X的M日简单移动平均 */
Value *MA(const Value *X, int M, Value *R);

/* EMA
	返回指数移动平均
	用法：EMA(X,M)：X的M日指数移动平均
	算法：Y = (X*2 + Y'*(M-1)) / (M+1) */
Value *EMA(const Value *X, int M, Value *R);
		
/* SMA
	返回平滑移动平均
	用法：SMA(X,N,M)：X的N日移动平均，M为权重
	算法：Y = (X*M + Y'*(N-M)) / N */
Value *SMA(const Value *X, int N, int M, Value *R);

}

#endif