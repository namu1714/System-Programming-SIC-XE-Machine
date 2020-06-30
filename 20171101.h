#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ctype.h>

#define CMD_MAX_SIZE 50
#define MEMORY_SIZE (1<<20) //1Mbyte 메모리공간
#define HASH_TABLE_SIZE 20
#define MNEMONIC_MAX 10
#define SYMBOL_MAX 10
#define MAX_ARGS 3
#define OBJ_LINE_MAX 30
#define ESTAB_SIZE 10
#define ARG_MAX_LEN 15

//command
int arg_flag;
char c[20];
char args[MAX_ARGS+1][ARG_MAX_LEN];

//명령어 history 만드는 node
typedef struct _node* node_pointer;
typedef struct _node{
	int num;
	char cmd[CMD_MAX_SIZE];
	node_pointer link;
}node;
node_pointer first, last;
int len; //저장된 history command 개수

unsigned char memory[MEMORY_SIZE]; //memory 
int last_addr; //dump에서 출력한 마지막 메모리 번지

//hashtable
typedef struct _hash* hash_pointer;
typedef struct _hash{
	char mnemonic[MNEMONIC_MAX];
	char format[MNEMONIC_MAX];
	int opcode;
	hash_pointer link;
}hash;
hash_pointer hashTable[HASH_TABLE_SIZE];

//symboltable
typedef struct _symbol* symbol_pointer;
typedef struct _symbol{
	char symbol[SYMBOL_MAX];
	int line;
	symbol_pointer link;
}symbol_node;
symbol_pointer symbolTable[26];

//assemble record
typedef struct t_record* record_pointer;
typedef struct t_record {
	int address;
	int len;
	char text[100];
	record_pointer link;
}t_record;
record_pointer rc_first, rc_last;

//external symbol record
typedef struct ex_node* estab_pointer;
typedef struct ex_node{
	char name[10];
	int address;
	int length;
	char type;
	estab_pointer link;
}ex_node;
estab_pointer ESTAB[ESTAB_SIZE];

//modification record
int* modify_record;
int modify_size;

//break point
int* break_point;
int bp_size;

/*--------------------------------------*/
int start_adr; //program start address
int base; //base register에 저장된 주소값
int prog_total_len; //프로그램 총 길이
int end_addr; //프로그램 마지막 address
int PC, CC;
int progaddr, csaddr, execaddr;

int regi_arr[6]; //A,X,L,B,S,T

/* 
 * function : RangeError
 * print error message if memory address is out of range
 * input    : none
 * return   : none 
*/
void RangeError();

/*
 * function : ValueError
 * print error message if value is out of range
 * input    : none
 * return   : none
*/
void ValueError();

/*
 * function : AddHistory
 * command를 history list에 추가
 * input    : (char*) proper command
 * return   : none
*/
void AddHistory(char* cmd);

/*
 * function : PrintHistory
 * 여태까지 사용한 command list 출력
 * input    : none
 * return   : none
*/
void PrintHistory();

/*
 * function : Directory
 * 현재 디렉터리에 있는 파일 목록 출력
 * input    : none
 * return   : none
*/
void Directory();

/*
 * function : Dump
 * 마지막 address+1 번지부터 10라인의 메모리 내용을 출력
 * input    : (int)마지막 memory address
 * return   : (int)출력 후 마지막 memory address
*/
int Dump(int last_addr);

/*
 * function : DumpStart
 * start번지부터 10라인의 메모리 내용을 출력
 * input    : (int)start address
 * return   : (int)출력 후 마지막 memory address
*/
int DumpStart(int start);

/*
 * function : DumpStartEnd
 * start번지부터 end번지까지 메모리 내용을 출력
 * input    : (int)start address
			  (int)end address
 * return   : (int)end address
*/
int DumpStartEnd(int start, int end);

/*
 * function : Edit
 * address 번지의 값을 value의 지정된 값으로 변경
 * input    : (int)address
			  (int)value
 * return   : none
*/
void Edit(int address, int value);

/*
 * function : Fill
 * start번지부터 end번지까지의 값을 value에 지정된 값으로 변경
 * input    : (int)start address
			  (int)end address
			  (int)value
 * return   : none
*/
void Fill(int start, int end, int value);

/*
 * function : PrintOpcode
 * print opcode hashtable list
 * input    : none
 * return   : none
*/
void PrintOpcode();

/*
 * function : FindOpcode
 * hashtable에서 mnemonic의 opcode를 찾아 반환
 * input    : (char*)mnemonic
 * return   : (hash_pointer)해당 mnemonic를 가리키는 pointer (NULL:failure)
*/
hash_pointer FindOpcode(char* m);

/*
 * function : HashFuction
 * string을 받아 해시값을 계산해 리턴
 * input    : (char*)key string
			  (int)size of hashtable
 * return   : (int)hash
*/
int HashFunction(char* s, int n);

/*
 * function : MakeHashTable
 * opcode.txt 파일을 읽어 hashtable 생성
 * input    : (int)(1:success, -1:failure(opcode.txt 파일 읽을 수 없음))
 * return   : none
*/
int MakeHashTable();

/*
 * function : FreeMemory
 * input    : none
 * return   : none
*/
void FreeMemory();

/*
 * function : InitAssemble
 * assemble function에 필요한 변수들을 초기화
 * input    : none
 * return   : none
*/
void InitAssemble();

/*
 * function : FindSymbol
 * symbol table에서 해당 symbol location을 찾아 반환
 * input    : (char*)symbol
 * return   : (int)symbol location (-1:failure)
*/
int FindSymbol(char* s);

/*
 * function : PrintFile
 * 파일 내용을 출력
 * input    : (char* filename)
 * return   : (int)(1:success, -1:failure)
*/
int PrintFile(char* filename);

/*
 * function : InsertSymbol
 * symbol table에 해당 symbol을 넣음
 * input    : (char*)symbol, (int)line
 * return   : none
*/
void InsertSymbol(char* s, int line);  

/*
 * function : Assemble
 * assemble 과정을 거친 후 리스트 파일과 오브젝트 파일 생성
 * input    : (char*)filename
 * return   : (int)(1:success, -1:error)
*/
int Assemble(char* filename);

/*
 * function : GetRegiNum
 * 입력받은 레지스터의 number 반환
 * input    : (char*)register
 * return   : (int)register number (-1:failure)
*/
int GetRegiNum(char* regi);

/*
 * function : LinkingLoader
 * object 파일 링킹 후 메모리에 로드
 * input    : (char)args[][ARG_MAX_LEN], (int)file_num
 * return   : (int)(1:success, -1:error)
*/
int LinkingLoader(char args[][ARG_MAX_LEN], int file_num);

/*
 * function : FindExternal
 * ESTAB에서 일치하는 external symbol를 찾아 반환
 * input    : (char*)symbol name
 * return   : (estab_pointer)해당 symbol을 가리키는 pointer (NULL:failure)
*/
estab_pointer FindExternal(char* s);

/*
 * function : PrintExternal
 * print external symbol list
 * input    : none
 * return   : none
*/
void PrintExternal();

/*
 * function : InsertExternal
 * Insert external symbol or control section into ESTAB
 * input    : (char*)name, (int)address, (int)length, (char)type
 * return   : none
*/
void InsertExternal(char* s, int addr, int len, char type); 

/*
 * function : InitLinkingLoader
 * Linking Loader에 필요한 자료구조들을 초기화
 * input    : none
 * return   : none
*/
void InitLinkingLoader();

/*
 * function : Run
 * 메모리에 올라간 프로그램 실행
 * input    : none
 * return   : (int)(1:success, -1:error)
*/
int Run();

/*
 * function : PrintBreakPoint
 * break point 목록 출력
 * input    : none
 * return   : none
*/
void PrintBreakPoint();

/*
 * function : ClearBreakPoint
 * 설정한 break point를 초기화
 * input    : none
 * return   : none
*/
void ClearBreakPoint();

/*
 * function : AddBreakPoint
 * 해당 메모리 위치에 break point 추가
 * input    : (int)bp
 * return   : none
*/
int AddBreakPoint(int bp);

/*
 * function : PrintRegisters
 * print register value
 * input    : none
 * return   : none
*/
void PrintRegisters();
/*
 * function : FindOpcodebyNum
 * opcode 값으로 mnemonic을 가리키는 pointer를 찾아 리턴
 * input    : (int)opcode
 * return   : (hash_pointer)pointer (NULL:failure)
*/
hash_pointer FindOpcodebyNum(int opcode);
