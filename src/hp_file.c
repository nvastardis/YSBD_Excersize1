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

typedef struct {
  int RecordCount;
  BF_Block* NextBlock;
} HP_block_info;


int HP_CreateFile(char *fileName){
  int fileDesc;
  void *data;
  HP_block_info *hpBlock;
  BF_Block *firstBlock;

  CALL_BF(BF_CreateFile(fileName));
  CALL_BF(BF_OpenFile(fileName, &fileDesc));
  CALL_BF(BF_GetBlock(fileDesc, 0, firstBlock));
  data = BF_Block_GetData(firstBlock);
  Record* rec = data;
  hpBlock = malloc(sizeof(HP_block_info));
  hpBlock->RecordCount = 0;
  hpBlock->NextBlock = NULL;
  memcpy(&firstBlock + 10 + sizeof(HP_block_info), hpBlock, sizeof(HP_block_info));
  CALL_BF(BF_UnpinBlock(firstBlock));
  return 0;
}

HP_info* HP_OpenFile(char *fileName){
  HP_info *fileInfo;
  
  int fileId;

  CALL_BF(BF_OpenFile(fileName, &fileId));
  fileInfo->fileDescription = fileId;
  return fileInfo;
}


int HP_CloseFile( HP_info *header_info ){
  CALL_BF(BF_CloseFile(header_info->fileDescription));
  free(header_info);
  return 0;
}

int HP_InsertEntry(HP_info *header_info, Record record){
  int counter,
      numberOfBlocks,
      maxNumberOfRecordsPerBlock;

  maxNumberOfRecordsPerBlock = BF_BLOCK_SIZE / sizeof(Record);
  CALL_BF(BF_GetBlockCounter(header_info->fileDescription, &numberOfBlocks));

  for(counter = 0; counter < numberOfBlocks; counter++){
    void *data;
    BF_Block *block;
    Record *records;
    HP_block_info block_info;

    CALL_BF(BF_GetBlock(header_info->fileDescription, counter, block));
    data = BF_Block_GetData(block);
    records = (Record*)data;
    memcpy(&block_info, data+10, sizeof(HP_block_info));
    if(block_info.RecordCount == maxNumberOfRecordsPerBlock){
      continue;
    }
    records[block_info.RecordCount + 1] = record;
    block_info.RecordCount ++;
    BF_Block_SetDirty(block);
    CALL_BF(BF_UnpinBlock(block));
    return 0;
  }

}

int HP_GetAllEntries(HP_info *header_info, int id){
    int counter, blocksInFile, blockSearched;

    CALL_BF(BF_GetBlockCounter(header_info->fileDescription, &blocksInFile))
    for(counter = 0; counter < blocksInFile; counter++){
        int recordCounter;
        void *data;
        BF_Block *block;
        Record *records;
        HP_block_info block_info;

        CALL_BF(BF_GetBlock(header_info->fileDescription, counter, block));
        data = BF_Block_GetData(block);
        records = (Record*) data;
        memcpy(&block_info, data+10, sizeof(HP_block_info));
        for(recordCounter = 0; recordCounter < block_info.RecordCount; recordCounter++){
            if(records[recordCounter].id == id){
                fprintf(stdout, "%d\t%s\t%s\t%s", 
                                records[recordCounter].id,
                                records[recordCounter].record,
                                records[recordCounter].name,
                                records[recordCounter].surname,
                                records[recordCounter].city
                                );
            }
        }
    }
    return blocksInFile;
}
