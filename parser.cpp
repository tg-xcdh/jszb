#include "parser.h"

#include <assert.h>
#include <malloc.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "base.h"
#include "lexer.h"
#include "parser-impl.h"

namespace tg {

#ifdef CONFIG_LOG_PARSER
//#define LOG_CLEAN
#endif

/* ------ AST开始 ------ */

static void nodeFree(Node *node)
{
	assert(node);
	node->clean(node);
	free(node);
}

static void formulaClean(Node *node)
{
	Formula *f = (Formula *)node;
#ifdef LOG_CLEAN
	info("formulaClean\n");
#endif
	for(int i = 0; i < f->stmts.size; ++i) {
		Stmt **arr = (Stmt **)f->stmts.data;
		nodeFree((Node *)arr[i]);
	}
	arrayFree(&f->stmts);
}

static void stmtClean(Node *node)
{
	Stmt *st = (Stmt *)node;
#ifdef LOG_CLEAN
	info("stmtClean\n");
#endif
	stringFree(&st->id);
	if (st->expr) {
		nodeFree((Node *)st->expr);
	}
}

static void intExprClean(Node *node)
{
#ifdef LOG_CLEAN
	info("intExprClean\n");
#endif
}

static void decimalExprClean(Node *node)
{
#ifdef LOG_CLEAN
	info("decimalExprClean\n");
#endif
}

static void idExprClean(Node *node)
{
	IdExpr *e = (IdExpr *)node;
#ifdef LOG_CLEAN
	info("idExprClean\n");
#endif
	stringFree(&e->val);
}

static void exprListClean(Node *node)
{
	ExprList *el = (ExprList *)node;
#ifdef LOG_CLEAN
	info("exprListClean\n");
#endif
	for (int i = 0; i < el->exprs.size; ++i) {
		Expr **arr = (Expr **)el->exprs.data;
		nodeFree((Node *)arr[i]);
	}
	arrayFree(&el->exprs);
}

static void funcCallClean(Node *node)
{
	FuncCall *e = (FuncCall *)node;
#ifdef LOG_CLEAN
	info("funcCallClean\n");
#endif
	stringFree(&e->id);
	nodeFree((Node *)e->args);
}

static void binaryExprClean(Node *node)
{
	BinaryExpr *e = (BinaryExpr *)node;
#ifdef LOG_CLEAN
	info("binaryExprClean\n");
#endif
	if (e->lhs) {
		nodeFree((Node *)e->lhs);
	}
	if (e->rhs) {
		nodeFree((Node *)e->rhs);
	}
}

static Formula *formulaNew(Array *arr)
{
	Formula *fm = (Formula *)malloc(sizeof(*fm));
	if (!fm)
		return 0;
	fm->node.clean = formulaClean;
	fm->stmts = *arr;
#ifndef NDEBUG
	memset(arr, 0, sizeof(*arr));
#endif
	return fm;
}

static Stmt *stmtNew(String *id, enum Token tok, Node *expr)
{
	Stmt *st = (Stmt *)malloc(sizeof(*st));
	if (!st)
		return 0;
	assert(tok == TK_COLON_EQ || tok == TK_COLON);
	assert(expr);
	st->node.clean = stmtClean;
	st->id = *id;
#ifndef NDEBUG
	memset(id, 0, sizeof(*id));
#endif
	st->op = tok;
	st->expr = expr;
	return st;
}

static IntExpr *intExprNew(int64_t val)
{
	IntExpr *e = (IntExpr *)malloc(sizeof(*e));
	if (!e)
		return 0;
	e->node.clean = intExprClean;
	e->val = val;
	return e;
}

static DecimalExpr *decimalExprNew(double val)
{
	DecimalExpr *e = (DecimalExpr *)malloc(sizeof(*e));
	if (!e)
		return 0;
	e->node.clean = decimalExprClean;
	e->val = val;
	return e;
}

static IdExpr *idExprNew(String *val)
{
	IdExpr *e = (IdExpr *)malloc(sizeof(*e));
	if (!e)
		return 0;
	e->node.clean = idExprClean;
	e->val = *val;
#ifndef NDEBUG
	memset(val, 0, sizeof(*val));
#endif
	return e;
}

static ExprList *exprListNew(Array *arr)
{
	ExprList *e = (ExprList *)malloc(sizeof(*e));
	if (!e)
		return 0;
	e->node.clean = exprListClean;
	e->exprs = *arr;
#ifndef NDEBUG
	memset(arr, 0, sizeof(*arr));
#endif
	return e;
}

static FuncCall *funcCallNew(String *id, ExprList *args)
{
	FuncCall *e = (FuncCall *)malloc(sizeof(*e));
	if (!e)
		return 0;
	e->node.clean = funcCallClean;
	e->id = *id;
	e->args = args;
#ifndef NDEBUG
	memset(id, 0, sizeof(*id));
#endif
	return e;
}

static BinaryExpr *binaryExprNew(Node *lhs, enum Token op, Node *rhs)
{
	BinaryExpr *e = (BinaryExpr *)malloc(sizeof(*e));
	if (!e)
		return 0;
	e->node.clean = binaryExprClean;
	e->lhs = lhs;
	e->op = op;
	e->rhs = rhs;
	return e;
}

/* ------ AST结束 ------ */

void *parserNew(void *userdata, int (*handleError)(int lineno, int charpos, int error, const char *errmsg, void *userdata))
{
	Parser *p = (Parser *)malloc(sizeof(*p));
	if (!p)
		return 0;
	p->lex = 0;
	p->tok = TK_NONE;
	p->tokval = 0;
	p->toklen = 0;
	p->userdata = userdata;
	p->handleError = handleError;
	p->errcount = 0;
	p->isquit = false;
	p->ast = 0;
	return p;
}

void parserFree(void *p)
{
	Parser *yacc = (Parser *)p;
	if (!yacc)
		return;
	if (yacc->lex) {
		lexerFree(yacc->lex);
	}
	if (yacc->ast) {
		nodeFree((Node *)yacc->ast);
	}
	free(yacc);
}

static bool handleParserError(Parser *p, int error, const char *errmsg)
{
	++p->errcount;
	if (!p->handleError || (p->handleError)(p->lex->lineno, p->lex->charpos,
											error, errmsg, p->userdata)) {
		p->isquit = true;
		return true;
	}
	return false;
}

static Node *parseExpr(Parser *p);

static ExprList *parseExprList(Parser *p)
{
	bool ok = true;
	Array args;
	
	arrayInit(&args, sizeof(Node *), 4);
	do {
		Node *e = parseExpr(p);
		if (e) {
			Node **p = (Node **)arrayAdd(&args);
			if (p) {
				*p = e;
			} else {
				ok = false;
				break;
			}
		} else {
			ok = false;
			if (handleParserError(p, 1, "解析exprList出错"))
				break;
		}
		if (p->tok != TK_COMMA)
			break;
		p->tok = lexerGetToken(p->lex, &p->tokval, &p->toklen);
	} while (1);
	
	if (ok) {
#ifdef CONFIG_LOG_PARSER
		info("解析得到ExprList\n");
#endif
		return exprListNew(&args);
	}
	return 0;
}

static Node *parseIdOrFuncCall(Parser *p)
{
	Node *expr = 0;
	String id;
	stringInit(&id, p->toklen);
	stringAdd(&id, p->tokval, p->toklen);
	
	p->tok = lexerGetToken(p->lex, &p->tokval, &p->toklen);
	if (p->tok == TK_LP) { // (
		ExprList *arg;
		p->tok = lexerGetToken(p->lex, &p->tokval, &p->toklen);
		arg = parseExprList(p);
		if (p->tok == TK_RP) {
#ifdef CONFIG_LOG_PARSER
			info("解析得到FuncCall\n");
#endif
			expr = (Node *)funcCallNew(&id, arg);
			p->tok = lexerGetToken(p->lex, &p->tokval, &p->toklen);
		} else {
			handleParserError(p, 1, "解析FuncCall,缺少)");
		}
	} else {
#ifdef CONFIG_LOG_PARSER
		info("解析得到IdExpr\n");
#endif
		expr = (Node *)idExprNew(&id);
	}
	return expr;
}

static Node *parseUnaryExpr(Parser *p)
{
	Node *expr = 0;

	if (p->tok == TK_INT) {
		char ch = p->tokval[p->toklen];
		p->tokval[p->toklen] = '\0';
#ifdef CONFIG_LOG_PARSER
		info("解析得到IntExpr\n");
#endif
		expr = (Node *)intExprNew(atoll(p->tokval));
		p->tokval[p->toklen] = ch;
		p->tok = lexerGetToken(p->lex, &p->tokval, &p->toklen);
	} else if (p->tok == TK_DECIMAL) {
		char ch = p->tokval[p->toklen];
		p->tokval[p->toklen] = '\0';
#ifdef CONFIG_LOG_PARSER
		info("解析得到DecimalExpr\n");
#endif
		expr = (Node *)decimalExprNew(atof(p->tokval));
		p->tokval[p->toklen] = ch;
		p->tok = lexerGetToken(p->lex, &p->tokval, &p->toklen);
	} else if (p->tok == TK_ID) { // ID | funcCall
		expr = (Node *)parseIdOrFuncCall(p);
	} else if (p->tok == TK_LP) {
		p->tok = lexerGetToken(p->lex, &p->tokval, &p->toklen);
		expr = parseExpr(p);
		if (p->tok == TK_RP) {
#ifdef CONFIG_LOG_PARSER
			info("解析得到(expr)\n");
#endif
			p->tok = lexerGetToken(p->lex, &p->tokval, &p->toklen);
		} else {
			if (expr) {
				nodeFree(expr);
				expr = 0;
			}
		}
	}
	
	return expr;
}

/* 优先级必须大于0 */
static const int PREC_ADD = 10;
static const int PREC_SUB = 10;
static const int PREC_MUL = 20;
static const int PREC_DIV = 20;

static int getPrec(enum Token tok)
{
	switch (tok) {
	case TK_ADD : return PREC_ADD; break;
	case TK_SUB : return PREC_SUB; break;
	case TK_MUL : return PREC_MUL; break;
	case TK_DIV : return PREC_DIV; break;
	default: break;
	}
	return -1;
}

static Node *parseBinaryExpr(Parser *p, Node **lhs, int prec)
{
	enum Token op;
	Node *rhs;
	int prec2;
	
	op = p->tok;
	p->tok = lexerGetToken(p->lex, &p->tokval, &p->toklen);
	rhs = parseUnaryExpr(p);
	if (!rhs) {
		handleParserError(p, 1, "解析BinaryExpr出错");
		return 0;
	}
	
	prec2 = getPrec(p->tok);
	if (prec2 < 0) {
		Node *e = (Node *)binaryExprNew(*lhs, op, rhs);
		if (e) {
			return e;
		}
	} else if (prec2 <= prec) {
		Node *newlhs = (Node *)binaryExprNew(*lhs, op, rhs);
		if (newlhs) {
			*lhs = newlhs;
			return parseBinaryExpr(p, lhs, prec2);
		}
	} else {
		Node *newrhs = parseBinaryExpr(p, &rhs, prec2);
		if (newrhs) {
			rhs = newrhs;
			Node *e2 = (Node *)binaryExprNew(*lhs, op, rhs);
			if (e2) {
				return e2;
			}
		}
	}
	nodeFree(rhs);
	return 0;
}

static Node *parseExpr(Parser *p)
{
	Node *expr = 0;
	expr = parseUnaryExpr(p);
	if (expr) {
		int prec = getPrec(p->tok);
		if (prec > 0) {
			Node *e2 = parseBinaryExpr(p, &expr, prec);
			if (e2) {
#ifdef CONFIG_LOG_PARSER
				info("解析得到BinaryExpr\n");
#endif
				expr = e2;
			} else {
				nodeFree(expr);
				expr = 0;
			}
		}
	}
	return expr;
}

static Stmt *parseStmt(Parser *p)
{
	String id;
	
	assert(p->tok == TK_ID);
	if (stringInit(&id, p->toklen))
		return 0;
	stringAdd(&id, p->tokval, p->toklen);
	
	p->tok = lexerGetToken(p->lex, &p->tokval, &p->toklen);
	if (p->tok == TK_COLON_EQ || p->tok == TK_COLON) {
		enum Token op = p->tok;
		p->tok = lexerGetToken(p->lex, &p->tokval, &p->toklen);
		Node *expr = parseExpr(p);
		if (expr) {
			if (p->tok == TK_SEMICOLON) {
#ifdef CONFIG_LOG_PARSER
				info("解析得到stmt\n");
#endif
				return stmtNew(&id, op, expr);
			} else {
				handleParserError(p, 0, "解析stmt,缺少;");
			}
		}
	} else {
		handleParserError(p, 1, "解析stmt,不识别符号,只能是:=或者:");
	}
	
	stringFree(&id);
	return 0;
}

static Formula *parseFormula(Parser *p)
{
	Stmt *stmt;
	Array stmts;

	arrayInit(&stmts, sizeof(Stmt *), 8);
	do {
		p->tok = lexerGetToken(p->lex, &p->tokval, &p->toklen);
		if (p->tok == TK_ID) {
			stmt = parseStmt(p);
			if (stmt) {
				Stmt **p = (Stmt **)arrayAdd(&stmts);
				*p = stmt;
			} else if (p->isquit){
				goto end;
			}
		} else if (p->tok == TK_EOF) {
			break;
		} else {
			if (handleParserError(p, 1, "解析formula,不识别符号")) {
				break;
			}
		}
	} while (1);
	
end:
#ifdef CONFIG_LOG_PARSER
	info("解析得到formula\n");
#endif
	return formulaNew(&stmts);
}

/* 自顶向下解析时, 保证进入到每个parseXXX函数先获取了下一个token,
 * parseFormula和parseStmt例外 */

static int parseAST(Parser *p)
{
#ifdef CONFIG_LOG_PARSER
	info("开始解析AST\n");
#endif
	p->ast = parseFormula(p);
	return p->ast ? 0 : -1;
}

int parserParseFile(void *p, const char *filename)
{
	int ret = -1;
	Parser *yacc = (Parser *)p;
	if (!yacc)
		return -1;
	
	assert(yacc->lex == 0); /* 每次解析完成之后释放了 */
	yacc->lex = lexerNew();
	if (!yacc->lex)
		return -1;
	
	if (lexerReadFile(yacc->lex, filename))
		goto end;
	
	if (parseAST(yacc))
		goto end;
	
	ret = 0;
end:
	if (yacc->lex) {
		lexerFree(yacc->lex);
		yacc->lex = 0;
	}
	return ret;
}

int parserParse(void *p, const char *str, int len)
{
	int ret = -1;
	Parser *yacc = (Parser *)p;
	if (!yacc)
		return -1;
	
	assert(yacc->lex == 0); /* 每次解析完成之后释放了 */
	yacc->lex = lexerNew();
	if (!yacc->lex)
		return -1;
	
	if (lexerRead(yacc->lex, str, len))
		goto end;
	
	if (parseAST(yacc))
		goto end;
	
	ret = 0;
end:
	if (yacc->lex) {
		lexerFree(yacc->lex);
		yacc->lex = 0;
	}
	return ret;
}

}
