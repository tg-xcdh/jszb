#include "indicators.h"

#include <assert.h>
#include <malloc.h>

namespace tg {

Value *valueNew()
{
	Value *v = (Value *)malloc(sizeof(*v));
	if (!v)
		return 0;
	v->isOwnMem = false;
	v->i = 0;
	v->f = 0;
	v->values = 0;
	v->size = 0;
	v->index = 0;
	return v;
}

const Value *OPEN(void *data)
{
	Quote *q = (Quote *)data;
	assert(q);
	return &q->open;
}

const Value *HIGH(void *data)
{
	Quote *q = (Quote *)data;
	assert(q);
	return &q->high;
}

const Value *LOW(void *data)
{
	Quote *q = (Quote *)data;
	assert(q);
	return &q->low;
}

const Value *CLOSE(void *data)
{
	Quote *q = (Quote *)data;
	assert(q);
	return &q->close;
}

Value *ADD(const Value *X, const Value *Y, Value *R)
{
	assert(X && Y);
	if (!X || !Y)
		return R;
	if (!R) {
		R = valueNew();
		if (!R)
			return 0;
		
	}
	return R;
}

Value *SUB(const Value *X, const Value *Y, Value *R);
Value *MUL(const Value *X, const Value *Y, Value *R);
Value *DIV(const Value *X, const Value *Y, Value *R);

/* R:=REF(X,N); */
Value *REF(const Value *X, int N, Value *R);

/* R:=MAX(X,M) */
Value *MAX(const Value *X, double M, Value *R);

/* R:=ABS(X,M) */
Value *ABS(const Value *X, double M, Value *R);

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
