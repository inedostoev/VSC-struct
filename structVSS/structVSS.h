#ifndef STRUCT_VSS_H
#define STRUCT_VSS_H


#include <Windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>


#define VSC_HEADER_OFFSET			0x1e00
#define MINIMAL_VSC_DATA_SIZE		0x200
#define VSC_DATA_SIZE				0x4000


enum RECORD_TYPE {
	VSC_HEADER =					0x1,
	VSC_CATALOG_BLOCK_HEADER =		0x2,
	VSC_BLOCK_DESCRIPTOR_LIST =		0x3,
	VSC_STORE_HEADER =				0x4,
	VSC_STORE_BLOCK_RANGES_LIST =	0x5,
	VSC_STORE_BITMAP =				0x6,
};

enum CATALOG_ENTRY_TYPE {
	EMPTY_CATALOG_ENTRY =			0x0,
	UNUSED_CATALOG_ENTRY =			0x1,
	CATALOG_ENTRY_2 =				0x2,
	CATALOG_ENTRY_3 =				0x3,
};

/* Volume Shadow Copy Header */
typedef struct {
	GUID vss_identifier;
	uint32_t version;
	uint32_t record_type;
	uint64_t current_offset;
	uint64_t unknown_1;
	uint64_t unknown_2;
	uint64_t catalog_offset;
	uint64_t maximum_size;
	GUID volume_identifier;
	GUID shadow_copy_storage_volume_identifier;
	uint8_t unknown_3:4; 
	uint32_t unknown_4[103];
} VOLUME_HEADER, *PVOLUME_HEADER;

PVOLUME_HEADER getVscHeader(HANDLE drive);
uint64_t getVscCatalogOffset(HANDLE drive);


/* Volume Shadow Copy Catalog entry */
typedef struct {
	uint64_t unknown_1[15];
} CATALOG_ENTRY_0x01, * PCATALOG_ENTRY_0x01;

typedef struct {
	uint64_t volume_size;
	GUID store_identifier;
	uint64_t unknown_1;
	uint64_t unknown_2;
	uint64_t creation_time;
	uint64_t unknown_3[9];
} CATALOG_ENTRY_0x02, * PCATALOG_ENTRY_0x02;

typedef struct {
	uint64_t store_block_list_offset;
	GUID store_identifier;
	uint64_t store_header_offset;
	uint64_t store_block_range_list_offset;
	uint64_t store_current_bitmap_offset;
	uint64_t ntfs_file_ref;
	uint64_t allocated_size;
	uint64_t store_previous_bitmap_offset;
	uint64_t unknown_1;
	uint64_t unknown_2[5];
} CATALOG_ENTRY_0x03, * PCATALOG_ENTRY_0x03;

typedef struct {
	uint64_t version;
	union {
		CATALOG_ENTRY_0x01 catalog_entry_0x01;
		CATALOG_ENTRY_0x02 catalog_entry_0x02;
		CATALOG_ENTRY_0x03 catalog_entry_0x03;
	};
} CATALOG_ENTRY, *PCATALOG_ENTRY;

/* Volume Shadow Copy Catalog block header */
typedef struct{
	GUID vss_identifier;
	uint32_t version;
	uint32_t record_type;
	uint64_t relative_offset;
	uint64_t current_offset;
	uint64_t next_offset;
	uint64_t unknown_1[10];
} CATALOG_BLOCK_HEADER, *PCATALOG_BLOCK_HEADER;

#define QT_CATALOGS_ENTRY	((VSC_DATA_SIZE - sizeof(CATALOG_BLOCK_HEADER)) / sizeof(CATALOG_ENTRY_0x01))

typedef struct {
	CATALOG_BLOCK_HEADER header;
	CATALOG_ENTRY catalog_entry[QT_CATALOGS_ENTRY];
} CATALOG_BLOCK, *PCATALOG_BLOCK;


PCATALOG_ENTRY catalogEntryIterator(HANDLE drive, uint64_t offset, PCATALOG_ENTRY (*func)(HANDLE drive, PCATALOG_ENTRY entry, void *buffer), void* buf);
void printCatalog(HANDLE drive, uint64_t offset);
PCATALOG_ENTRY getCatalogEntryByVscGuid(HANDLE drive, uint64_t offset, GUID guid);
PCATALOG_ENTRY printCatalogEntry(HANDLE drive, PCATALOG_ENTRY entry, void* buf);

/* Store header */
typedef struct {
	GUID vss_identifier;
	uint32_t version;
	uint32_t record_type;
	uint64_t relative_block_offset;
	uint64_t current_block_offset;
	uint64_t next_block_offset;
	uint64_t store_information_size;
	uint64_t unknown[9];
}STORE_BLOCK_HEADER, *PSTORE_BLOCK_HEADER;

/* Store information */
typedef struct {
	GUID unknown_identifier;
	GUID shadow_copy_identifier;
	GUID shadow_copy_set_identifier;
	uint32_t snapshot_context;
	uint32_t provider;
	uint32_t attribute_flags;
	//and another
}STORE_BLOCK_INFORMATION, *PSTORE_BLOCK_INFORMATION;

typedef struct {
	STORE_BLOCK_HEADER header;
	STORE_BLOCK_INFORMATION information;
}STORE_INFORMATION, *PSTORE_INFORMATION;

PSTORE_BLOCK_INFORMATION getStoreInformation(HANDLE drive, uint64_t offset);


/* Store block list */

#define QT_BLOCK_DESCRIPTORS ((VSC_DATA_SIZE - sizeof(STORE_BLOCK_HEADER)) / sizeof(STORE_BLOCK_DESCRIPTOR)) 

typedef struct {
	uint64_t original_data_block_offset;
	uint64_t relative_store_data_block_offset;
	uint64_t store_data_block_offset;
	uint32_t flags;
	uint32_t allocation_bitmap;
} STORE_BLOCK_DESCRIPTOR, *PSTORE_BLOCK_DESCRIPTOR;

typedef struct {
	STORE_BLOCK_HEADER header;
	STORE_BLOCK_DESCRIPTOR store_block_list[QT_BLOCK_DESCRIPTORS];
} STORE_BLOCK_LIST, *PSTORE_BLOCK_LIST;

PSTORE_BLOCK_DESCRIPTOR storeBlockIterator(HANDLE drive, uint64_t offset, PSTORE_BLOCK_DESCRIPTOR(*func)(HANDLE drive, PSTORE_BLOCK_DESCRIPTOR entry, void* buffer), void* buf);
void printStoreBlockList(HANDLE drive, uint64_t offset);

/*

typedef struct {
	uint64_t store_offset;
	uint64_t relative_offset;
	uint64_t block_range_size;
} STORE_BLOCK_RANGE_LIST_ARRAY, *PSTORE_BLOCK_RANGE_LIST_ARRAY;

typedef struct {
	STORE_BLOCK_HEADER header;
	STORE_BLOCK_RANGE_LIST_ARRAY store_block_range_list_array[165];
	uint64_t unused; 
} STORE_BLOCK_RANGE_LIST, *PSTORE_BLOCK_RANGE_LIST;


PSTORE_BLOCK_RANGE_LIST getStoreBlockRangeList(HANDLE drive, uint64_t offset);

*/

/*****************************FUNCTIONS*****************************/

HANDLE openVolume(LPCWSTR volumePath);
void setFilePointer(HANDLE drive, LONGLONG offset);


#endif