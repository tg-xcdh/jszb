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
			sscanf(buf, "%lf %lf %lf %lf", &o, &h, &l, &c);
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
	q->open = valueNew();
	q->high = valueNew();
	q->low = valueNew();
	q->close = valueNew();
	
	valueExtend(q->open, count);
	valueExtend(q->high, count);
	valueExtend(q->low, count);
	valueExtend(q->close, count);
	
	for (int i = 0; i < count; ++i) {
		valueSet(q->open, i, *(double *)arrayGet(&opens, i));
		valueSet(q->high, i, *(double *)arrayGet(&highs, i));
		valueSet(q->low, i, *(double *)arrayGet(&lows, i));
		valueSet(q->close, i, *(double *)arrayGet(&closes, i));
		q->open->size++;
		q->high->size++;
		q->low->size++;
		q->close->size++;
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

int main(int argc, const char **argv)
{
	testInit(100);
	tg::indicatorInit();

	extern void testRSI();
	testRSI();
	extern void testKDJ();
	testKDJ();
	extern void testMACD();
	testMACD();
	
	tg::indicatorShutdown();
	
	testShutdown();
	
	return 0;
}