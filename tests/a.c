#include <stdio.h>
#include <stdlib.h>
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
typedef struct { // Difficult struct
	int in;
	float* num;
	char* str;
} dif;
// Basic
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
// Strings
int compareStr(char* str1, char* str2)
{
	return strcmp(str1, str2);
}
void testStr(char* str)
{
	printf("%s", str);
}
// Structures
void testStruct(structure val)
{
	printf("in:%d\nfl:%f", val.in, val.fl);
}
int compareStruct(structure val)
{
	return val.in > val.fl;
}
structure retStruct(int in, float fl)
{
	structure r = { .in = in, .fl = fl };
	return r;
}
// More difficult structs
cmx complex(int num)
{
	structure s = { .in = num +1 , .fl = (float)(num + 2) };
	cmx r = { .in = num, .st = s };
	return r;
}
int cmxParam(cmx c)
{
	return c.in + c.st.fl + c.st.in;
}
// Pointers
void triplePtr(int* ptr)
{
	*ptr *= 3;
}
// Pointer storage
int* allocInt(int value)
{
	// Allocates memory
	int* mem = (int*)malloc(sizeof(int));
	*mem = value * 3;
	return mem;
}
void allocIntv2(int* mem)
{
	// Allocates memory
	mem = (int*)malloc(sizeof(int));
	*mem = 15;
}
void freeInt(int* mem)
{
	// Frees memory
	free(mem);
}
// EVEN MORE difficult structs :DD
dif difficult(int num, char* srt)
{
	/* dif d = { .in = num, .num = num * 4.2f, .str = srt }; */
}
