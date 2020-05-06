// ThreadsCalcs.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#define NVALS 7
#define MAX_VAL 100
#define MIN_VAL 10

#define MIN_COUNT 5
#define MAX_COUNT 10

INT addrIndex, countIndex;


void BiggestDivider(INT* arr, INT count, INT* bds) {
	//const INT SINT = 4;

	for (size_t i = 0; i < count; i++)
	{
		INT curr = arr[i];
		//bd seems to have no sense...
		bds[i] = curr;
	}
}

INT Sum() {

	//get array and his size
	INT* arr = (INT*)TlsGetValue(countIndex);
	INT count = (INT)TlsGetValue(addrIndex);

	INT r = 0;
	for (size_t i = 0; i < count; i++)
	{
		r += arr[i];
	}
	return r;
}

VOID ThreadFunc(VOID) {

	//making generator random
	DWORD currId = GetCurrentThreadId();
	srand((unsigned int)currId);

	//get count
	INT count = rand() % (MAX_COUNT - MIN_COUNT) + MIN_COUNT;

	//make values
	int* vals = new int[count];
	for (size_t j = 0; j < count; j++)
	{
		vals[j] = rand() % (MAX_VAL - MIN_VAL) + MIN_VAL;
	}

	INT* rArray = new INT[count];
	BiggestDivider(vals, count, rArray);

	//save to TLS
	if (!TlsSetValue(countIndex, (LPVOID)rArray)) {
		_tprintf(TEXT("Value (1) has't been set\n"));
	}

	if (!TlsSetValue(addrIndex, (LPVOID)count)) {
		_tprintf(TEXT("Value (2) has't been set\n"));
	}

	_tprintf(TEXT("\nID: %d, Sum: %d\n\n"), currId, Sum());

	delete[] vals;
	delete[] rArray;
}

int _tmain(INT argc, TCHAR** argv)
{
	if (argc != 2) {
		_tprintf(TEXT("Wrong n of args\n"));
		return 0;
	}

	//get count of threads
	INT nThreads = _ttoi(argv[1]);

	HANDLE* threads = new HANDLE[nThreads];

	//allocate tls for all the threads
	addrIndex = TlsAlloc(); //array pointer
	countIndex = TlsAlloc(); // count of elements

	//launch threads with their values
	for (size_t i = 0; i < nThreads; i++)
	{
		DWORD IDThread;
		threads[i] = CreateThread(NULL, // default security attributes 
			0,                           // use default stack size 
			(LPTHREAD_START_ROUTINE)ThreadFunc, // thread function 
			NULL,                    // no thread function argument 
			0,                       // use default creation flags 
			&IDThread);              // returns thread identifier 
	}

	for (size_t i = 0; i < nThreads; i++)
		WaitForSingleObject(threads[i], INFINITE);

	TlsFree(addrIndex);
	delete[] threads;

	return 0;
}