#include <assert.h>
#include <string.h>

#include "base.h"
#include "lexer.h"
#include "parser.h"

#include "test-base.h"

static const char *FOUMULA = ""
	"LC:=REF(CLOSE,1);\n"
	"RSI1:SMA(MAX(CLOSE-LC,0),N1,1)/SMA(ABS(CLOSE-LC),N1,1)*100;\n"
	"RSI2:SMA(MAX(CLOSE-LC,0),N2,1)/SMA(ABS(CLOSE-LC),N2,1)*100;\n"
	"RSI3:SMA(MAX(CLOSE-LC,0),N3,1)/SMA(ABS(CLOSE-LC),N3,1)*100;";

using namespace tg;

void testRSI()
{
	info("开始单元测试RSI\n");
	info("公式为\n%s\n", FOUMULA);
	
	void *parser = parserNew(0, testHandleError);
	
	parserParse(parser, FOUMULA, strlen(FOUMULA));
	if (!parserInterp(parser, 0)) {
		info("解释运行成功\n");
	} else {
		warn("解释运行失败\n");
	}
	
	parserFree(parser);
	
	info("结束单元测试RSI\n\n");
}