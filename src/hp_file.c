#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "hp_file.h"
#include "record.h"

#define CALL_BF(call)       \
{                           \
  BF_ErrorCode code = call; \
  if (code != BF_OK) {         \
    BF_PrintError(code);    \
    exit(code);        \
  }                         \
}

#define RECORDS_PER_BLOCK BF_BLOCK_SIZE / sizeof(Record)

typedef struct {
  int RecordCount;
  BF_Block* NextBlock;
} HP_block_info;

int SetUpNewBlock(HP_info *hp_info, Record record, BF_Block *previousBlock);


int HP_CreateFile(char *fileName){
    int fileDesc;
    void *data;
    HP_info *hp_info;
    BF_Block *firstBlock;

    if(BF_CreateFile(fileName)){
        return -1;
    }
    if(BF_OpenFile(fileName, &fileDesc)){
        return -1;
    }
    BF_Block_Init(&firstBlock);
    if(BF_AllocateBlock( fileDesc, firstBlock)){
        return -1;;
    }

    hp_info = (HP_info *) BF_Block_GetData(firstBlock);
    hp_info->fileDescription = fileDesc;

    BF_Block_SetDirty(firstBlock);
    if(BF_UnpinBlock(firstBlock)){
        return -1;
    }
    if(BF_CloseFile(fileDesc)){
        return -1;
    }
    return 0;
}

HP_info* HP_OpenFile(char *fileName){
    HP_info *fileInfo;
    fileInfo = malloc(sizeof(HP_info*));
    if(fileInfo == NULL){
        return NULL;
    }
    if(BF_OpenFile(fileName, &fileInfo->fileDescription)){
        return NULL;
    }
    return fileInfo;
}


int HP_CloseFile( HP_info *header_info ){
    int counter,
        numberOfBlocks;

    if(BF_GetBlockCounter(header_info->fileDescription, &numberOfBlocks)){
        return -1;
    }
    for(counter = 0; counter < numberOfBlocks; counter++){
        BF_Block *block;

        BF_Block_Init(&block);
        if(BF_GetBlock(header_info->fileDescription, counter, block)){
            return -1;
        }
        BF_Block_Destroy(&block);
    }

    if(BF_CloseFile(header_info->fileDescription)){
        return -1;
    }
    free(header_info);
    return 0;
}

int HP_InsertEntry(HP_info *header_info, Record record){
    int counter,
        numberOfBlocks;
    void *data;
    BF_Block *block;
    Record *records;
    HP_block_info block_info;
    if(BF_GetBlockCounter(header_info->fileDescription, &numberOfBlocks)){
        return -1;
    }
    
    if(numberOfBlocks == 1){
        return (SetUpNewBlock(header_info, record, NULL));
    }
    else{
        BF_Block_Init(&block);
        if(BF_GetBlock(header_info->fileDescription, numberOfBlocks - 1, block)){
            return -1;
        }
        data = BF_Block_GetData(block);
        memcpy(&block_info, data+10, sizeof(HP_block_info));
        if(block_info.RecordCount ==RECORDS_PER_BLOCK){
            return (SetUpNewBlock(header_info, record, block));
        }
        records = (Record*)data;
        records[block_info.RecordCount] = record;
        block_info.RecordCount++;
        memcpy(data+10, &block_info, sizeof(HP_block_info));
        BF_Block_SetDirty(block);
        if(BF_UnpinBlock(block)){
            return -1;
        }
    }
    return 0;

}

int HP_GetAllEntries(HP_info *header_info, int id){
    int recordCounter, counter, blocksInFile, recordsSearched, fileFound;
    void *data;
    Record *records;

    recordsSearched = 0;
    fileFound = 0;
    if(BF_GetBlockCounter(header_info->fileDescription, &blocksInFile)){
        return -1;
    }

    for(counter = 1; counter < blocksInFile; counter++){
        BF_Block *block;
        HP_block_info block_info;

        BF_Block_Init(&block);
        if(BF_GetBlock(header_info->fileDescription, counter, block)){
            return -1;
        }
        data = BF_Block_GetData(block);
        records = (Record*) data;
        memcpy(&block_info, data+10, sizeof(HP_block_info));
        for(recordCounter = 0; recordCounter < block_info.RecordCount; recordCounter++){
            if(records[recordCounter].id == id){
                printf( "%d\t%s\t%s\t%s\t%s\n", 
                                records[recordCounter].id,
                                records[recordCounter].record,
                                records[recordCounter].name,
                                records[recordCounter].surname,
                                records[recordCounter].city
                                );
                if(BF_UnpinBlock(block)){
                    return -1;
                }
                return recordsSearched;
                
            }
            recordsSearched++;
        }
        if(BF_UnpinBlock(block)){
            return -1;
        }
    }
    return -1;
}

int SetUpNewBlock(HP_info *hp_info, Record record, BF_Block *previousBlock){
    int counter,
        numberOfBlocks;
    void *data;
    BF_Block *block;
    Record *blockData;
    HP_block_info block_info;

    BF_Block_Init(&block);
    if(BF_AllocateBlock(hp_info->fileDescription, block)){
        return -1;
    }
    data = BF_Block_GetData(block);
    blockData = (Record *) data;
    blockData[0] = record;
    
    block_info.NextBlock = NULL;
    block_info.RecordCount = 1;
    memcpy(data+10, &block_info, sizeof(HP_block_info));
    
    BF_Block_SetDirty(block);
    if(BF_UnpinBlock(block)){
        return -1;
    }

    if(previousBlock != NULL){
        data = BF_Block_GetData(previousBlock);
        memcpy(&block_info, data + 10, sizeof(HP_block_info));

        block_info.NextBlock = block;

        memcpy(data + 10, block, sizeof(HP_block_info));
    }
    return -1;

    
}
