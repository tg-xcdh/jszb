#include "parser.h"

#include <assert.h>
#include <float.h>
#include <malloc.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "base.h"
#include "indicators.h"
#include "lexer.h"
#include "parser-impl.h"

namespace tg {

#ifdef CONFIG_LOG_PARSER
//#define LOG_PARSE
//#define LOG_CLEAN
#define LOG_INTERP
#endif

/* 使用notepad++,在windows和linux下统一使用ANSI编码格式 */

/* ------ AST开始 ------ */

static void nodeFree(Node *node)
{
	assert(node);
	node->clean(node);
	free(node);
}

/* ------ clean开始 ------ */

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

/* ------ clean结束 ------ */

/* ------ interp开始 ------ */

#ifdef LOG_INTERP
static void logInterpPrefix(void *parser)
{
	for (int i = 0; i < ((Parser *)parser)->interpDepth; ++i) {
		rawlog("\t");
	}
}
#endif

static Value *parserFindVariable(Parser *p, const char *name)
{
	assert(p);
	for (int i = 0; i < p->ast->stmts.size; ++i) {
		Stmt **arr = (Stmt **)p->ast->stmts.data;
		Stmt *st = arr[i];
		if (strcmp(name, st->id.data) == 0) {
			return st->value;
		}
	}
	return 0;
}

static Value *formulaInterp(Node *node, void *parser)
{
#ifndef NDEBUG
	assert(node->type == NT_FORMULA);
#endif
	Formula *e = (Formula *)node;
#ifdef LOG_INTERP
	rawlog("formulaInterp\n");
	((Parser *)parser)->interpDepth++;
#endif
	for (int i = 0; i < e->stmts.size; ++i) {
		Stmt **arr = (Stmt **)e->stmts.data;
		Node *nd = (Node *)arr[i];
		nd->interp(nd, parser);
	}
#ifdef LOG_INTERP
	((Parser *)parser)->interpDepth--;
#endif
	return 0;
}

static Value *stmtInterp(Node *node, void *parser)
{
#ifndef NDEBUG
	assert(node->type == NT_STMT);
#endif
	Stmt *e = (Stmt *)node;
	assert(e->op == TK_COLON_EQ || e->op == TK_COLON);
#ifdef LOG_INTERP
	logInterpPrefix(parser);
	rawlog("stmtInterp\n");
	((Parser *)parser)->interpDepth++;
	logInterpPrefix(parser);
	rawlog("%s\n", e->id.data);
	logInterpPrefix(parser);
	rawlog("%s\n", token2str(e->op));
#endif
	assert(e->expr);
	Value *v = e->expr->interp(e->expr, parser);
	assert(e->value == 0 || e->value == v);
	e->value = v;
#ifdef LOG_INTERP
	((Parser *)parser)->interpDepth--;
#endif
	return 0;
}

static Value *intExprInterp(Node *node, void *parser)
{
#ifndef NDEBUG
	assert(node->type == NT_INT_EXPR);
#endif
	IntExpr *e = (IntExpr *)node;
#ifdef LOG_INTERP
	logInterpPrefix(parser);
	rawlog("intExprInterp %lld\n", e->value->i);
#endif
	return e->value;
}

static Value *decimalExprInterp(Node *node, void *parser)
{
#ifndef NDEBUG
	assert(node->type == NT_DECIMAL_EXPR);
#endif
	DecimalExpr *e = (DecimalExpr *)node;
#ifdef LOG_INTERP
	logInterpPrefix(parser);
	rawlog("decimalExprInterp %f\n", e->value->f);
#endif
	return e->value;
}

static Value *idExprInterp(Node *node, void *parser)
{
#ifndef NDEBUG
	assert(node->type == NT_ID_EXPR);
#endif
	IdExpr *e = (IdExpr *)node;
#ifdef LOG_INTERP
	logInterpPrefix(parser);
	rawlog("idExprInterp %s\n", e->val.data);
#endif
	ValueFn fn;
	/* 搜索内置的变量 */
	fn = findVariable(e->val.data);
	if (fn)
		return fn(parser, 0, 0, 0);
	
	/* 搜索公式定义中的变量 */
	return parserFindVariable((Parser *)parser, e->val.data);
}

static Value *exprListInterp(Node *node, void *parser)
{
#ifndef NDEBUG
	assert(node->type == NT_EXPR_LIST);
#endif
	ExprList *e = (ExprList *)node;
#ifdef LOG_INTERP
	logInterpPrefix(parser);
	rawlog("exprListInterp\n");
	((Parser *)parser)->interpDepth++;
#endif
	for (int i = 0; i < e->exprs.size; ++i) {
		Expr **arr = (Expr **)e->exprs.data;
		Node *nd = (Node *)arr[i];
		e->values[i] = nd->interp(nd, parser);
	}
#ifdef LOG_INTERP
	((Parser *)parser)->interpDepth--;
#endif
	return 0;
}

static Value *funcCallInterp(Node *node, void *parser)
{
#ifndef NDEBUG
	assert(node->type == NT_FUNC_CALL);
#endif
	FuncCall *e = (FuncCall *)node;
#ifdef LOG_INTERP
	logInterpPrefix(parser);
	rawlog("funcCallInterp\n");
	((Parser *)parser)->interpDepth++;
	logInterpPrefix(parser);
	rawlog("%s\n", e->id.data);
#endif
	int argc = 0;
	Value **args = 0;
	if (e->args) {
		e->args->node.interp((Node *)e->args, parser);
		argc = e->args->exprs.size;
		args = e->args->values;
	}
	ValueFn fn = findFunction(e->id.data);
	if (fn) {
		e->value = fn(parser, argc, (const Value **)args, e->value);
	}
#ifdef LOG_INTERP
	((Parser *)parser)->interpDepth--;
#endif
	return e->value;
}

static Value *binaryExprInterp(Node *node, void *parser)
{
#ifndef NDEBUG
	assert(node->type == NT_BINARY_EXPR);
#endif
	BinaryExpr *e = (BinaryExpr *)node;
#ifdef LOG_INTERP
	logInterpPrefix(parser);
	rawlog("binaryExprInterp\n");
	((Parser *)parser)->interpDepth++;
#endif
	assert(e->lhs && e->rhs);
	Value *lhs = e->lhs->interp(e->lhs, parser);
	Value *rhs = e->rhs->interp(e->rhs, parser);
	switch (e->op) {
	case TK_ADD: e->value = ADD(lhs, rhs, e->value); break; /* + */
	case TK_SUB: e->value = SUB(lhs, rhs, e->value); break; /* - */
	case TK_MUL: e->value = MUL(lhs, rhs, e->value); break; /* * */
	case TK_DIV: e->value = DIV(lhs, rhs, e->value); break; /* / */
	default:
		assert(0);
		break;
	}
#ifdef LOG_INTERP
	logInterpPrefix(parser);
	rawlog("%s\n", token2str(e->op));
	((Parser *)parser)->interpDepth--;
#endif
	return e->value;
}

/* ------ interp结束 ------ */

/* ------ new开始 ------ */

static Formula *formulaNew(Array *arr)
{
	Formula *fm = (Formula *)malloc(sizeof(*fm));
	if (!fm)
		return 0;
#ifndef NDEBUG
	fm->node.type = NT_FORMULA;
#endif
	fm->node.clean = formulaClean;
	fm->node.interp = formulaInterp;
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
#ifndef NDEBUG
	st->node.type = NT_STMT;
#endif
	st->value = 0;
	st->node.clean = stmtClean;
	st->node.interp = stmtInterp;
	st->id = *id;
#ifndef NDEBUG
	memset(id, 0, sizeof(*id));
#endif
	st->op = tok;
	st->expr = expr;
	return st;
}

static IntExpr *intExprNew(int val)
{
	IntExpr *e = (IntExpr *)malloc(sizeof(*e));
	if (!e)
		return 0;
#ifndef NDEBUG
	e->node.type = NT_INT_EXPR;
#endif
	e->value = valueNew(VT_INT);
	if (!e->value) {
		free(e);
		return 0;
	}
	e->value->i = val;
	e->node.clean = intExprClean;
	e->node.interp = intExprInterp;
	return e;
}

static DecimalExpr *decimalExprNew(double val)
{
	DecimalExpr *e = (DecimalExpr *)malloc(sizeof(*e));
	if (!e)
		return 0;
#ifndef NDEBUG
	e->node.type = NT_DECIMAL_EXPR;
#endif
	e->value = valueNew(VT_DOUBLE);
	if (!e->value) {
		free(e);
		return 0;
	}
	e->value->f = val;
	e->node.clean = decimalExprClean;
	e->node.interp = decimalExprInterp;
	return e;
}

static IdExpr *idExprNew(String *val)
{
	IdExpr *e = (IdExpr *)malloc(sizeof(*e));
	if (!e)
		return 0;
#ifndef NDEBUG
	e->node.type = NT_ID_EXPR;
#endif
	e->value = 0;
	e->node.clean = idExprClean;
	e->node.interp = idExprInterp;
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
#ifndef NDEBUG
	e->node.type = NT_EXPR_LIST;
#endif
	e->node.clean = exprListClean;
	e->node.interp = exprListInterp;
	e->exprs = *arr;
	if (e->exprs.size > 0) {
		e->values = (Value **)malloc(sizeof(Value *) * e->exprs.size);
		memset(e->values, 0, sizeof(Value *)*e->exprs.size);
	} else {
		e->values = 0;
	}
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
#ifndef NDEBUG
	e->node.type = NT_FUNC_CALL;
#endif
	e->node.clean = funcCallClean;
	e->node.interp = funcCallInterp;
	e->id = *id;
	e->args = args;
	e->value = 0;
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
#ifndef NDEBUG
	e->node.type = NT_BINARY_EXPR;
#endif
	e->node.clean = binaryExprClean;
	e->node.interp = binaryExprInterp;
	e->lhs = lhs;
	e->op = op;
	e->rhs = rhs;
	e->value = 0;
	return e;
}

/* ------ new结束 ------ */

/* ------ AST结束 ------ */

void *parserNew(void *errdata, int (*handleError)(int lineno, int charpos, int error, const char *errmsg, void *errdata))
{
	Parser *p = (Parser *)malloc(sizeof(*p));
	if (!p)
		return 0;
	p->lex = 0;
	p->tok = TK_NONE;
	p->tokval = 0;
	p->toklen = 0;
	p->errdata = errdata;
	p->handleError = handleError;
	p->errcount = 0;
	p->isquit = false;
	p->ast = 0;
	p->userdata = 0;
#ifdef CONFIG_LOG_PARSER
	p->interpDepth = 0;
#endif
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
											error, errmsg, p->errdata)) {
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
#ifdef LOG_PARSE
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
#ifdef LOG_PARSE
			info("解析得到FuncCall\n");
#endif
			expr = (Node *)funcCallNew(&id, arg);
			p->tok = lexerGetToken(p->lex, &p->tokval, &p->toklen);
		} else {
			handleParserError(p, 1, "解析FuncCall,缺少)");
		}
	} else {
#ifdef LOG_PARSE
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
#ifdef LOG_PARSE
		info("解析得到IntExpr\n");
#endif
		expr = (Node *)intExprNew(atoi(p->tokval));
		p->tokval[p->toklen] = ch;
		p->tok = lexerGetToken(p->lex, &p->tokval, &p->toklen);
	} else if (p->tok == TK_DECIMAL) {
		char ch = p->tokval[p->toklen];
		p->tokval[p->toklen] = '\0';
#ifdef LOG_PARSE
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
#ifdef LOG_PARSE
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
#ifdef LOG_PARSE
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
#ifdef LOG_PARSE
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
#ifdef LOG_PARSE
	info("解析得到formula\n");
#endif
	return formulaNew(&stmts);
}

/* 自顶向下解析时, 保证进入到每个parseXXX函数先获取了下一个token,
 * parseFormula和parseStmt例外 */

static int parseAST(Parser *p)
{
#ifdef LOG_PARSE
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

int parserInterp(void *p, void *userdata)
{
	Parser *yacc = (Parser *)p;
	if (!yacc || !yacc->ast)
		return -1;
	assert(yacc->userdata == 0 || yacc->userdata == userdata);
	yacc->userdata = userdata;
	yacc->ast->node.interp((Node *)yacc->ast, p);
	
	return 0;
}

int parserGetIndicator(void *p, const char *name, double *outf)
{
	int ret = -1;
	double f = -DBL_MAX;
	Value *v = parserFindVariable((Parser *)p, name);
	if (v) {
		ret = 0;
		if (v->type == VT_ARRAY_DOUBLE) {
			f = valueGet(v, v->size-1);
		} else if (v->type == VT_INT) {
			f = v->i;
		} else if (v->type == VT_DOUBLE) {
			f = v->f;
		} else {
			assert(0);
		}
	}
	if (outf)
		*outf = f;
	return ret;
}

}
