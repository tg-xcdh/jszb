#ifndef TG_INDICATOR_BASE_H
#define TG_INDICATOR_BASE_H

#ifdef WIN32
//#undef fseek
//#define fseek _fseeki64
//#undef ftell
//#define ftell _ftelli64

#define atoll _atoi64
#define strtoull _strtoi64
#define isnan _isnan

#else
/* */
#endif

namespace tg {

/* 日志宏 */
enum Level {
	INFO,
	WARN,
	ERROR,
	FATAL,
	LDEBUG,
	RAWLOG, /* 不带任何附件信息的 */
};

#ifdef CONFIG_LOG_DEBUG
#define debug(...) doLog(LDEBUG, __FILE__, __LINE__, __VA_ARGS__)
#else
#define debug(...) do {} while (0)
#endif
#define info(...) doLog(INFO, __FILE__, __LINE__, __VA_ARGS__)
#define warn(...) doLog(WARN, __FILE__, __LINE__, __VA_ARGS__)
#define error(...) doLog(ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define fatal(...) doLog(FATAL, __FILE__, __LINE__, __VA_ARGS__)
#define rawlog(...) doLog(RAWLOG, __FILE__, __LINE__, __VA_ARGS__)

void doLog(int level, const char *filename, int line, const char *fmt, ...);

/* ------ 字符串开始 ------ */

typedef struct {
	int capacity;
	int size;
	char *data;
} String;

int stringInit(String *s, int capacity);
void stringFree(String *s);

int stringAdd(String *s, const char *str, int len);

/* ------ 字符串结束 ------ */

/* ------ 数组开始 ------ */

typedef struct {
	int objectSize;
	int capacity;
	int size;
	void *data;
} Array;

int arrayInit(Array *arr, int objectSize, int capacity);
void arrayFree(Array *arr);

void *arrayAdd(Array *arr);

void *arrayGet(Array *arr, int i);

/* ------ 数组结束 ------ */

/* ------ 哈希表开始 ------ */

/* char *作为关键字 */
int cstrCmp(const void *key1, const void *key2);
unsigned int cstrHash(const void *key);
/* int作为关键字 */
int intCmp(const void *key1, const void *key2);
unsigned int intHash(const void *key);

typedef struct HashNode {
	const void *key;
	void *value;
	struct HashNode *next;
} HashNode;
/* <void *, void *> */
typedef struct {
	int size; /* 大小 */
	int (*cmp)(const void *, const void *);/* 1:>,0==,-1:< */
	unsigned int (*hash)(const void *);
	HashNode *freeNode; /* 在删除元素时，空出来的HashNode */
	HashNode *data; /* 存放所有的数据 */
	int dataCapacity; /* data的容量 */
	HashNode **lookups; /* 表示那些元素被占用了，那些空闲中 */
} HashTable;

int hashTableInit(HashTable *ht, int size, int (*cmp)(const void *, const void *), unsigned int (*hash)(const void *));
void hashTableFree(HashTable *ht);

int hashTableInsert(HashTable *ht, const void *key, void *value, void **oldvalue);
int hashTableFind(HashTable *ht, const void *key, void **value);
int hashTableRemove(HashTable *ht, const void *key, void **value);

/* ------ 哈希表结束 ------ */

} // namespace tg

#endif
