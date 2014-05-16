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

void *arrayGet(Array *arr, int i)
{
	char *data = (char *)arr->data;
	return &data[arr->objectSize * i];
}

/* ------ 数组结束 ------ */

/* ------ 哈希表开始 ------ */

int cstrCmp(const void *key1, const void *key2)
{
	const char *s1 = (const char *)key1;
	const char *s2 = (const char *)key2;
	if (s1 == s2)
		return 0;
	if (!s1)
		return -1;
	if (!s2)
		return 1;
	return strcmp(s1, s2);
}

/* https://www.byvoid.com/blog/string-hash-compare */
// BKDR Hash Function
static unsigned int BKDRHash(const char *str)
{
	unsigned int seed = 131; // 31 131 1313 13131 131313 etc..
	unsigned int hash = 0;
	while (*str) {
		hash = hash * seed + (*str++);
	}
	return (hash & 0x7FFFFFFF);
}

unsigned int cstrHash(const void *key)
{
	const char *s = (const char *)key;
	assert(s);
	return BKDRHash(s);
}

int intCmp(const void *key1, const void *key2)
{
	int i1 = (int)key1;
	int i2 = (int)key2;
	if (i1 < i2)
		return -1;
	else if (i2 > i2)
		return 1;
	return 0;
}

unsigned int intHash(const void *key)
{
	return (int)key;
}

int hashTableInit(HashTable *ht, int size, int (*cmp)(const void *, const void *), unsigned int (*hash)(const void *))
{
	assert(ht && size > 0 && cmp && hash);
	ht->size = 0;
	ht->cmp = cmp;
	ht->hash = hash;
	/* 把data和lookups一起分配 */
	ht->data = (HashNode *)malloc(size * sizeof(HashNode) + size * sizeof(HashNode *));
	if (!ht->data)
		return -1;
	ht->dataCapacity = size;
	ht->lookups = (HashNode **)&ht->data[size];
	memset(ht->lookups, 0, size * sizeof(HashNode *));
	ht->freeNode = &ht->data[0];
	for (int i = 1; i < ht->dataCapacity; ++i) {
		ht->data[i-1].next = &ht->data[i];
	}
	ht->data[ht->dataCapacity-1].next = 0;
	return 0;
}

void hashTableFree(HashTable *ht)
{
	if (ht) {
		free(ht->data);
#ifndef NDEBUG
		ht->data = 0;
		ht->lookups = 0;
#endif
	}
}

static unsigned int hashTableHash(HashTable *ht, const void *key)
{
	unsigned int h;
	int hashsize = ht->dataCapacity;
	h = ht->hash(key);
	h = h % hashsize; // 维持2次幂???
	return h;
}

static void hashTableInsertWithHash(HashTable *ht, unsigned int hash, const void *key, void *value)
{
	HashNode *next;
	assert(ht->freeNode && ht->size < ht->dataCapacity);
	ht->freeNode->key = key;
	ht->freeNode->value = value;
	next = ht->freeNode;
	ht->freeNode->next = ht->lookups[hash];
	ht->lookups[hash] = ht->freeNode;
	ht->freeNode = next;
	ht->size++;
}

int hashTableInsert(HashTable *ht, const void *key, void *value, void **oldvalue)
{
	unsigned int h;
	HashNode *ph;
	assert(ht);
	if (oldvalue)
		*oldvalue = 0;
	int hashsize = ht->dataCapacity;
	h = ht->hash(key);
	h = h % hashsize; // 维持2次幂???
	ph = ht->lookups[h];
	
	while (ph) { /* 同一hash的元素已经存在过 */
		if (!ht->cmp(ph->key, key)) { /* 覆盖原有元素 */
			if (oldvalue)
				*oldvalue = ph->value;
			ph->value = value;
			return 0;
		}
		ph = ph->next;
	}
	
	if (ht->size >= ht->dataCapacity) { /* 需要自动扩展大小，问题:是否可以保持用于hash的hashsize不变??? */
		HashTable ht2;
		if (hashTableInit(&ht2, ht->size*2, ht->cmp, ht->hash)) {
			return -1;
		}
		for (int i = 0; i < ht->dataCapacity; ++i) {
			HashNode *p = ht->lookups[i];
			if (!p)
				continue;
			while (p) {
				unsigned int h2 = hashTableHash(&ht2, p->key);
				hashTableInsertWithHash(&ht2, h2, p->key, p->value);
				p = p->next;
			}
		}
		hashTableFree(ht);
		*ht = ht2;
	}
	h = h % ht->dataCapacity;
	assert(h >= 0 && (int)h < ht->dataCapacity);
	hashTableInsertWithHash(ht, h, key, value);
	return 0;
}

int hashTableFind(HashTable *ht, const void *key, void **value)
{
	unsigned int h;
	HashNode *ph;
	if (value)
		*value = 0;
	if (!ht)
		return -1;
	h = hashTableHash(ht, key);
	assert(h >= 0 && (int)h < ht->dataCapacity);
	ph = ht->lookups[h];
	
	while (ph) { /* 同一hash的元素已经存在过 */
		if (!ht->cmp(ph->key, key)) {
			if (value)
				*value = ph->value;
			ph->value = value;
			return 0;
		}
		ph = ph->next;
	}
	
	return -1;
}

int hashTableRemove(HashTable *ht, const void *key, void **value)
{
	unsigned int h;
	HashNode *ph;
	HashNode **ppnext;
	if (value)
		*value = 0;
	if (!ht)
		return -1;
	h = hashTableHash(ht, key);
	assert(h >= 0 && (int)h < ht->dataCapacity);
	ph = ht->lookups[h];
	ppnext = &ht->lookups[h];
	
	while (ph) { /* 同一hash的元素已经存在过 */
		if (!ht->cmp(ph->key, key)) {
			if (value)
				*value = ph->value;
			*ppnext = ph->next;
			ph->next = ht->freeNode;
			ht->freeNode = ph;
			return 0;
		}
		ppnext = &(ph->next);
		ph = ph->next;
	}
	return -1;
}

/* ------ 哈希表结束 ------ */

}

