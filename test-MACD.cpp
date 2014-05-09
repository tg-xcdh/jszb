#include <assert.h>
#include <string.h>

#include "base.h"
#include "lexer.h"
#include "parser.h"

#include "test-base.h"

static const char *FOUMULA = ""
	"DIF:EMA(CLOSE,SHORT)-EMA(CLOSE,LONG);\n"
	"DEA:EMA(DIF,MID);\n"
	"MACD:(DIF-DEA)*2;";
	
using namespace tg;

void testMACD()
{
	info("开始单元测试MACD\n");
	info("公式为\n%s\n", FOUMULA);
	
	void *parser = parserNew(0, testHandleError);
	
	parserParse(parser, FOUMULA, strlen(FOUMULA));
	
	parserFree(parser);
	
	info("结束单元测试\n\n");
}
