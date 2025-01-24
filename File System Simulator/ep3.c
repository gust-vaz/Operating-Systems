#include "ep3.h"

FILE *disk;
uint8_t *bitmap;
int16_t *FAT;
DBNode *root;

unsigned char *dataBuffer;
unsigned char *metadata;
int nFatBlocks = (N_BLOCKS/(BLOCK_SIZE/2)) + (N_BLOCKS%(BLOCK_SIZE/2)!=0);

// AUXILIARY FUNCTIONS
void setBit(int bit_index, int value){
    int byte_index = bit_index / 8;
    int bit_position = bit_index % 8;
    if(value)   bitmap[byte_index] |= (1 << bit_position);  // Set the bit to 1
    else        bitmap[byte_index] &= ~(1 << bit_position); // Set the bit to 0
}
int getBit(int bit_index){
    int byte_index = bit_index / 8;
    int bit_position = bit_index % 8;
    return(bitmap[byte_index] >> bit_position) & 1;
}
void updateBitmap(){
    fseek(disk, 0, SEEK_SET);
    fwrite(bitmap, 1, BLOCK_SIZE, disk);
}
void updateFAT(){
    fseek(disk, -nFatBlocks*BLOCK_SIZE, SEEK_END);
    fwrite(FAT, sizeof(uint16_t), N_BLOCKS, disk);
}
TimeDetails decodifyTime(long int time){
    TimeDetails details;
    struct tm *timeinfo;
    time_t tp = (time_t) time;
    timeinfo = localtime(&tp);
    details.year = timeinfo->tm_year+1900;
    details.mon = timeinfo->tm_mon+1;
    details.day = timeinfo->tm_mday;
    details.hour = timeinfo->tm_hour;
    details.min = timeinfo->tm_min;
    details.sec = timeinfo->tm_sec;
    return details;
}
TokensLL *spliter(char* input, char* delimiter){
    char *token = strtok(input, delimiter);
    TokensLL *head = NULL;
    TokensLL *curr = NULL;
    TokensLL *new = NULL;
    while(token != NULL){
        new = (TokensLL*)malloc(sizeof(TokensLL));
        new->tok = (char*)malloc((strlen(token)+1)*sizeof(char));
        strcpy(new->tok, token);
        new->next = NULL;
        if(head == NULL) head = new;
        else curr->next = new;
        curr = new;
        token = strtok(NULL, delimiter);
    }
    return head;
}

// MANIPULATION FUNCTIONS
FileLL *getFiles(int blockIndex){
    FileLL *head, *curr, *new; 
    int k, currIndex = 0;
    head = curr = NULL;

    fseek(disk, blockIndex*BLOCK_SIZE, SEEK_SET);
    fread(metadata, 1, METADATA_SIZE, disk);
    while(metadata[0] != 0){
        new = (FileLL*)malloc(sizeof(FileLL));
        new->name = (char*)malloc((MAX_FILE_NAME+1)*sizeof(char));
        new->pos.block = blockIndex;
        new->pos.index = currIndex;
    
        for(k = 0; k < MAX_FILE_NAME; k++)
            new->name[k] = metadata[k];
        new->name[k+1] = '\0';
        new->content = (metadata[k+1] << 8) | metadata[k]; k += 2;
        new->size = (metadata[k+3] << 24) | (metadata[k+2] << 16) | (metadata[k+1] << 8) | metadata[k]; k += 4;
        new->access = (metadata[k+3] << 24) | (metadata[k+2] << 16) | (metadata[k+1] << 8) | metadata[k]; k += 4;
        new->modification = (metadata[k+3] << 24) | (metadata[k+2] << 16) | (metadata[k+1] << 8) | metadata[k]; k += 4;
        new->creation = (metadata[k+3] << 24) | (metadata[k+2] << 16) | (metadata[k+1] << 8) | metadata[k]; k += 4;

        new->next = NULL;
        if(head == NULL) head = new;
        else curr->next = new;
        curr = new;

        currIndex += METADATA_SIZE;
        if(currIndex+METADATA_SIZE >= BLOCK_SIZE){
            blockIndex = FAT[blockIndex];
            if(blockIndex == -1) break;
            fseek(disk, blockIndex*BLOCK_SIZE, SEEK_SET);
            currIndex = 0;
        }
        fread(metadata, 1, METADATA_SIZE, disk);
    }
    return head;
}
int createGenericFile(int blockIndex, int currIndex, char *name, int contentSize){
    int k, contentBlock;
    // Allocate space for the file metadata
    if(currIndex+METADATA_SIZE >= BLOCK_SIZE)
        for(int i = 0; i < N_BLOCKS; i++)
            if(!getBit(i)){
                setBit(i, 1);
                FAT[blockIndex] = i;
                blockIndex = i;
                currIndex = 0;
                break;
            }
    // Allocate space for the file content
    for(int i = 0; i < N_BLOCKS; i++)
        if(!getBit(i)){
            setBit(i, 1);
            contentBlock = i;
            break;
        }
    // Getting name
    for(k = 0; k < strlen(name); k++)
        metadata[k] = name[k];
    for(; k < MAX_FILE_NAME; k++)
        metadata[k] = 0;
    // Getting content block
    metadata[k++] = contentBlock & 0xFF;
    metadata[k++] = (contentBlock >> 8) & 0xFF;
    // Getting size
    metadata[k++] = contentSize & 0xFF;
    metadata[k++] = (contentSize >> 8) & 0xFF;
    metadata[k++] = (contentSize >> 16) & 0xFF;
    metadata[k++] = (contentSize >> 24) & 0xFF;
    // Getting time
    long int now; now = time(NULL);
    for(int i = 0; i < 3; i++){
        metadata[k++] = now & 0xFF;
        metadata[k++] = (now >> 8) & 0xFF;
        metadata[k++] = (now >> 16) & 0xFF;
        metadata[k++] = (now >> 24) & 0xFF;
    }
    // Write metadata
    fseek(disk, blockIndex*BLOCK_SIZE, SEEK_SET);
    fread(dataBuffer, 1, BLOCK_SIZE, disk);
    for(int i = 0; i < METADATA_SIZE; i++)
        dataBuffer[currIndex+i] = metadata[i];
    fseek(disk, blockIndex*BLOCK_SIZE, SEEK_SET);
    fwrite(dataBuffer, 1, BLOCK_SIZE, disk);

    return contentBlock;
}
void updateTime(Position pos, int type){
    fseek(disk, pos.block*BLOCK_SIZE, SEEK_SET);
    fread(dataBuffer, 1, BLOCK_SIZE, disk);
    long int now; now = time(NULL);
    dataBuffer[pos.index+MAX_FILE_NAME+6+type*4] = now & 0xFF;
    dataBuffer[pos.index+MAX_FILE_NAME+7+type*4] = (now >> 8) & 0xFF;
    dataBuffer[pos.index+MAX_FILE_NAME+8+type*4] = (now >> 16) & 0xFF;
    dataBuffer[pos.index+MAX_FILE_NAME+9+type*4] = (now >> 24) & 0xFF;
    fseek(disk, pos.block*BLOCK_SIZE, SEEK_SET);
    fwrite(dataBuffer, 1, BLOCK_SIZE, disk);    
}
void transferBlock(Position oldPos, Position newPos){
    if(oldPos.block == newPos.block && oldPos.index == newPos.index) return;
    
    unsigned char *oldData = (unsigned char*)malloc(BLOCK_SIZE);
    fseek(disk, oldPos.block*BLOCK_SIZE, SEEK_SET);
    fread(oldData, 1, BLOCK_SIZE, disk);

    fseek(disk, newPos.block*BLOCK_SIZE, SEEK_SET);
    fread(dataBuffer, 1, BLOCK_SIZE, disk);
    for(int i = 0; i < METADATA_SIZE; i++)
        dataBuffer[newPos.index+i] = oldData[oldPos.index+i];
    fseek(disk, newPos.block*BLOCK_SIZE, SEEK_SET);
    fwrite(dataBuffer, 1, BLOCK_SIZE, disk);
    free(oldData);
}
void resetBlock(Position pos, int size){
    fseek(disk, pos.block*BLOCK_SIZE, SEEK_SET);
    fread(dataBuffer, 1, BLOCK_SIZE, disk);
    for(int i = 0; i < size; i++)
        dataBuffer[pos.index+i] = 0;
    fseek(disk, pos.block*BLOCK_SIZE, SEEK_SET);
    fwrite(dataBuffer, 1, BLOCK_SIZE, disk);
}
void DBDestroyer(DBNode *node){
    for(int i = 0; i < node->nChildren; i++)
        DBDestroyer(node->children[i]);
    if(node->nChildren > 0)
        free(node->children);
    free(node->name);
    free(node);
}
void DBPrinter(DBNode *node, int level){
    for(int i = 0; i < level; i++){
        if(i == level-1) printf(" ├── ");
        else printf(" │   ");
    }
    if(node->size == IS_DIR) printf(ANSI_COLOR_GREEN ANSI_STYLE_BOLD "%s/\n" ANSI_RESET_ALL, node->name);
    else printf(ANSI_STYLE_BOLD "%s\n" ANSI_RESET_ALL, node->name);
    for(int i = 0; i < node->nChildren; i++)
        DBPrinter(node->children[i], level+1);
}
void DBCounter(DBNode *node, int *nFiles, int *nDirs, long int *wastedSpace){
    if(node->size == IS_DIR) 
        (*nDirs)++;
    else{
        (*nFiles)++;
        (*wastedSpace) += BLOCK_SIZE - node->size % BLOCK_SIZE;
    }
    for(int i = 0; i < node->nChildren; i++)
        DBCounter(node->children[i], nFiles, nDirs, wastedSpace);
}

// COMMAND FUNCTIONS
void mount(char* path){
    int nEmptyBlocks = N_BLOCKS - 2 - nFatBlocks;
    if(disk != NULL) dismount();
    bitmap = (uint8_t*)malloc(BLOCK_SIZE);
    FAT = (int16_t*)malloc(N_BLOCKS*sizeof(int16_t));
    dataBuffer = (unsigned char*)malloc(BLOCK_SIZE);
    metadata = (unsigned char*)malloc(METADATA_SIZE);

    // Load file system
    if(access(path, F_OK) == 0){
        disk = fopen(path, "r+b");
        if(disk == NULL){
            printf("Erro ao abrir o sistema de arquivos\n");
            return;
        }
        fseek(disk, 0, SEEK_SET);
        fread(bitmap, 1, BLOCK_SIZE, disk);
        // Load FAT
        fseek(disk, -nFatBlocks*BLOCK_SIZE, SEEK_END);
        fread(FAT, sizeof(int16_t), N_BLOCKS, disk);

        DBNode *toPrint = updateDB("", 1);
        DBPrinter(toPrint, 0);
        DBDestroyer(toPrint);
    }
    // Create file system
    else{
        // Set buffers
        int16_t *fatBuffer = (int16_t*)malloc(BLOCK_SIZE);
        for(int i = 0; i < BLOCK_SIZE/2; i++) fatBuffer[i] = -1;
        for(int i = 0; i < BLOCK_SIZE; i++) dataBuffer[i] = 0;
        for(int i = 0; i < BLOCK_SIZE; i++) bitmap[i] = 0;
        setBit(0, 1); // itself is using space
        setBit(1, 1); // root is using space
        for(int i = N_BLOCKS-nFatBlocks; i < N_BLOCKS; i++)
            setBit(i, 1); // FAT is using space
        // Create the file
        disk = fopen(path, "w+b");
        if(disk == NULL){
            free(fatBuffer);
            printf("Erro ao criar um novo sistema de arquivos\n"); 
            return;
        }
        // Fill the file
        fwrite(bitmap, 1, BLOCK_SIZE, disk);            // bitmap block
        fwrite(dataBuffer, 1, BLOCK_SIZE, disk);        // root block
        for(int i = 0; i < nEmptyBlocks; i++)
            fwrite(dataBuffer, 1, BLOCK_SIZE, disk);    // empty blocks
        for(int i = 0; i < nFatBlocks; i++)
            fwrite(fatBuffer, 1, BLOCK_SIZE, disk);     // FAT blocks
        free(fatBuffer);
        // Load FAT
        fseek(disk, -nFatBlocks*BLOCK_SIZE, SEEK_END);
        fread(FAT, sizeof(int16_t), N_BLOCKS, disk);
    }
}
void dismount(){
    free(dataBuffer); free(metadata);
    free(bitmap); free(FAT);
    fclose(disk); disk = NULL;
}
int createDir(TokensLL* path, int blockIndex, int isFirstCheck){
    int currIndex = 0;
    // Check if the directory already exists
    FileLL *headFiles = getFiles(blockIndex);
    while(headFiles != NULL){
        if(headFiles->size == IS_DIR && strcmp(headFiles->name, path->tok) == 0){
            if(path->next != NULL && createDir(path->next, headFiles->content, isFirstCheck)){
                updateTime(headFiles->pos, 0);
                updateTime(headFiles->pos, 1);
            }
            return 0;
        }
        blockIndex = headFiles->pos.block;
        currIndex = headFiles->pos.index + METADATA_SIZE;
        headFiles = headFiles->next;
    }
    // Check if there's enough space to insert all the new directories
    if(isFirstCheck){
        int nBlocks = 0;
        if(currIndex+METADATA_SIZE >= BLOCK_SIZE) nBlocks++;
        for(TokensLL *aux = path; aux != NULL; aux = aux->next) nBlocks++;
        for(int i = 0; i < N_BLOCKS; i++){
            if(!getBit(i)){
                nBlocks--;
                if(nBlocks == 0) break;
            }
        }
        if(nBlocks > 0){
            printf("Não há espaço para criar o diretório %s\n", path->tok);
            return 0;
        }
    }
    // Create the directory
    int contentBlock = createGenericFile(blockIndex, currIndex, path->tok, IS_DIR);
    if(path->next != NULL) createDir(path->next, contentBlock, 0);
    updateBitmap(); updateFAT();
    return 1;
}
void listDir(TokensLL* path, int blockIndex){
    TimeDetails creation, modification, access;
    FileLL *headFiles = getFiles(blockIndex);
    if(path == NULL){
        while(headFiles != NULL){
            creation = decodifyTime(headFiles->creation);
            modification = decodifyTime(headFiles->modification);
            access = decodifyTime(headFiles->access);

            if(headFiles->size == IS_DIR) printf(ANSI_COLOR_GREEN ANSI_STYLE_BOLD "%s/\n" ANSI_RESET_ALL, headFiles->name);
            else printf(ANSI_STYLE_BOLD "%s \n " ANSI_RESET_ALL "Tamanho: %ld B\n", headFiles->name, headFiles->size);
            printf(" Data de Criação | Data de Última Modificação \t| Data de Último Acesso\n");
            printf(" %.2d/%.2d/%.4d\t ", creation.day, creation.mon, creation.year);
            printf("| %.2d/%.2d/%.4d\t\t\t", modification.day, modification.mon, modification.year);
            printf("| %.2d/%.2d/%.4d\n", access.day, access.mon, access.year);
            printf(" %.2d:%.2d:%.2d\t ", creation.hour, creation.min, creation.sec);
            printf("| %.2d:%.2d:%.2d\t\t\t", modification.hour, modification.min, modification.sec);
            printf("| %.2d:%.2d:%.2d\n", access.hour, access.min, access.sec);
            headFiles = headFiles->next;
        }
    }
    else{
        while(headFiles != NULL && !(headFiles->size == IS_DIR && strcmp(headFiles->name, path->tok) == 0))
            headFiles = headFiles->next;
        if(headFiles == NULL){
            printf("Diretório %s não encontrado\n", path->tok);
            return;
        }
        // Update the access time only if it's the last directory we're seeing
        listDir(path->next, headFiles->content);
        if(path->next == NULL)
            updateTime(headFiles->pos, 0);
    }
}
int copyFile(TokensLL* origin, TokensLL* destiny, int blockIndex){
    int currIndex = 0, oldBlock = blockIndex;
    // Check if the destiny exists
    FileLL *headFiles = getFiles(blockIndex);
    if(destiny->next != NULL){
        while(headFiles != NULL){
            if(headFiles->size == IS_DIR && strcmp(headFiles->name, destiny->tok) == 0){
                if(copyFile(origin, destiny->next, headFiles->content)){
                    updateTime(headFiles->pos, 0);
                    updateTime(headFiles->pos, 1);
                }
                return 0;
            }
            headFiles = headFiles->next;
        }
        printf("Diretório destino não encontrado\n");
        return 0;
    }
    // Check if the origin exists
    FILE *obj = fopen(origin->tok, "r");
    if(obj == NULL){
        printf("Arquivo %s não encontrado\n", origin->tok);
        return 0;
    }
    // Get the size of the file
    int contentSize = 0;
    for(char ch = fgetc(obj); ch != EOF; ch = fgetc(obj))
        contentSize++;
    fclose(obj);
    // Check if there's enough space to insert the file
    int nBlocks = (contentSize / BLOCK_SIZE) + (contentSize % BLOCK_SIZE != 0);
    while(headFiles != NULL){
        currIndex = headFiles->pos.index + METADATA_SIZE;
        blockIndex = headFiles->pos.block;
        // Check if the file already exists
        if(headFiles->size != IS_DIR && strcmp(headFiles->name, destiny->tok) == 0){
            int moreBlocks = nBlocks - (headFiles->size / BLOCK_SIZE) - (headFiles->size % BLOCK_SIZE != 0) - (headFiles->size == 0);
            for(int i = 0; i < N_BLOCKS && moreBlocks > 0; i++)
                if(!getBit(i)) moreBlocks--;
            if(moreBlocks > 0){
                printf("Não há espaço para copiar o arquivo %s\n", origin->tok);
                return 0;
            }
            // Erase the old file
            removeFile(destiny, oldBlock);
            headFiles = getFiles(oldBlock);
            if(headFiles == NULL){
                currIndex = 0;
                blockIndex = oldBlock;
                break;
            }
            currIndex = headFiles->pos.index + METADATA_SIZE;
            blockIndex = headFiles->pos.block;
        }
        headFiles = headFiles->next;
    }
    if(currIndex+METADATA_SIZE >= BLOCK_SIZE) nBlocks++;
    BlocksLL *headBlocks = NULL, *currBlocks = NULL, *newBlocks = NULL;
    for(int i = 0; i < N_BLOCKS; i++){
        if(!getBit(i)){
            newBlocks = (BlocksLL*)malloc(sizeof(BlocksLL));
            newBlocks->index = i;
            newBlocks->next = NULL;
            if(headBlocks == NULL) headBlocks = newBlocks;
            else currBlocks->next = newBlocks;
            currBlocks = newBlocks;
            nBlocks--;
            if(nBlocks == 0) break;
        }
    }
    if(nBlocks > 0){
        printf("Não há espaço para copiar o arquivo %s\n", origin->tok);
        return 0;
    }
    // Insert the file
    int newBlock = createGenericFile(blockIndex, currIndex, destiny->tok, contentSize);
    // Allocate space for the file content
    newBlocks = headBlocks;
    headBlocks = headBlocks->next;
    if(currIndex+METADATA_SIZE >= BLOCK_SIZE)
        headBlocks = headBlocks->next;
    blockIndex = newBlock;
    for(BlocksLL *aux = headBlocks; aux != NULL; aux = aux->next){
        setBit(aux->index, 1);
        FAT[blockIndex] = aux->index;
        blockIndex = aux->index;
    }
    // Write the file content
    fseek(disk, newBlock*BLOCK_SIZE, SEEK_SET);
    fread(dataBuffer, 1, BLOCK_SIZE, disk);
    int k = currIndex = 0;
    obj = fopen(origin->tok, "r");
    for(char ch = fgetc(obj); k < contentSize; ch = fgetc(obj)){
        if(currIndex == BLOCK_SIZE){
            fseek(disk, newBlock*BLOCK_SIZE, SEEK_SET);
            fwrite(dataBuffer, 1, BLOCK_SIZE, disk);
            newBlock = FAT[newBlock];
            fseek(disk, newBlock*BLOCK_SIZE, SEEK_SET);
            fread(dataBuffer, 1, BLOCK_SIZE, disk);
            currIndex = 0;
        }
        dataBuffer[currIndex++] = ch; k++;
    }
    fseek(disk, newBlock*BLOCK_SIZE, SEEK_SET);
    fwrite(dataBuffer, 1, BLOCK_SIZE, disk);

    fclose(obj);
    updateBitmap(); updateFAT();
    while(newBlocks != NULL){
        currBlocks = newBlocks;
        newBlocks = newBlocks->next;
        free(currBlocks);
    }
    return 1;
}
int showFile(TokensLL* path, int blockIndex){
    int currIndex;
    // Check if the path exists
    FileLL *headFiles = getFiles(blockIndex);
    if(path->next != NULL){
        while(headFiles != NULL){
            if(headFiles->size == IS_DIR && strcmp(headFiles->name, path->tok) == 0){
                if(showFile(path->next, headFiles->content))
                    updateTime(headFiles->pos, 0);
                return 0;
            }
            headFiles = headFiles->next;
        }
        printf("Diretório %s não encontrado\n", path->tok);
        return 0;
    }
    // Check if the file exists
    while(headFiles != NULL && !(headFiles->size != IS_DIR && strcmp(headFiles->name, path->tok) == 0))
        headFiles = headFiles->next;
    if(headFiles == NULL){
        printf("Arquivo %s não encontrado\n", path->tok);
        return 0;
    }
    // Show the file
    currIndex = 0;
    blockIndex = headFiles->content;
    fseek(disk, blockIndex*BLOCK_SIZE, SEEK_SET);
    fread(dataBuffer, 1, BLOCK_SIZE, disk);
    for(int i = 0; i < headFiles->size; i++){
        if(currIndex == BLOCK_SIZE){
            blockIndex = FAT[blockIndex];
            fseek(disk, blockIndex*BLOCK_SIZE, SEEK_SET);
            fread(dataBuffer, 1, BLOCK_SIZE, disk);
            currIndex = 0;
        }
        printf("%c", dataBuffer[currIndex++]);
    }
    updateTime(headFiles->pos, 0);
    return 1;
}
int touchFile(TokensLL* path, int blockIndex){
    int currIndex = 0;
    // Check if the path exists
    FileLL *headFiles = getFiles(blockIndex);
    if(path->next != NULL){
        while(headFiles != NULL){
            if(headFiles->size == IS_DIR && strcmp(headFiles->name, path->tok) == 0){
                int aux = touchFile(path->next, headFiles->content);
                if(aux == 1){
                    updateTime(headFiles->pos, 0);
                    updateTime(headFiles->pos, 1);
                }
                else if(aux == 2) updateTime(headFiles->pos, 0);
                return 0;
            }
            headFiles = headFiles->next;
        }
        printf("Diretório %s não encontrado\n", path->tok);
        return 0;
    }
    // Check if the file already exists
    while(headFiles != NULL && !(headFiles->size != IS_DIR && strcmp(headFiles->name, path->tok) == 0)){
        currIndex = headFiles->pos.index + METADATA_SIZE;
        blockIndex = headFiles->pos.block;
        headFiles = headFiles->next;
    }
    if(headFiles != NULL){
        updateTime(headFiles->pos, 0);
        return 2;
    }
    // Check if there's enough space to insert the file
    int nBlocks = 1 + (currIndex+METADATA_SIZE >= BLOCK_SIZE);
    for(int i = 0; i < N_BLOCKS; i++){
        if(!getBit(i)){
            nBlocks--;
            if(nBlocks == 0) break;
        }
    }
    if(nBlocks > 0){
        printf("Não há espaço para criar o arquivo %s\n", path->tok);
        return 0;
    }
    // Create the file
    createGenericFile(blockIndex, currIndex, path->tok, 0);
    updateBitmap(); updateFAT();
    return 1;
}
int removeFile(TokensLL* path, int blockIndex){
    // Check if the path exists
    FileLL *headFiles = getFiles(blockIndex);
    if(path->next != NULL){
        while(headFiles != NULL){
            if(headFiles->size == IS_DIR && strcmp(headFiles->name, path->tok) == 0){
                if(removeFile(path->next, headFiles->content)){
                    updateTime(headFiles->pos, 0);
                    updateTime(headFiles->pos, 1);
                }
                return 0;
            }
            headFiles = headFiles->next;
        }
        printf("Diretório %s não encontrado\n", path->tok);
        return 0;
    }
    // Find file
    while(headFiles != NULL && !(headFiles->size != IS_DIR && strcmp(headFiles->name, path->tok) == 0))
        headFiles = headFiles->next;
    if(headFiles == NULL){
        printf("Arquivo %s não encontrado\n", path->tok);
        return 0;
    }
    // Erase metadata
    Position prevPos, oldPos, newPos;
    newPos = oldPos = prevPos = headFiles->pos;
    for(FileLL *currFiles = headFiles; currFiles != NULL; currFiles = currFiles->next){
        prevPos = oldPos;
        oldPos = currFiles->pos;
    }
    transferBlock(oldPos, newPos);
    resetBlock(oldPos, METADATA_SIZE);
    if(prevPos.block != oldPos.block){
        setBit(oldPos.block, 0);
        FAT[prevPos.block] = -1;
    }
    // Erase content
    newPos.block = headFiles->content;
    newPos.index = 0;
    do {
        resetBlock(newPos, BLOCK_SIZE);
        setBit(newPos.block, 0);
        prevPos.block = newPos.block;
        newPos.block = FAT[prevPos.block];
        FAT[prevPos.block] = -1;
    } while(newPos.block != -1);

    updateBitmap(); updateFAT();
    return 1;
}
int removeDir(TokensLL* path, int blockIndex){
    // Check if the path exists
    FileLL *headFiles = getFiles(blockIndex);
    if(path->next != NULL){
        while(headFiles != NULL){
            if(headFiles->size == IS_DIR && strcmp(headFiles->name, path->tok) == 0){
                if(removeDir(path->next, headFiles->content)){
                    updateTime(headFiles->pos, 0);
                    updateTime(headFiles->pos, 1);
                }
                return 0;
            }
            headFiles = headFiles->next;
        }
        printf("Diretório %s não encontrado\n", path->tok);
        return 0;
    }
    // Find directory
    while(headFiles != NULL && !(headFiles->size == IS_DIR && strcmp(headFiles->name, path->tok) == 0))
        headFiles = headFiles->next;
    if(headFiles == NULL){
        printf("Diretório %s não encontrado\n", path->tok);
        return 0;
    }
    // Erase metadata
    Position prevPos, oldPos, newPos;
    newPos = oldPos = prevPos = headFiles->pos;
    for(FileLL *currFiles = headFiles; currFiles != NULL; currFiles = currFiles->next){
        prevPos = oldPos;
        oldPos = currFiles->pos;
    }
    transferBlock(oldPos, newPos);
    resetBlock(oldPos, METADATA_SIZE);
    if(prevPos.block != oldPos.block){
        setBit(oldPos.block, 0);
        FAT[prevPos.block] = -1;
    }
    // Erase its content
    FileLL *dirFiles = getFiles(headFiles->content);
    while(dirFiles != NULL){
        TokensLL *name = (TokensLL*)malloc(sizeof(TokensLL));
        name->tok = (char*)malloc((strlen(dirFiles->name)+1)*sizeof(char));
        strcpy(name->tok, dirFiles->name);
        name->next = NULL;

        if(dirFiles->size == IS_DIR){
            removeDir(name, headFiles->content);
            printf("Diretório %s removido\n", name->tok);
        }
        else{    
            removeFile(name, headFiles->content);
            printf("Arquivo %s removido\n", name->tok);
        }
        dirFiles = dirFiles->next;
        free(name->tok);
        free(name);
    }
    setBit(headFiles->content, 0);
    updateBitmap(); updateFAT();
    return 1;
}
DBNode* updateDB(char* name, int blockIndex){
    FileLL *headFiles = getFiles(blockIndex);
    int nChildren = 0;
    for(FileLL *currFiles = headFiles; currFiles != NULL; currFiles = currFiles->next)
        nChildren++;
    
    DBNode *node = (DBNode*)malloc(sizeof(DBNode));
    node->name = (char*)malloc((strlen(name)+1)*sizeof(char));
    strcpy(node->name, name);
    node->nChildren = nChildren;
    node->size = IS_DIR;
    if(nChildren == 0) return node;

    node->children = (DBNode**)malloc(nChildren*sizeof(DBNode*));
    for(int i = 0; i < nChildren; i++){
        if(headFiles->size == IS_DIR)
            node->children[i] = updateDB(headFiles->name, headFiles->content);
        else{
            node->children[i] = (DBNode*)malloc(sizeof(DBNode));
            node->children[i]->name = (char*)malloc((strlen(headFiles->name)+1)*sizeof(char));
            strcpy(node->children[i]->name, headFiles->name);
            node->children[i]->size = headFiles->size;
            node->children[i]->children = NULL;
            node->children[i]->nChildren = 0;
        }
        headFiles = headFiles->next;
    }
    return node;
}
void searchDB(TokensLL *path, char* name, DBNode *node){
    // Complete the current path
    TokensLL *newPath = (TokensLL*)malloc(sizeof(TokensLL));
    newPath->tok = (char*)malloc((strlen(node->name)+1)*sizeof(char));
    newPath->next = NULL;
    strcpy(newPath->tok, node->name);
    if(path == NULL)
        path = newPath;
    else{
        TokensLL *aux = path;
        while(aux->next != NULL) aux = aux->next;
        aux->next = newPath;
    }
    // Check if the name is contained
    if(strstr(node->name, name) != NULL){
        TokensLL *aux = path;
        while(aux != NULL){
            printf("/%s", aux->tok);
            aux = aux->next;
        }
        if(node->size == IS_DIR) printf("/");
        printf("\n");
    }
    // Search in the children
    for(int i = 0; i < node->nChildren; i++)
        searchDB(path, name, node->children[i]);
}
void status(){
    int nFiles = 0, nDirs = 0, nFreeSpace = 0;
    long int wastedSpace = 0, freeSpace;
    DBNode *toCount = updateDB("", 1);
    DBCounter(toCount, &nFiles, &nDirs, &wastedSpace);
    DBDestroyer(toCount);
    printf("Quantidade de arquivos: %d\n", nFiles);
    printf("Quantidade de diretórios: %d\n", nDirs);
    for(int i = 0; i < N_BLOCKS; i++)
        if(!getBit(i)) nFreeSpace++;
    freeSpace = nFreeSpace * BLOCK_SIZE;
    printf("Espaço livre: %ld B (%d blocos)\n", freeSpace, nFreeSpace);
    printf("Espaço desperdiçado: %ld B\n", wastedSpace);
}

int main(){
    TokensLL *input;
    char *notParsed;

    while(1){
        notParsed = readline("{ep3}: ");
        if(notParsed == NULL || *notParsed == '\0') continue;
        input = spliter(notParsed, " ");
        if(strcmp(input->tok, "sai") == 0) break;
        else if(strcmp(input->tok, "monta") == 0) mount(input->next->tok);
        else if(strcmp(input->tok, "busca") == 0){
            if(root != NULL)
                for(int i = 0; i < root->nChildren; i++)
                    searchDB(NULL, input->next->tok, root->children[i]);
        }
        if(disk == NULL) continue;
        else if(strcmp(input->tok, "desmonta") == 0) dismount();
        else if(strcmp(input->tok, "criadir") == 0) createDir(spliter(input->next->tok, "/"), 1, 1);
        else if(strcmp(input->tok, "lista") == 0) listDir(spliter(input->next->tok, "/"), 1);
        else if(strcmp(input->tok, "copia") == 0) copyFile(input->next, spliter(input->next->next->tok, "/"), 1);
        else if(strcmp(input->tok, "mostra") == 0) showFile(spliter(input->next->tok, "/"), 1);
        else if(strcmp(input->tok, "toca") == 0) touchFile(spliter(input->next->tok, "/"), 1);
        else if(strcmp(input->tok, "apaga") == 0) removeFile(spliter(input->next->tok, "/"), 1);
        else if(strcmp(input->tok, "apagadir") == 0) removeDir(spliter(input->next->tok, "/"), 1);
        else if(strcmp(input->tok, "atualizadb") == 0){
            if(root != NULL) 
                DBDestroyer(root);
            root = updateDB("", 1);
        }
        else if(strcmp(input->tok, "status") == 0) status();
    }
    if(disk != NULL) dismount();
    
    return 0;
}