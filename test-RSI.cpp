#include <assert.h>
#include <string.h>

#include "base.h"
#include "lexer.h"
#include "parser.h"

#include "test-base.h"

static const char *FOUMULA = ""
	"N1:=6;\n"
	"N2:=12;\n"
	"N3:=24;\n"
	"LC:=REF(CLOSE,1);\n"
	"RSI1:SMA(MAX(CLOSE-LC,0),N1,1)/SMA(ABS(CLOSE-LC),N1,1)*100;\n"
	"RSI2:SMA(MAX(CLOSE-LC,0),N2,1)/SMA(ABS(CLOSE-LC),N2,1)*100;\n"
	"RSI3:SMA(MAX(CLOSE-LC,0),N3,1)/SMA(ABS(CLOSE-LC),N3,1)*100;";

using namespace tg;
namespace tg { struct Quote ; }

extern tg::Quote *q;

static void *parser = 0;

void testRSIInit()
{
	info("开始单元测试RSI\n");
	info("公式为\n%s\n", FOUMULA);

	assert(!parser);
	parser = parserNew(0, testHandleError);
	parserParse(parser, FOUMULA, strlen(FOUMULA));
}

void testRSI()
{
	if (!parserInterp(parser, q)) {
		info("RSI解释运行成功\n");
		double rsi;
		if (!parserGetIndicator(parser, "RSI1", &rsi)) {
			info("RSI1=%f\n", rsi);
		}
		if (!parserGetIndicator(parser, "RSI2", &rsi)) {
			info("RSI2=%f\n", rsi);
		}
		if (!parserGetIndicator(parser, "RSI3", &rsi)) {
			info("RSI3=%f\n", rsi);
		}
	} else {
		warn("RSI解释运行失败\n");
	}
}

void testRSIShutdown()
{
	parserFree(parser);
	parser = 0;
	info("结束单元测试RSI\n\n");
}
