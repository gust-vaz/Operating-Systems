#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <unistd.h>

#include <readline/readline.h>

#define N_BLOCKS 25600          // 100MB = 102400KB; 102400/4 = 25600 blocks of 4KB
#define BLOCK_SIZE 4096         // 4KB is 4096B

#define MAX_FILE_NAME 255
#define METADATA_SIZE 273   // 255(name) + 2(content) + 4(size) + 3*4(time)
#define IS_DIR -1

#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_STYLE_BOLD "\x1b[1m"
#define ANSI_RESET_ALL "\x1b[0m"

typedef struct {
    int year;
    int mon;
    int day;
    int hour;
    int min;
    int sec;
} TimeDetails;
typedef struct {
    int block;
    int index;
} Position;
typedef struct DBNode{
    struct DBNode** children;
    char* name;
    int nChildren;
    long int size;
} DBNode;

typedef struct TokensLL{
    struct TokensLL* next;
    char* tok;
} TokensLL;
typedef struct FileLL{
    struct FileLL* next;
    char* name;             // 255 bytes + '\0'
    int content;            // 2 bytes
    long int size;          // 4 bytes
    long int access;        // 4 bytes
    long int modification;  // 4 bytes
    long int creation;      // 4 bytes
    Position pos;   // block and index where the file metadata is stored
} FileLL;
typedef struct BlocksLL{
    struct BlocksLL* next;
    int index;
} BlocksLL;

// AUXILIARY FUNCTIONS
void setBit(int,int);
int getBit(int);
void updateBitmap();
void updateFAT();
TimeDetails decodifyTime(long int);
TokensLL* spliter(char*,char*);

// MANIPULATION FUNCTIONS
/* Return all the files and directories of the directory that starts in the
 * block. */
FileLL *getFiles(int);
/* Allocate a new block for the file or directory and return its content block.
 * Put the metadata in the right position, just after the last file or directory.
 * It doesn't consider if there is enough space in the disk and it doens't
 * update bitmap or FAT. It doesn't allocate more than 1 block for content.*/
int createGenericFile(int,int,char*,int);
/* Update the time of access(type = 0), modification(type = 1) 
 * or creation(type = 2) of the file metadata. */
void updateTime(Position,int);
/* Copy the file metadata from the old Position to the new Position. */
void transferBlock(Position,Position);
/* Fill the content in the block with 0 at the interval [index, index+size). */
void resetBlock(Position,int);
/* Free in all the nodes of DBNode tree. */
void DBDestroyer(DBNode*);
/* Print the DBNode tree in a similar way to pstree. */
void DBPrinter(DBNode*,int);
/* Count the number of directories, files and wasted space. 
 * Diretório root é contabilizado.*/
void DBCounter(DBNode*,int*,int*,long int*);

// COMMAND FUNCTIONS
void mount(char*);
void dismount();
int createDir(TokensLL*,int,int);
void listDir(TokensLL*,int);
int copyFile(TokensLL*,TokensLL*,int);
int showFile(TokensLL*,int);
int touchFile(TokensLL*,int);
int removeFile(TokensLL*,int);
int removeDir(TokensLL*,int);
DBNode* updateDB(char*,int);
void searchDB(TokensLL*,char*,DBNode*);
void status();