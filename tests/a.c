#include <stdio.h>
#include <string.h>
// This is the source code for lib.so
// This is used in the shared library tests
// Use the command:
// 	gcc a.c --shared -fPIC -o lib.so
// to compile lib.so
typedef struct {
	int in;
	float fl;
} structure;
typedef struct { // Nested struct
	int in;
	structure st;
} cmx;
int test(int i)
{
	return i * 2;
}
void testVoid()
{
	1 + 2 == 3;
	// Doesn't matter what this does
	// point is that it doesn't return
	// anything.
	// Printing would be smart,
	// but I have yet to manage to
	// capture stdout
}
int compareStr(char* str1, char* str2)
{
	return strcmp(str1, str2);
}
void testStr(char* str)
{
	printf("%s", str);
}
void testStruct(structure val)
{
	printf("in:%d\nfl:%f", val.in, val.fl);
}
int compareStruct(structure val)
{
	return val.in > val.fl;
}
void triplePtr(int* ptr)
{
	*ptr *= 3;
}
structure retStruct(int in, float fl)
{
	structure r = { .in = in, .fl = fl };
	return r;
}
cmx complex(int num)
{
	structure s = { .in = num +1 , .fl = (float)(num + 2) };
	cmx r = { .in = num, .st = s };
	return r;
}
