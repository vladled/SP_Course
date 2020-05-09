// Archiever.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#define MAX_BUF 200
#define STCHAR sizeof(TCHAR)
#define WAITING_TIME 2000

int _tmain(INT argc, TCHAR** argv)
{
	DWORD realC = 1;
	LPCTSTR utilPath = TEXT("7z.exe");
	LPTSTR act = argv[1];
	LPTSTR archName, dirOrFile;

	//making a pipe
	HANDLE meReads, childWrites;
	SECURITY_ATTRIBUTES saAttr = 
		{sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};

	if (!CreatePipe(&meReads, &childWrites, &saAttr, MAX_BUF*STCHAR)) {
		_tprintf(TEXT("Pipe hasn't been created\n"));
		return 0;
	}

	if (!SetHandleInformation(meReads, HANDLE_FLAG_INHERIT, 0)) {
		_tprintf(TEXT("Not inherited handle\n"));
		return 0;
	}

	//creating process
	LPTSTR cmdLine = new TCHAR[MAX_BUF];
	memset(cmdLine, 0, MAX_BUF*STCHAR);

	//making command line
	archName = new TCHAR[MAX_BUF];
	dirOrFile = new TCHAR[MAX_BUF];
	GetFullPathName(argv[2], MAX_BUF, archName, NULL);
	GetFullPathName(argv[3], MAX_BUF, dirOrFile, NULL);

	switch (act[0]) {
	case TEXT('zip'):
		_stprintf(cmdLine, TEXT("%s %s %s %s"),
			utilPath, act, archName, dirOrFile);
		break;

	case TEXT('unzip'):
		_stprintf(cmdLine, TEXT("%s %s %s"),
			utilPath, act, archName);

		//set cwd for extraction
		if(!SetCurrentDirectory(dirOrFile)){
			_tprintf(TEXT("Wrong path"));
			return 0;
		}
		break;

	default :
		_tprintf(TEXT("Wrong command"));
		return 0;
	}

	STARTUPINFO si = {};
	PROCESS_INFORMATION pi = {};

	GetStartupInfo(&si);
	si.cb = sizeof(STARTUPINFO);
	si.hStdOutput = si.hStdError = childWrites;
	si.hStdInput = NULL;
	si.dwFlags = STARTF_USESTDHANDLES;

	if (!CreateProcess(NULL,
		cmdLine,
		NULL, NULL, TRUE, 0, NULL, NULL,
		&si, &pi)) {

		_tprintf(TEXT("Can't create process"));
		return 0;
	}

	////
	//GetCurrentDirectory(MAX_BUF, wbuf);
	//_tprintf(TEXT("\n===Cwd is: %s"), wbuf);
	//delete[] wbuf;
	////

	//get result of process
	
	realC = 1;
	LPSTR buf = new CHAR[MAX_BUF+1];
	memset(buf, 0, MAX_BUF+1);
	BOOL r = TRUE;
	BOOL error = FALSE;
	LPSTR
		needle1 = "WARNING",
		needle2 = "ERROR";

	WaitForSingleObject(pi.hThread, WAITING_TIME);
	//define if operation succeeded
	do {
		r = ReadFile(meReads, (LPVOID)buf, MAX_BUF, &realC, NULL);
		buf[realC] = '\0';

		//error search
		if (!error) {
			error = strstr(buf, needle1) 
				|| strstr(buf, needle2);
		}

		printf("%s", buf);
	} while ((realC | r != 0) && realC == MAX_BUF);

	if (!error) {
		_tprintf(TEXT("\n\n\tEverithing is ok.\n"));
	}
	else {
		_tprintf(TEXT("\n\n\tSorry, error occured.\n"));
	}

	//close process
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
	CloseHandle(childWrites);

    return 0;
}