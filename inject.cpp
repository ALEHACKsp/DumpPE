#include <stdio.h>
#include <windows.h>

/*
	TESTS:
		Windows 10 x64
*/

/*
	BUILD:
		g++ dumpPE.cpp -o dumpPE.exe
		g++ dumpPE.cpp -o dumpPE.exe -m32
*/

/*
	TODO:
		1) Check for the process archeticture
*/

int main(int argc, char **argv) {
	
	if (argc > 3)
	{
		DWORD dwPid = atoi(argv[1]);

		LPVOID lpAddress = NULL;
#if defined(_M_X64) || defined(__amd64__)
		sscanf(argv[2], "%llx", &lpAddress);
#else
		sscanf(argv[2], "%lx", &lpAddress);
#endif

		LPCSTR szOutPe = argv[3];

		HANDLE hProcess = NULL;
		if (!(hProcess = OpenProcess(
			PROCESS_VM_READ,
			FALSE,
			dwPid
		)))
		{
			printf("Error at OpenProcess, code = %d\n", GetLastError());
			return 0;
		};

		SIZE_T stReadBytes;
		CHAR bDosHeader[sizeof(IMAGE_DOS_HEADER)] = { 0 };
		if (!ReadProcessMemory(
			hProcess,
			lpAddress,
			bDosHeader,
			sizeof(bDosHeader),
			&stReadBytes
		) || (stReadBytes != sizeof(bDosHeader)))
		{
			printf("Error at ReadProcessMemory, code = %d\n", GetLastError());
			return FALSE;
		};
		PIMAGE_DOS_HEADER lpDosHeader = (PIMAGE_DOS_HEADER)bDosHeader;

		CHAR bNtHeader[sizeof(IMAGE_NT_HEADERS)] = { 0 };
		if (!ReadProcessMemory(
			hProcess,
#if defined(_M_X64) || defined(__amd64__)
			(LPVOID)((ULONGLONG)lpAddress + lpDosHeader->e_lfanew),
#else
			(LPVOID)((ULONG)lpAddress + lpDosHeader->e_lfanew),
#endif
			bNtHeader,
			sizeof(bNtHeader),
			&stReadBytes
		) || (stReadBytes != sizeof(bNtHeader)))
		{
			printf("Error at ReadProcessMemory, code = %d, %x\n", GetLastError(), lpDosHeader->e_lfanew);
			return FALSE;
		};
		PIMAGE_NT_HEADERS lpNtHeader = (PIMAGE_NT_HEADERS)bNtHeader;

		LPVOID lpMappedImage = NULL;
		if (!(lpMappedImage = VirtualAlloc(
			NULL,
			lpNtHeader->OptionalHeader.SizeOfImage,
			(MEM_COMMIT | MEM_RESERVE),
			PAGE_READWRITE
		)))
		{
			printf("Error at VirtualAlloc, code = %d\n", GetLastError());
			return FALSE;
		};

		if (!ReadProcessMemory(
			hProcess,
			lpAddress,
			lpMappedImage,
			lpNtHeader->OptionalHeader.SizeOfImage,
			&stReadBytes
		) || (stReadBytes != lpNtHeader->OptionalHeader.SizeOfImage))
		{
			printf("Error at ReadProcessMemory, code = %d\n", GetLastError());
			return FALSE;
		};

		if (!DeleteFileA(szOutPe))
		{
			if (ERROR_FILE_NOT_FOUND != GetLastError()) {
				printf("Error at DeleteFileA, code = %d\n", GetLastError());
				return 0;
			}
		}

		HANDLE hFile;
		if (!(hFile = CreateFileA(
			szOutPe,
			FILE_APPEND_DATA,
			0,
			NULL,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL
		)) || INVALID_HANDLE_VALUE == hFile)
		{
			printf("Error at CreateFileA, code = %d\n", GetLastError());
			return FALSE;
		};

		DWORD dwWrittenBytes;
		if (!WriteFile(
			hFile,
			lpMappedImage,
			lpNtHeader->OptionalHeader.SizeOfHeaders,
			&dwWrittenBytes,
			NULL
		) || (lpNtHeader->OptionalHeader.SizeOfHeaders != dwWrittenBytes))
		{
			printf("Error at WriteFile, code = %d\n", GetLastError());
			return 0;
		};

		IMAGE_SECTION_HEADER* lpSectionHeaderArray = (IMAGE_SECTION_HEADER*)((ULONG_PTR)lpMappedImage + lpDosHeader->e_lfanew + sizeof(IMAGE_NT_HEADERS));

		for (DWORD dwSecIndex = 0; dwSecIndex < lpNtHeader->FileHeader.NumberOfSections; dwSecIndex++)
		{
			if (!WriteFile(
				hFile,
#if defined(_M_X64) || defined(__amd64__)
				(LPVOID)((ULONGLONG)lpMappedImage + lpSectionHeaderArray[dwSecIndex].VirtualAddress),
#else
				(LPVOID)((ULONG)lpMappedImage + lpSectionHeaderArray[dwSecIndex].VirtualAddress),
#endif
				lpSectionHeaderArray[dwSecIndex].SizeOfRawData,
				&dwWrittenBytes,
				NULL
			) || (lpSectionHeaderArray[dwSecIndex].SizeOfRawData != dwWrittenBytes))
			{
				printf("Error at WriteFile, code = %d\n", GetLastError());
				return 0;
			};
		};

		CloseHandle(hFile);

		puts("Done !!!");

	}
	else
	{
		printf("%s [pid] [hex_adress] [out_pe]\n", argv[0]);
		return 0;
	}
}
