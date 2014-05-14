#ifndef TG_INDICATOR_PARSER_IMPL_H
#define TG_INDICATOR_PARSER_IMPL_H

namespace tg {

/* ------ AST开始 ------ */

enum NodeType {
	NT_NONE = TK_ALL + 1, 
	NT_FORMULA,
	NT_STMT,
	NT_EXPR,
	NT_INT,
	NT_DECIMAL,
	NT_ID,
	NT_FUNC_CALL,
	NT_BINARY_EXPR,
	NT_ALL
	
};

struct Node {
	void (*clean)(Node *node);
};

struct Stmt;
struct Expr;

struct Formula {
	struct Node node;
	Array stmts;
};

struct Stmt {
	struct Node node;
	String id;
	enum Token op; /* TK_COLON_EQ/TK_COLON */
	Node *expr;
};

struct IntExpr {
	struct Node node;
	int64_t val;
};

struct DecimalExpr {
	struct Node node;
	double val;
};

struct IdExpr {
	struct Node node;
	String val;
};

struct ExprList {
	struct Node node;
	Array exprs;
};

struct FuncCall {
	struct Node node;
	String id;
	ExprList *args;
};

struct BinaryExpr {
	struct Node node;
	Node *lhs;
	enum Token op;
	Node *rhs;
};

/* ------ AST结束 ------ */

class Parser {
public:
	Lexer *lex;
	
	enum Token tok;
	char *tokval; /* 指向当前token的值(由于缓冲区在lex中,没有以字符'\0'结尾) */
	int toklen; /* 当前token的长度 */
	
	void *userdata;
	int (*handleError)(int lineno, int charpos, int error, const char *errmsg, void *);
	int errcount;
	bool isquit;
	
	Formula *ast;
};

}

#endif
