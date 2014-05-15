#include "base.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

namespace tg {

static int logLevel = INFO;

//#define CONFIG_LOG_TIME

#ifdef CONFIG_LOG_TIME
static void getLocalTime(struct tm *t)
{
	time_t now = time(0);
	localtime_r(&now, t);
}
#endif

void doLog(int level, const char *filename, int line, const char *fmt, ...)
{
	va_list va;
	const char *prefix = "";
#ifdef CONFIG_LOG_TIME
	struct tm t;
#endif

	if (level < logLevel)
		return;
	switch (level) {
	case INFO:
		prefix = "INFO  ";
		break;
	case WARN:
		prefix = "WARN  ";
		break;
	case ERROR:
		prefix = "ERROR ";
		break;
	case FATAL:
		prefix = "FATAL ";
		break;
	case LDEBUG:
		prefix = "DEBUG ";
		break;
	case RAWLOG:
		prefix = "";
		break;
	default:
		assert(0);
		break;
	}

#ifdef CONFIG_LOG_TIME
	getLocalTime(&t);
#endif
	
	va_start(va, fmt);
#ifdef CONFIG_LOG_TIME
	if (level != RAWLOG) {
		printf("%d-%02d-%02d %02d:%02d:%02d %s",
			t.tm_year+1990, t.tm_mon+1, t.tm_mday,
			t.tm_hour, t.tm_min, t.tm_sec, prefix);
	}
#else
	printf("%s", prefix);
#endif
	vprintf(fmt, va);
	va_end(va);
	
	if (level == FATAL)
		exit(1);
}

/* ------ 字符串开始 ------ */

int stringInit(String *s, int capacity)
{
	s->capacity = 0;
	s->size = 0;
	if (capacity > 0) {
		s->data = (char *)malloc(capacity + 1);
		if (!s->data)
			return -1;
	} else {
		s->data = 0;
	}
	s->capacity = capacity + 1;
	return 0;
}

void stringFree(String *s)
{
	if (!s)
		return;
	free(s->data);
}

int stringAdd(String *s, const char *str, int len)
{
	if (len + s->size + 1 > s->capacity) {
		char *data2 = (char *)realloc(s->data, s->capacity + len + 1);
		if (!data2)
			return -1;
		s->data = data2;
		s->capacity += len + 1;
	}
	memcpy(&s->data[s->size], str, len);
	s->size += len;
	s->data[s->size] = '\0';
	return 0;
}

	
/* ------ 字符串结束 ------ */

/* ------ 数组开始 ------ */

int arrayInit(Array *arr, int objectSize, int capacity)
{
	assert(arr && objectSize > 0 && capacity >= 0);
	arr->objectSize = objectSize;
	arr->capacity = 0;
	arr->size = 0;
	arr->data = malloc(capacity * objectSize);
	if (!arr->data)
		return -1;
	arr->capacity = capacity;
	return 0;
}

void arrayFree(Array *arr)
{
	if (!arr)
		return;
	free(arr->data);
}

void *arrayAdd(Array *arr)
{
	assert(arr);
	if (arr->size == arr->capacity) {
		void *data2 = realloc(arr->data, arr->capacity * 2);
		if (!data2)
			return 0;
		arr->data = data2;
		arr->capacity = arr->capacity * 2;
	}
	char *data = (char *)arr->data;
	data = &data[arr->objectSize * arr->size];
	arr->size++;
	return data;
}

/* ------ 数组结束 ------ */

}

