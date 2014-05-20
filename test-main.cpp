#include <malloc.h>
#include <stdio.h>

#include "base.h"
#include "indicators.h"

using namespace tg;

static Array opens;
static Array highs;
static Array lows;
static Array closes;

Quote *q = 0;
static int startIndex = 0;

static void testInit(int count)
{
	arrayInit(&opens, sizeof(double), 1000);
	arrayInit(&highs, sizeof(double), 1000);
	arrayInit(&lows, sizeof(double), 1000);
	arrayInit(&closes, sizeof(double), 1000);

	const char *filename = "test-data.csv";
	FILE *f = fopen(filename, "r");
	if (f) {
		char buf[1024];
		fgets(buf, sizeof(buf), f);
		while (!feof(f)) {
			buf[0] = '\0';
			fgets(buf, sizeof(buf), f);
			double o, h, l, c;
			o = h = l = c = 0;
			sscanf(buf, "%lf,%lf,%lf,%lf", &o, &h, &l, &c);
			if (o > 0 && h > 0 && l > 0 && c > 0) {
				*(double *)arrayAdd(&opens) = o;
				*(double *)arrayAdd(&highs) = h;
				*(double *)arrayAdd(&lows) = l;
				*(double *)arrayAdd(&closes) = c;
			}
		}
	} else {
		warn("打开文件%s失败\n", filename);
		
		for (int i = 1; i <= 10000; ++i) {
			*(double *)arrayAdd(&opens) = i;
			*(double *)arrayAdd(&highs) = i;
			*(double *)arrayAdd(&lows) = i;
			*(double *)arrayAdd(&closes) = i;
		}
	}

	q = (Quote *)malloc(sizeof(*q));
	if (!q)
		return;
	q->open = valueNew(VT_ARRAY_DOUBLE);
	q->high = valueNew(VT_ARRAY_DOUBLE);
	q->low = valueNew(VT_ARRAY_DOUBLE);
	q->close = valueNew(VT_ARRAY_DOUBLE);
	
	valueExtend(q->open, count);
	valueExtend(q->high, count);
	valueExtend(q->low, count);
	valueExtend(q->close, count);
	
	for (int i = 0; i < count; ++i) {
		valueAdd(q->open, *(double *)arrayGet(&opens, i));
		valueAdd(q->high, *(double *)arrayGet(&highs, i));
		valueAdd(q->low, *(double *)arrayGet(&lows, i));
		valueAdd(q->close, *(double *)arrayGet(&closes, i));
	}
	
	startIndex = count;
}

static void testShutdown()
{
	arrayFree(&opens);
	arrayFree(&highs);
	arrayFree(&lows);
	arrayFree(&closes);
	
	if (q) {
		valueFree(q->open);
		valueFree(q->high);
		valueFree(q->low);
		valueFree(q->close);
		free(q);
	}
}

#define TEST_INIT(name) \
	extern void test##name##Init(); \
	test##name##Init();
#define TEST(name) \
	extern void test##name(); \
	test##name();
#define TEST_SHUTDOWN(name) \
	extern void test##name##Shutdown(); \
	test##name##Shutdown();

int main(int argc, const char **argv)
{
	testInit(100);
	tg::indicatorInit();

	TEST_INIT(RSI);
	TEST_INIT(KDJ);
	TEST_INIT(MACD);

	const int INTERVAL = 1;

	for (int i = startIndex, count = startIndex; i < closes.size; i += INTERVAL, ++count) {
		for (int j = i; j < (i + INTERVAL) && j < closes.size; ++j) {
			if (j == i) { /* 第一个元素模拟股票软件中新增了一根K线 */
				valueAdd(q->open, *(double *)arrayGet(&opens, j));
				valueAdd(q->high, *(double *)arrayGet(&highs, j));
				valueAdd(q->low, *(double *)arrayGet(&lows, j));
				valueAdd(q->close, *(double *)arrayGet(&closes, j));
			} else { /* 其他元素模拟股票软件中当前K线的更新 */
				valueSet(q->open, count, *(double *)arrayGet(&opens, j));
				valueSet(q->high, count, *(double *)arrayGet(&highs, j));
				valueSet(q->low, count, *(double *)arrayGet(&lows, j));
				valueSet(q->close, count, *(double *)arrayGet(&closes, j));
			}
			TEST(RSI);
			TEST(KDJ);
			TEST(MACD);
		}
	}

	TEST_SHUTDOWN(RSI);
	TEST_SHUTDOWN(KDJ);
	TEST_SHUTDOWN(MACD);

	tg::indicatorShutdown();
	testShutdown();
	
	return 0;
}