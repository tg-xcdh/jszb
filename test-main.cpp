#include <stdio.h>

int main(int argc, const char **argv)
{
	extern void testRSI();
	testRSI();
	extern void testKDJ();
	testKDJ();
	extern void testMACD();
	testMACD();
	
	return 0;
}