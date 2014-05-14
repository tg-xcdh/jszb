#ifndef TG_INDICATOR_PARSER_H
#define TG_INDICATOR_PARSER_H

namespace tg {

/* 语法规则
 * indicator: formula
 *			;
 * formula 	: // 空 
 *			| formula stmt
 *			;
 * stmt	: varDecl | indDecl				ID [‘:=’‘:’] expr ‘;’
 *		;
 * varDecl : ID ‘:=’ expr ‘;’
 *		   ;
 * indDecl : ID ‘:’ expr ‘;’
 *		   ;
 * expr : INT | DECIMAL | ID | funcCall | binaryExpr | ‘(’ expr ‘)’
 *		;
 * binaryExpr : expr ‘+’ expr 
 *			  | expr ‘-’ expr
 *			  | expr ‘*’ expr
 *			  | expr ‘/’ expr
 *			  ;
 * funcCall : ID ‘(’ funcArgs ‘)’ ;
 * funcArgs : 空 
 *			| exprList
 *			;
 * exprList : expr
 *			| exprList ‘,’ expr
 *			;
 */

/* handleError返回0表示继续, 返回1表示中断 */
void *parserNew(void *userdata, int (*handleError)(int lineno, int charpos, int error, const char *errmsg, void *userdata));
void parserFree(void *p);

int parserParseFile(void *p, const char *filename);
int parserParse(void *p, const char *str, int len);

int parserInterpret(void *p, void *ctx);

double parserGetIndicator(void *p, const char *name);

}

#endif
