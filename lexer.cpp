#include "lexer.h"

#include <assert.h>
#include <ctype.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include "base.h"

namespace tg {

const char *token2str(enum Token tok)
{
	switch (tok) {
	case TK_ID			: return "ID"; /* 标识符 [a-zA-Z]([a-zA-Z0-9])* */
	case TK_INT			: return "INT"; /* 整数 [0-9]+ */
	case TK_DECIMAL		: return "DECIMAL"; /* 小数 [0-9]*\.[0-9]+ */
	case TK_COLON		: return ":"; /* : */
	case TK_COLON_EQ	: return ":="; /* := */
	case TK_SEMICOLON	: return ";"; /* ; */
	case TK_LP			: return "("; /* ( */
	case TK_RP			: return ")"; /* ) */
	case TK_COMMA		: return ","; /* , */
	case TK_ADD			: return "+"; /* + */
	case TK_SUB			: return "-"; /* - */
	case TK_MUL			: return "*"; /* * */
	case TK_DIV			: return "/"; /* / */
	case TK_EOF			: return "EOF"; /* 表示结束 */
	case TK_ERR			: return "ERR"; /* 表示错误 */
	case TK_NONE		: return "NONE"; /* 表示什么都不是 */
	case TK_ALL			: return "ALL";
	default:break;
	}
	return "未知token";
}

Lexer *lexerNew()
{
	Lexer *l = (Lexer *)malloc(sizeof(*l));
	if (!l)
		return 0;
	l->code = 0;
	l->capacity = 0;
	l->size = 0;
	l->cursor = 0;
	l->lineno = 1;
	l->charpos = 0;
	return l;
}

void lexerFree(Lexer *l)
{
	if (!l)
		return;
	free(l->code);
	free(l);
}

static int lexerExtend(Lexer *l, int len)
{
	if (!l->code || l->size + len > l->capacity) {
		char *mm = (char *)realloc(l->code, l->capacity + len);
		if (!mm)
			return -1;
		l->code = mm;
		l->capacity += len;
	}
	return 0;
}

int lexerReadFile(Lexer *l, const char *filename)
{
	FILE *f;
	unsigned long len;
	int ret = -1;
	
	f = fopen(filename, "rb");
	if (!f)
		goto end;
	fseek(f, 0, SEEK_END);
	len = ftell(f);
	rewind(f);

	if (lexerExtend(l, len+1))
		goto end;
	if (fread(&l->code[l->size], 1, len, f) != len)
		goto end;
	l->size += len;
	l->code[l->size] = '\0';
	ret = 0;

end:
	if (f)
		fclose(f);
	return ret;
}

int lexerRead(Lexer *l, const char *str, int len)
{
	if (lexerExtend(l, len+1))
		return -1;
	memcpy(&l->code[l->size], str, len);
	l->size += len;
	l->code[l->size] = '\0';
	return 0;
}

static enum Token lexerMatch_ID(Lexer *l, int *len)
{
	char ch;

	(*len)++;
	do {
		if (l->cursor == l->size)
			break;
		ch = l->code[l->cursor++];
		if (isalnum(ch)) {
			(*len)++;
		} else {
			l->cursor--;
			break;
		}
	} while (1);

	return TK_ID;
}

static enum Token lexerMatch_INT_DECIMAL(Lexer *l, int *len)
{
	char ch;
	enum Token tok = TK_INT;

	(*len)++;
	do {
		if (l->cursor == l->size)
			break;
		ch = l->code[l->cursor++];
		if (isdigit(ch)) {
			(*len)++;
		} else if (ch == '.' && tok != TK_DECIMAL) {
			tok = TK_DECIMAL;
			(*len)++;
		} else {
			l->cursor--;
			break;
		}
	} while (1);

	return tok;
}

static enum Token parseToken(Lexer *l, char **value, int *len)
{
	char ch;
	
	assert(value && len);
	*value = 0;
	*len = 0;
	assert(l->cursor >= 0 && l->cursor <= l->size);
	if (l->cursor == l->size || !l->code)
		return TK_EOF;

	/* 跳过空白字符 */
	do {
		ch = l->code[l->cursor++];
		if (!isspace(ch))
			break;
		if (l->cursor == l->size)
			return TK_EOF;
	} while (1);

	assert(l->cursor > 0);
	*value = &l->code[l->cursor - 1];
	
	if (isalpha(ch)) {
		return lexerMatch_ID(l, len);
	} else if (isdigit(ch)) {
		return lexerMatch_INT_DECIMAL(l, len);
	} else if (ch == ':') {
		(*len)++;
		if (l->cursor == l->size)
			return TK_COLON;
		if (l->code[l->cursor] == '=') {
			(*len)++;
			l->cursor++;
			return TK_COLON_EQ;
		}
		return TK_COLON;
	} else if (ch == ';') {
		(*len)++;
		return TK_SEMICOLON;
	} else if (ch == '(') {
		(*len)++;
		return TK_LP;
	} else if (ch == ')') {
		(*len)++;
		return TK_RP;
	} else if (ch == ',') {
		(*len)++;
		return TK_COMMA;
	} else if (ch == '+') {
		(*len)++;
		return TK_ADD;
	} else if (ch == '-') {
		(*len)++;
		return TK_SUB;
	} else if (ch == '*') {
		(*len)++;
		return TK_MUL;
	} else if (ch == '/') {
		(*len)++;
		return TK_DIV;
	}
	return TK_ERR;
}

#ifdef CONFIG_LOG_LEXER
static const char *token2str(enum Token tok)
{
	switch (tok) {
	case TK_ID			: return "TK_ID"; break; /* 标识符 [a-zA-Z]([a-zA-Z0-9])* */
	case TK_INT			: return "TK_INT"; break; /* 整数 [0-9]+ */
	case TK_DECIMAL		: return "TK_DECIMAL"; break; /* 小数 [0-9]*\.[0-9]+ */
	case TK_COLON		: return "TK_COLON"; break; /* : */
	case TK_COLON_EQ	: return "TK_COLON_EQ"; break; /* := */
	case TK_SEMICOLON	: return "TK_SEMICOLON"; break; /* ; */
	case TK_LP			: return "TK_LP"; break; /* ( */
	case TK_RP			: return "TK_RP"; break; /* ) */
	case TK_COMMA		: return "TK_COMMA"; break; /* , */
	case TK_ADD			: return "TK_ADD"; break; /* + */
	case TK_SUB			: return "TK_SUB"; break; /* - */
	case TK_MUL			: return "TK_MUL"; break; /* * */
	case TK_DIV			: return "TK_DIV"; break; /* / */
	case TK_EOF			: return "TK_EOF"; break; /* 表示结束 */
	case TK_ERR			: return "TK_ERR"; break; /* 表示错误 */
	case TK_NONE		: return "TK_NONE"; break; /* 表示什么都不是 */
	case TK_ALL			: return "TK_ALL"; break;
	default:break;
	}
	return "未知";
}
#endif

enum Token lexerGetToken(Lexer *l, char **value, int *len)
{
	enum Token tok = parseToken(l, value, len);
#ifdef CONFIG_LOG_LEXER
	{
		char ch;
		if (*len > 0) {
			ch = (*value)[*len];
			(*value)[*len] = '\0';
		}
		debug("token=%s %s %d\n", token2str(tok), *value, *len);
		if (*len > 0) {
			(*value)[*len] = ch;
		}
	}
#endif
	return tok;
}

}
