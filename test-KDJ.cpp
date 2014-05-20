#include <assert.h>
#include <string.h>

#include "base.h"
#include "lexer.h"
#include "parser.h"

#include "test-base.h"

static const char *FOUMULA = ""
	"N:=3;\n"
	"M1:=9;\n"
	"M2:=3;\n"
	"RSV:=(CLOSE-LLV(LOW,N))/(HHV(HIGH,N)-LLV(LOW,N))*100;\n"
	"K:SMA(RSV,M1,1);\n"
	"D:SMA(K,M2,1);\n"
	"J:3*K-2*D;";

using namespace tg;
namespace tg { struct Quote ; }

extern tg::Quote *q;

static void *parser = 0;

void testKDJInit()
{
	info("开始单元测试KDJ\n");
	info("公式为\n%s\n", FOUMULA);

	parser = parserNew(0, testHandleError);
	parserParse(parser, FOUMULA, strlen(FOUMULA));
}

void testKDJ()
{
	if (!parserInterp(parser, q)) {
		info("KDJ解释运行成功\n");
		double f;
		if (!parserGetIndicator(parser, "K", &f)) {
			info("K=%f\n", f);
		}
		if (!parserGetIndicator(parser, "D", &f)) {
			info("D=%f\n", f);
		}
		if (!parserGetIndicator(parser, "J", &f)) {
			info("J=%f\n", f);
		}
	} else {
		warn("KDJ解释运行失败\n");
	}
}

void testKDJShutdown()
{
	parserFree(parser);
	info("结束单元测试KDJ\n\n");
}
