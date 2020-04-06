#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>

#include "structVSS.h"


int main() {
	/*
	char* buffer;
	FILE* fd;

	{
		if ((buffer = (char*)calloc(1024 * 8 + 1, sizeof(char))) == NULL) {
			printf("Can not allocate memory\n");
		}
		memset(buffer, 52, 8 * 1024);
		buffer[8 * 1024] = '\0';

		fd = fopen("c:\\ChangeFile.c", "ab+");
		if (fd == NULL)
		{
			printf("cant opn file\n");
		}

		fprintf(fd, "%s", buffer);
		fflush(fd);
		fclose(fd);
	}

	system("PAUSE");

	{
		if ((buffer = (char*)calloc(1024 * 8 + 1, sizeof(char))) == NULL) {
			printf("Can not allocate memory\n");
		}
		memset(buffer, 53, 8 * 1024);
		buffer[8 * 1024] = '\0';

		fd = fopen("c:\\ChangeFile.c", "rb+");
		if (fd == NULL) {
			printf("cant opn file\n");
		}

		fprintf(fd, "%s", buffer);
		fflush(fd);
		fclose(fd);
	}

	system("PAUSE");


	return 0;
	*/

	/***********************************************************************/
	
	HANDLE drive = openVolume(L"\\\\.\\c:");
	LONGLONG catalogOffset = getVscCatalogOffset(drive);
	
	printf("Catalog offset: 0x%I64x\n", catalogOffset);
	
	PCATALOG_BLOCK pCatH;

	if (catalogOffset == 0x0) {
		printf("Catalog doesnt exist\n");
		return 0;
	}
	
	//printCatalog(drive, catalogOffset);
	//{07DDF492 - FCC2 - 4A21 - A10C - 8BD193F16ECA}

	GUID guid;
	
	guid.Data1 = 0x07DDF492;
	guid.Data2 = 0xFCC2;
	guid.Data3 = 0x4A21;
	guid.Data4[0] = 0xA1;
	guid.Data4[1] = 0x0C;
	guid.Data4[2] = 0x8B;
	guid.Data4[3] = 0xD1;
	guid.Data4[4] = 0x93;
	guid.Data4[5] = 0xF1;
	guid.Data4[6] = 0x6E;
	guid.Data4[7] = 0xCA;

	PCATALOG_ENTRY entry = getCatalogEntryByVscGuid(drive, catalogOffset, guid);

	//printCatalogEntry(drive, entry, NULL);

	//getStoreInformation(drive, entry->catalog_entry_0x03.store_header_offset);

	printStoreBlockList(drive, entry->catalog_entry_0x03.store_block_list_offset);





	CloseHandle(drive);

	return 0;
}