#include "test-base.h"

#include "base.h"

namespace tg {

int testHandleError(int lineno, int charpos, int error, const char *errmsg, void *userdata)
{
	if (error) {
		error("³ö´í %d:%d %s\n", lineno, charpos, errmsg);
	} else {
		info("³ö´í %d:%d %s\n", lineno, charpos, errmsg);
	}
	return 1;
}

}
