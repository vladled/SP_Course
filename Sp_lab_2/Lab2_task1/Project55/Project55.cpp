#include <iostream>
#include "windows.h"
#include <vector>
#include <stdio.h>

#define CREATE '1'
#define MODIFY '2'
#define DELETE1 '3'
#define PRINT '4'
#define EXIT '5'

CONST WCHAR FILE_PATH[] = L"ContextRecords.rb";

typedef struct
{
	WORD IdRecord;
	FILETIME TimeOfCreate;
	CHAR RecordText[81];
	WORD RecordCounterChanges;
} RECORD, *P_RECORD;

typedef struct
{
	WORD NotNullRecordsCount;
	WORD FileForRecordsSize;
} HEADER, *P_HEADER;

void PrintMenu();
bool PrintFileContent(HANDLE hRecordsFile, DWORD dwCountRecords);
bool ProgrammInitData(HANDLE* hRecordsFile, DWORD* fileSize, DWORD* countRecords);
bool CreateAndWriteRecord(HANDLE hRecordsFile, DWORD* fileSize, CONST CHAR* recordText, DWORD* dwRecordId);
int GetRecordsCount(DWORD fileSize);
bool WriteInFile(HANDLE hRecordsFile, RECORD usRec, DWORD offset);
bool WriteOrGetHeader(HANDLE hRecordsFile, DWORD dwCreateOrGetHeader, P_HEADER pHeader, BOOL clearFile);
bool DeleteRecord(HANDLE hRecordsFile, DWORD idToDelete, DWORD* countRecords, DWORD* fileSize);
bool ModifyRecord(HANDLE hRecordsFile, DWORD idToModify);
void createChoise(HANDLE hfile, DWORD dwSize, DWORD dwCount);
bool modifyChoise(HANDLE hfile, DWORD dwCount);
bool deleteChoise(HANDLE hfile, DWORD dwSize, DWORD dwCount);

using namespace std;

int main(int argc, char* argv[])
{
	char Choise = '0';
	DWORD dwCountOfRecords = 0;
	DWORD dwSizeOfFile = 0;
	HANDLE hFileRecords = NULL;
	if (!ProgrammInitData(&hFileRecords, &dwSizeOfFile, &dwCountOfRecords))
	{
		cout << "Cant initialize file with records\n";
		return 1;
	}

	while (1)
	{
		PrintMenu();
		cin >> Choise;
		switch (Choise)
		{
		case CREATE:
		{
			createChoise(hFileRecords, dwSizeOfFile, dwCountOfRecords);
		} break;

		case MODIFY:
		{
			bool result = modifyChoise(hFileRecords, dwCountOfRecords);
			if (!result)
			{
				continue;
			}
		} break;

		case DELETE1:
		{
			bool result = deleteChoise(hFileRecords, dwSizeOfFile, dwCountOfRecords);
			if (!result)
			{
				continue;
			}
		} break;

		case PRINT:
		{
			if (!PrintFileContent(hFileRecords, dwCountOfRecords))
			{
				cout << "Error! Can't print header and records!\n";
				continue;
			}
		} break;

		case EXIT:
		{
			CloseHandle(hFileRecords);
			return 0;
		} break;

		default: cout << "Wrong choice! Try again." << endl;
			continue;
		}
	}
}

bool WriteOrGetHeader(HANDLE hRecordsFile, DWORD dwCreateOrGetHeader, P_HEADER pHeader, BOOL clearFile)
{
	DWORD cBytes = 0;
	if (SetFilePointer(hRecordsFile, 0, 0, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		return false;
	}
	if (dwCreateOrGetHeader == 1)
	{
		if ((WriteFile(hRecordsFile, pHeader, sizeof(HEADER), &cBytes, NULL) == false) ||
			(cBytes != sizeof(HEADER)))
		{
			return false;
		}
		if (clearFile)
		{
			if (SetEndOfFile(hRecordsFile) == false)
			{
				return false;
			}
		}
	}
	else if (dwCreateOrGetHeader == 2)
	{
		if ((ReadFile(hRecordsFile, pHeader, sizeof(HEADER), &cBytes, NULL) == false) ||
			(cBytes != sizeof(HEADER)))
		{
			return false;
		}
	}
	else
	{
		return false;
	}
	return true;
}

bool ModifyRecord(HANDLE hRecordsFile, DWORD idToModify)
{
	HEADER header;
	CHAR inBuff[81] = { 0 };

	if (!WriteOrGetHeader(hRecordsFile, 2, &header, FALSE))
	{
		return false;
	}

	cout << "Input text to change current record text or '0' to reset current record\n";
	cin >> inBuff;
	inBuff[80] = '\0';

	RECORD recBuff = { 0 };
	DWORD cBytes = 0;
	BOOL isNullRecord = false;
	if (SetFilePointer(hRecordsFile, sizeof(HEADER) + sizeof(RECORD) * idToModify,
		NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		return false;
	}
	if ((ReadFile(hRecordsFile, &recBuff, sizeof(RECORD), &cBytes, NULL) == false) ||
		(cBytes != sizeof(RECORD)))
	{
		return false;
	}
	isNullRecord = recBuff.RecordText[0] == '\0' ? true : false;

	if (strcmp(inBuff, "0") == 0)
	{
		memset(recBuff.RecordText, '\0', sizeof(recBuff.RecordText));
	}
	else
	{
		strcpy_s(recBuff.RecordText, 80, inBuff);
		recBuff.RecordText[80] = '\0';
	}
	recBuff.RecordCounterChanges++;

	if (!WriteInFile(hRecordsFile, recBuff, (sizeof(HEADER) + sizeof(RECORD) * idToModify)))
	{
		return false;
	}

	if (isNullRecord && (strcmp(inBuff, "0") != 0))
	{
		header.NotNullRecordsCount++;
	}
	if (!isNullRecord && (strcmp(inBuff, "0") == 0))
	{
		header.NotNullRecordsCount--;
	}

	if (!WriteOrGetHeader(hRecordsFile, 1, &header, FALSE))
	{
		return false;
	}
	return true;
}

bool DeleteRecord(HANDLE hRecordsFile, DWORD idToDelete, DWORD* countRecords, DWORD* fileSize)
{
	HEADER header;
	if (!WriteOrGetHeader(hRecordsFile, 2, &header, FALSE))
	{
		return false;
	}

	if (SetFilePointer(hRecordsFile, (sizeof(HEADER) + ((idToDelete) * sizeof(RECORD))),
		NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		return false;
	}

	RECORD delRec;
	if (ReadFile(hRecordsFile, &delRec, sizeof(delRec), 0, NULL) == false)
	{
		return false;
	}

	WORD countElemToEndFromDeletedEl = (*countRecords) - idToDelete - 1;
	RECORD* recordsBuff = (RECORD*)calloc(countElemToEndFromDeletedEl, sizeof(RECORD));
	DWORD cBytes;

	if ((ReadFile(hRecordsFile, recordsBuff, sizeof(RECORD) * countElemToEndFromDeletedEl, &cBytes, NULL) == false) ||
		cBytes != sizeof(RECORD) * countElemToEndFromDeletedEl)
	{
		return false;
	}

	for (WORD i = 0; i < countElemToEndFromDeletedEl; i++)
	{
		recordsBuff[i].IdRecord = i + idToDelete;
	}

	if (SetFilePointer(hRecordsFile, 0 - (sizeof(RECORD) * countElemToEndFromDeletedEl + sizeof(RECORD)),
		NULL, FILE_CURRENT) == INVALID_SET_FILE_POINTER)
	{
		return false;
	}

	if ((WriteFile(hRecordsFile, recordsBuff, sizeof(RECORD) * countElemToEndFromDeletedEl, &cBytes, NULL) == false) ||
		(cBytes != sizeof(RECORD) * countElemToEndFromDeletedEl))
	{
		return false;
	}
	free(recordsBuff);
	SetEndOfFile(hRecordsFile);

	header.NotNullRecordsCount = delRec.RecordText[0] == '\0' ?
		header.NotNullRecordsCount : header.NotNullRecordsCount - 1;
	header.FileForRecordsSize = header.FileForRecordsSize - sizeof(RECORD);
	(*countRecords)--;
	(*fileSize) -= sizeof(RECORD);

	if (!WriteOrGetHeader(hRecordsFile, 1, &header, FALSE))
	{
		return false;
	}
	return true;
}

bool CreateAndWriteRecord(HANDLE hRecordsFile, DWORD * fileSize, CONST CHAR* recText, DWORD * dwRecordId)
{
	BOOL isNullRecord = false;
	RECORD usRec;
	if (strcmp(recText, "0") == 0)
	{
		memset(usRec.RecordText, '\0', sizeof(usRec.RecordText));
		isNullRecord = true;
	}
	else
	{
		strcpy_s(usRec.RecordText, 80, recText);
		usRec.RecordText[80] = '\0';
		isNullRecord = false;
	}
	usRec.RecordCounterChanges = 0;
	usRec.IdRecord = *dwRecordId;

	SYSTEMTIME sysTime = { 0 };
	FILETIME fileTime = { 0 };
	GetLocalTime(&sysTime);
	SystemTimeToFileTime(&sysTime, &fileTime);
	usRec.TimeOfCreate = fileTime;

	if (!WriteInFile(hRecordsFile, usRec, sizeof(HEADER) + (*dwRecordId) * sizeof(RECORD)))
	{
		return false;
	}

	HEADER header = { 0 };
	WriteOrGetHeader(hRecordsFile, 2, &header, FALSE);
	header.FileForRecordsSize += sizeof(RECORD);
	header.NotNullRecordsCount = isNullRecord ? header.NotNullRecordsCount : ++header.NotNullRecordsCount;
	WriteOrGetHeader(hRecordsFile, 1, &header, FALSE);

	(*dwRecordId)++;
	(*fileSize) += sizeof(RECORD);

	return true;
}

bool WriteInFile(HANDLE hRecordsFile, RECORD usRec, DWORD offset)
{
	if (SetFilePointer(hRecordsFile, offset, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		return false;
	}

	DWORD countWritenBytes = 0;
	if ((WriteFile(hRecordsFile, &usRec, sizeof(usRec), &countWritenBytes, NULL) == false) ||
		(countWritenBytes != sizeof(usRec)))
	{
		return false;
	}
	return true;
}

void PrintMenu()
{
	cout << "\t MENU" << endl;
	cout << " 1 - Create record in file" << endl;
	cout << " 2 - Modify record in file" << endl;
	cout << " 3 - Delete record in file" << endl;
	cout << " 4 - Print header and records from file" << endl;
	cout << " 5 - Exit" << endl;
}

int GetRecordsCount(DWORD fileSize)
{
	WORD recordsCount = (fileSize - sizeof(HEADER)) / sizeof(RECORD);
	return recordsCount;
}

bool ProgrammInitData(HANDLE* hRecordsFile, DWORD* fileSize, DWORD* countRecords)
{
	*hRecordsFile = CreateFile(FILE_PATH,
		GENERIC_ALL,
		FILE_SHARE_READ,
		NULL,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (*hRecordsFile == INVALID_HANDLE_VALUE)
	{
		return false;
	}
	*fileSize = GetFileSize(*hRecordsFile, NULL);
	if (*fileSize == INVALID_FILE_SIZE)
	{
		return false;
	}

	if (*fileSize <= sizeof(HEADER))
	{
		HEADER header;
		header.NotNullRecordsCount = 0;
		header.FileForRecordsSize = sizeof(HEADER);
		if (!WriteOrGetHeader(*hRecordsFile, 1, &header, TRUE))
		{
			return false;
		}
		*fileSize = sizeof(HEADER);
	}

	*countRecords = GetRecordsCount(*fileSize);
	return true;
}

bool PrintFileContent(HANDLE hRecordsFile, DWORD dwCountRecords)
{
	HEADER header = { 0 };
	RECORD recordsBuff = { 0 };
	DWORD cReadedBytes = 0;
	if (!WriteOrGetHeader(hRecordsFile, 2, &header, FALSE))
	{
		return false;
	}
	cout << "HEADER\n";
	cout << "* Not null records count:" << header.NotNullRecordsCount << endl;
	cout << "* File size:" << header.FileForRecordsSize << " Bytes" << endl;

	while ((ReadFile(hRecordsFile, &recordsBuff, sizeof(RECORD), &cReadedBytes, NULL) != false) &&
		(cReadedBytes == sizeof(RECORD)))
	{
		SYSTEMTIME sysTime;
		FileTimeToSystemTime(&recordsBuff.TimeOfCreate, &sysTime);
		cout << "RECORD\n";
		cout << "*Record ID:" << recordsBuff.IdRecord << endl;
		printf("* Time of create: %d:%d:%d, %d:%d\n", sysTime.wDay, sysTime.wMonth, sysTime.wYear, sysTime.wHour, sysTime.wMinute);
		cout << "* Count of changes:" << recordsBuff.RecordCounterChanges << endl;
		cout << "* Record Text:" << recordsBuff.RecordText << endl;
	}
	return true;
}

void createChoise(HANDLE hfile, DWORD dwSize, DWORD dwCount)
{
	CHAR inBuff[80] = { 0 };
	cout << "Input text for new record (max length is 80 characters)\n or input '0' to create record with null content\n";
	cin >> inBuff;
	if (!CreateAndWriteRecord(hfile, &dwSize, inBuff, &dwCount))
	{
		cout << "Error! Can't create new record!" << endl;
	}
	else
	{
		cout << "Created successfully!" << endl;
	}
}

bool modifyChoise(HANDLE hfile, DWORD dwCount)
{
	DWORD idToMod;
	cout << "Input record id for modify:";
	cin >> idToMod;
	if (idToMod >= dwCount)
	{
		cout << "Error! Record with this id is not exist!\n";
		return false;
	}
	else
	{
		if (!ModifyRecord(hfile, idToMod))
		{
			cout << "Error!. Can't modify this record!";
			return false;
		}
		else
		{
			cout << "Modified successfully!" << endl;
		}
	}
	return true;
}

bool deleteChoise(HANDLE hfile, DWORD dwSize, DWORD dwCount)
{
	DWORD idToDelete;
	cout << "Input record id for deleting:";
	cin >> idToDelete;
	if (idToDelete >= dwCount)
	{
		cout << "Error! Record with this id is not exist!\n";
		return false;
	}
	else
	{
		if (!DeleteRecord(hfile, idToDelete, &dwCount, &dwSize))
		{
			cout << "Error!. Can't delete this record!\n";
		}
		else
		{
			cout << "Deleted successfully!" << endl;
		}
		return false;
	}
	return true;
}