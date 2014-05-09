#ifndef TG_INDICATOR_LEXER_H
#define TG_INDICATOR_LEXER_H

namespace tg {

enum Token {
	TK_ID, /* 标识符 [a-zA-Z]([a-zA-Z0-9])* */
	TK_INT, /* 整数 [0-9]+ */
	TK_DECIMAL, /* 小数 [0-9]*\.[0-9]+ */
	TK_COLON, /* : */
	TK_COLON_EQ, /* := */
	TK_SEMICOLON, /* ; */
	TK_LP, /* ( */
	TK_RP, /* ) */
	TK_COMMA, /* , */
	TK_ADD, /* + */
	TK_SUB, /* - */
	TK_MUL, /* * */
	TK_DIV, /* / */
	TK_EOF, /* 表示结束 */
	TK_ERR, /* 表示错误 */
	TK_NONE, /* 表示什么都不是 */
	TK_ALL
};

struct Lexer {
	char *code; /* 全部载入内存的代码 */
	int capacity;
	int size;
	int cursor;
	int lineno; /* 行号 */
	int charpos; /* 一行之内的字符位置 */
};

typedef struct Lexer Lexer;

Lexer *lexerNew();
void lexerFree(Lexer *l);

int lexerReadFile(Lexer *l, const char *filename);
int lexerRead(Lexer *l, const char *str, int len);

enum Token lexerGetToken(Lexer *l, char **value, int *len);

}

#endif