#include "structVSS.h"

#define DEBUG 

#ifdef DEBUG
#define DEBUG_PRINT(fmt, args)			\
	fprintf(stderr, "DEBUG: %s:%d:%s(): " fmt, __FILE__, __LINE__, __func__, ##args)
#else
#define DEBUG_PRINT(fmt, args)
#endif

void printf_guid(GUID guid) {
	printf("{%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX}\n",
		guid.Data1, guid.Data2, guid.Data3,
		guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
		guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
}

void setFilePointer(HANDLE drive, LONGLONG offset) {
	LARGE_INTEGER off;
	off.QuadPart = offset;

	HRESULT result = SetFilePointerEx(drive, off, NULL, FILE_BEGIN);

	if (!result) {
		DEBUG_PRINT("SetFilePointerEx: %lu\n", GetLastError());
	}
}

HANDLE openVolume(LPCWSTR volumePath) {
	HANDLE drive = CreateFile(volumePath,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING,
		NULL);

	if (drive == INVALID_HANDLE_VALUE) {
		DEBUG_PRINT("CreateFile(openDevice): %lu\n", GetLastError());
		return 0;
	}

	setFilePointer(drive, 0);

	return drive;
}

PVOLUME_HEADER getVscHeader(HANDLE drive) {
	BOOL result;
	DWORD numOfBytesRead = 1;

	setFilePointer(drive, VSC_HEADER_OFFSET);

	void* buffer = calloc(MINIMAL_VSC_DATA_SIZE, sizeof(char));
	if (!buffer) {
		DEBUG_PRINT("Can't allocate memory: %lu\n", GetLastError());
		return NULL;
	}

	result = ReadFile(drive, buffer, MINIMAL_VSC_DATA_SIZE, &numOfBytesRead, NULL);

	if (!result || numOfBytesRead != MINIMAL_VSC_DATA_SIZE) {
		DEBUG_PRINT("Can't read volume: %lu\n", GetLastError());
		return NULL;
	}

	PVOLUME_HEADER header = (PVOLUME_HEADER)buffer;

	if (header->record_type != VSC_HEADER) {
		DEBUG_PRINT("Record type != VSC_HEADER\n", 1);
		return NULL;
	}

#if defined( HAVE_DEBUG_OUTPUT )
	printf("================================Volume Header================================\n");
	printf("VSS identifier: ");
	printf_guid(header->vss_identifier);
	printf("Version: 0x%x\n", header->version);
	printf("Record type: 0x%x\n", header->record_type);
	printf("Current offset: 0x%I64x\n", header->current_offset);
	printf("Unknown (Next offset?): 0x%I64x\n", header->unknown_1);
	printf("Catalog offset: 0x%I64x\n", header->catalog_offset);
	printf("Maximum size: 0x%I64x\n", header->maximum_size);
	printf("Volume identifier: ");
	printf_guid(header->volume_identifier);
	printf("Shadow copy storage volume identifier: ");
	printf_guid(header->shadow_copy_storage_volume_identifier);
	printf("=============================================================================\n");
#endif

	return header;
}

uint64_t getVscCatalogOffset(HANDLE drive) {
	uint64_t catalogOffset = 0;

	PVOLUME_HEADER pVolumeHeader = getVscHeader(drive);
	catalogOffset = pVolumeHeader->catalog_offset;

	free(pVolumeHeader);

	return catalogOffset;
}

PCATALOG_ENTRY catalogEntryIterator(HANDLE drive, uint64_t offset, PCATALOG_ENTRY (*func)(HANDLE drive, PCATALOG_ENTRY entry, void* buffer), void* buf) {
	BOOL result;
	DWORD numOfBytesRead = 1;
	PCATALOG_BLOCK pCatalogBlock;
	PCATALOG_BLOCK_HEADER header;

	setFilePointer(drive, offset);

	void* buffer = calloc(VSC_DATA_SIZE, sizeof(char));
	if (!buffer) {
		DEBUG_PRINT("Can't allocate memory\n", 1);
		return NULL;
	}

	do {
		result = ReadFile(drive, buffer, VSC_DATA_SIZE, &numOfBytesRead, NULL);

		if (!result) {
			DEBUG_PRINT("Can't read volume\n", 1);
			return NULL;
		}

		pCatalogBlock = (PCATALOG_BLOCK)buffer;
		header = &(pCatalogBlock->header);

		if (pCatalogBlock->header.record_type != VSC_CATALOG_BLOCK_HEADER) {
			DEBUG_PRINT("Record type != VSC_CATALOG_BLOCK_HEADER\n", 1);
			return NULL;
		}

#if defined (HAVE_DEBUG_OUTPUT)
		printf("==========================Catalog block header===============================\n");

		printf("VSS identifier: ");
		printf_guid(header->vss_identifier);
		printf("Version: 0x%I64x\n", header->version);
		printf("Record type: 0x%I64x\n", header->record_type);
		printf("Relative (catalog block) offset: 0x%I64x\n", header->relative_offset);
		printf("Current (catalog block) offset: 0x%I64x\n", header->current_offset);
		printf("Next (catalog block) offset: 0x%I64x\n", header->next_offset);
		
		if(header->next_offset == 0)
			printf("=============================================================================\n");
#endif
		uint64_t version;

		for (int i = 0; i < QT_CATALOGS_ENTRY; i++) {	
			if (func) {
				PCATALOG_ENTRY catalog_entry = func(drive, &(pCatalogBlock->catalog_entry[i]), buf);
				if (catalog_entry) {
					free(buffer);
					return catalog_entry;
				}
			}
		}
		setFilePointer(drive, header->next_offset);
	} while (header->next_offset != 0);

	free(buffer);
	return NULL;
}

PCATALOG_ENTRY printCatalogEntry(HANDLE drive, PCATALOG_ENTRY entry, void* buf) {
	uint64_t version;
	version = entry->version;
	if (version == UNUSED_CATALOG_ENTRY) {
		PCATALOG_ENTRY_0x01 catalog_entry = &(entry->catalog_entry_0x01);
		printf("\nUnused catalog entry (type 0x01)\n");
	}
	else if (version == CATALOG_ENTRY_2) {
		PCATALOG_ENTRY_0x02 catalog_entry = &(entry->catalog_entry_0x02);
		printf("\nCatalog entry version: 0x%I64x\n", version);
		printf("\tVolume size %lu\n", catalog_entry->volume_size);
		printf("\tStore identifier: ");
		printf_guid(catalog_entry->store_identifier);
		printf("\tUnknown (Sequence number): 0x%I64x\n", catalog_entry->unknown_1);
		printf("\tUnknown (Flags?): 0x%I64x\n", catalog_entry->unknown_2);
	}
	else if (version == CATALOG_ENTRY_3) {
		PCATALOG_ENTRY_0x03 catalog_entry = &(entry->catalog_entry_0x03);
		printf("Catalog entry version: 0x%I64x\n", version);
		printf("\tStore block list offset: 0x%I64x\n", catalog_entry->store_block_list_offset);
		printf("\tStore identifier: ");
		printf_guid(catalog_entry->store_identifier);
		printf("\tStore header offset: 0x%I64x\n", catalog_entry->store_header_offset);
		printf("\tStore block range list offset: 0x%I64x\n", catalog_entry->store_block_range_list_offset);
		printf("\tStore (current) bitmap offset: 0x%I64x\n", catalog_entry->store_current_bitmap_offset);
		printf("\tNTFS (metadata) file reference: 0x%I64x\n", catalog_entry->ntfs_file_ref);
		printf("\tStore previous bitmap offset: 0x%I64x\n", catalog_entry->store_previous_bitmap_offset);
		printf("\tUnknown: 0x%I64x\n", catalog_entry->unknown_1);
	}
	return NULL;
}

void printCatalog(HANDLE drive, uint64_t offset) {
	catalogEntryIterator(drive, offset, printCatalogEntry, NULL);
}

PCATALOG_ENTRY compareByGuid(HANDLE drive, PCATALOG_ENTRY entry, void* buf) {
	GUID guid = *(GUID*)buf;

	if (entry->version != CATALOG_ENTRY_3) return NULL;

	PSTORE_BLOCK_INFORMATION pInfo = getStoreInformation(drive, entry->catalog_entry_0x03.store_header_offset);

	if (!memcmp(&guid, &(pInfo->shadow_copy_identifier), sizeof(GUID))) {
		PCATALOG_ENTRY catalog_entry = (PCATALOG_ENTRY)calloc(1, sizeof(CATALOG_ENTRY));
		memcpy(catalog_entry, entry, sizeof(CATALOG_ENTRY));
		return catalog_entry;
	}
	return NULL;
}

PCATALOG_ENTRY getCatalogEntryByVscGuid(HANDLE drive, uint64_t offset, GUID guid) {
	return catalogEntryIterator(drive, offset, compareByGuid, &guid);
}

PSTORE_BLOCK_INFORMATION getStoreInformation(HANDLE drive, uint64_t offset) {
	PSTORE_INFORMATION pStoreInfo;
	PSTORE_BLOCK_HEADER pHeader;
	PSTORE_BLOCK_INFORMATION pInfo;
	BOOL result;
	DWORD numOfBytesRead = 1;

	setFilePointer(drive, offset);

	void* buffer = calloc(VSC_DATA_SIZE, sizeof(char));
	if (!buffer) {
		DEBUG_PRINT("Can't allocate memory\n", 1);
		return NULL;
	}

	result = ReadFile(drive, buffer, VSC_DATA_SIZE, &numOfBytesRead, NULL);

	if (!result) {
		DEBUG_PRINT("Can't read volume\n", 1);
		return NULL;
	}

	pStoreInfo = (PSTORE_INFORMATION)buffer;
	pHeader = &(STORE_BLOCK_HEADER)(pStoreInfo->header);
	pInfo = &(STORE_BLOCK_INFORMATION)(pStoreInfo->information);

	if (pHeader->record_type != VSC_STORE_HEADER) {
		DEBUG_PRINT("record_type != VSC_STORE_HEADER\n", 1);
		return NULL;
	}

#if defined (HAVE_DEBUG_OUTPUT)
	printf("=================================Store Header================================\n");
	printf("\t\tVSS identifier: ");
	printf_guid(pHeader->vss_identifier);
	printf("\t\tVersion: 0x%I64x\n", pHeader->version);
	printf("\t\tRecord type: 0x%I64x\n", pHeader->record_type);
	printf("\t\tRelative (block) offset: 0x%I64x\n", pHeader->relative_block_offset);
	printf("\t\tCurrent (block) offset: 0x%I64x\n", pHeader->current_block_offset);
	printf("\t\tNext (block) offsetL 0x%I64x\n", pHeader->next_block_offset);
	printf("\t\tSize of store information: 0x%I64x\n", pHeader->store_information_size);
	printf("=================================Store Info==================================\n");
	printf("\t\tUnknown (identifier?): ");
	printf_guid(pInfo->unknown_identifier);
	printf("\t\tVSC identifier: ");
	printf_guid(pInfo->shadow_copy_identifier);
	printf("\t\tVSC set identifier: ");
	printf_guid(pInfo->shadow_copy_set_identifier);
	printf("\t\tSnapshot context: 0x%I32x\n", pInfo->snapshot_context);
	printf("\t\tUnknown (Provider?): 0x%I32x\n", pInfo->provider);
	printf("\t\tAttribute flags: 0x%I32x\n", pInfo->attribute_flags);
	printf("=============================================================================\n");
#endif

	return pInfo;
}

#define HAVE_DEBUG_OUTPUT


PSTORE_BLOCK_DESCRIPTOR storeBlockIterator(HANDLE drive, uint64_t offset, PSTORE_BLOCK_DESCRIPTOR(*func)(HANDLE drive, PSTORE_BLOCK_DESCRIPTOR entry, void* buffer), void* buf) {
	PSTORE_BLOCK_LIST pstore_block_list = NULL;
	PSTORE_BLOCK_DESCRIPTOR block_descriptor_list = NULL;
	BOOL result;
	DWORD numOfBytesRead = 1;
	void* zero_buf = calloc(1, sizeof(STORE_BLOCK_DESCRIPTOR));

	setFilePointer(drive, offset);

	void* buffer = calloc(VSC_DATA_SIZE, sizeof(char));
	if (!buffer) {
		DEBUG_PRINT("Can't allocate memory\n", 1);
		return NULL;
	}

	do {
		result = ReadFile(drive, buffer, VSC_DATA_SIZE, &numOfBytesRead, NULL);

		if (!result) {
			DEBUG_PRINT("Can't read volume\n", 1);
			return NULL;
		}

		pstore_block_list = (PSTORE_BLOCK_LIST)buffer;
		block_descriptor_list = pstore_block_list->store_block_list;

		if (pstore_block_list->header.record_type != VSC_BLOCK_DESCRIPTOR_LIST) {
			DEBUG_PRINT("record_type != VSC_BLOCK_DESCRIPTOR_LIST\n", 1);
			continue;
		}

		for (int i = 0; i < QT_BLOCK_DESCRIPTORS; i++) {
			if(!memcmp(zero_buf, &(block_descriptor_list[i]), sizeof(STORE_BLOCK_DESCRIPTOR))) continue;
			
			if (func) {
				STORE_BLOCK_DESCRIPTOR *store_block_descriptor = func(drive, &(block_descriptor_list[i]), buf);
				if (store_block_descriptor) {
					free(buffer);
					free(zero_buf);
					return store_block_descriptor;
				}
			}
		}
		setFilePointer(drive, pstore_block_list->header.next_block_offset);

	} while (pstore_block_list->header.next_block_offset != 0);

	free(buffer);
	free(zero_buf);
	return NULL;
}

PSTORE_BLOCK_DESCRIPTOR printBlockDescriptor(HANDLE drive, PSTORE_BLOCK_DESCRIPTOR descriptor, void* buffer) {
	printf("\t\tOriginal data block offset: 0x%I64x\n", descriptor->original_data_block_offset);
	printf("\t\tRelative store data block offset: 0x%I64x\n", descriptor->relative_store_data_block_offset);
	printf("\t\tStore data block offset: 0x%I64x\n", descriptor->store_data_block_offset);
	printf("\t\tFlags: 0x%I32x\n", descriptor->flags);
	printf("\t\tAllocation bitmap: 0x%I32x\n\n", descriptor->allocation_bitmap);
	return NULL;
}

void printStoreBlockList(HANDLE drive, uint64_t offset) {
	storeBlockIterator(drive, offset, printBlockDescriptor, NULL);
}

/*
PSTORE_BLOCK_RANGE_LIST getStoreBlockRangeList(HANDLE drive, uint64_t offset) {
	PSTORE_BLOCK_RANGE_LIST pstore_block_range_list = NULL;
	HRESULT result;
	DWORD numOfBytesRead = 1;

	setFilePointer(drive, offset);

	void* buffer = calloc(0x4000, sizeof(char));
	if (!buffer) {
		DEBUG_PRINT("Can't allocate memory\n", 1);
		return NULL;
	}

	do {
		result = ReadFile(drive, buffer, 0x4000, &numOfBytesRead, NULL);

		if (!result) {
			DEBUG_PRINT("Can't read volume\n", 1);
			return NULL;
		}

		pstore_block_range_list = (PSTORE_BLOCK_RANGE_LIST)buffer;
		if (pstore_block_range_list->header.record_type != 0x5) break;

		for (int i = 0; i < 165; i++) {
			if (pstore_block_range_list->store_block_range_list_array[i].block_range_size == 0) continue;
			printf("\t\tStore (block range start) offset: 0x%I64x\n", pstore_block_range_list->store_block_range_list_array[i].store_offset);
			printf("\t\tRelative (block range start) offset: 0x%I64x\n", pstore_block_range_list->store_block_range_list_array[i].relative_offset);
			printf("\t\tBlock range size: 0x%I64x\n\n", pstore_block_range_list->store_block_range_list_array[i].block_range_size);
		}
		
		setFilePointer(drive, pstore_block_range_list->header.next_block_offset);

	} while (pstore_block_range_list->header.next_block_offset != 0);

	return pstore_block_range_list;
}
*/

