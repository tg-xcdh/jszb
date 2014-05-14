#include <assert.h>
#include <string.h>

#include "base.h"
#include "lexer.h"
#include "parser.h"

#include "test-base.h"

static const char *FOUMULA = ""
		"RSV:=(CLOSE-LLV(LOW,N))/(HHV(HIGH,N)-LLV(LOW,N))*100;\n"
		"K:SMA(RSV,M1,1);\n"
		"D:SMA(K,M2,1);\n"
		"J:3*K-2*D;";

using namespace tg;

void testKDJ()
{
	info("开始单元测试KDJ\n");
	info("公式为\n%s\n", FOUMULA);
	
	void *parser = parserNew(0, testHandleError);
	
	parserParse(parser, FOUMULA, strlen(FOUMULA));
	
	parserFree(parser);
	
	info("结束单元测试\n\n");
}
