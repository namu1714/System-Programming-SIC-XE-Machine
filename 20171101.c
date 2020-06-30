#include "20171101.h"

int main(){
	char s[CMD_MAX_SIZE]; //입력받을 커맨드
	char s_copy[CMD_MAX_SIZE];
	char* token;
	int start, end, value, address;

	//initialization
	len = 0, last_addr = 0xFFFFF;
	progaddr = 0x00, PC = 0x00, end_addr = 0x00;
	bp_size = 0;
	memset(regi_arr, 0, sizeof(regi_arr));
	memset(memory, 0 , sizeof(memory));

	if(MakeHashTable() == -1) return 0; //opcode.txt파일 없을 경우 종료
	FILE *f_sym = fopen("symbol.txt", "w"); //symbol table 파일내용 있으면 지우기
	fclose(f_sym);
	
	//init symbol table
	for(int i=0; i<26; i++)
		symbolTable[i] = NULL;

	//init external symbol table
	for(int i=0; i<ESTAB_SIZE; i++)
		ESTAB[i] = NULL;
	
	while(1){
		//명령어 입력
		arg_flag = 0;
		printf("sicsim> ");
		fgets(s, CMD_MAX_SIZE, stdin);
		s[strlen(s)-1] = '\0';
		
		//string line을 arg로 나누기
		strcpy(s_copy, s);
		token = strtok(s_copy, " ");
		if(token == NULL) continue; //공백
		strcpy(c, token); //command
		c[strlen(token)] = '\0';
		arg_flag++;
	
		//args 생성
		for(int i=0;i<MAX_ARGS+1;i++){
			token = strtok(NULL, ", ");
			if(token == NULL) break;
			strcpy(args[i], token);
			args[i][strlen(token)] = '\0';
			arg_flag++;
		}
			
		//sicsim 종료	
		if((!strcmp(c,"q") || !strcmp(c,"quit")) && arg_flag==1)
			break;

		//명령어 리스트 출력
		else if((!strcmp(c, "h") || !strcmp(c, "help")) && arg_flag==1){
			AddHistory(s);
			printf("h[elp]\n");
			printf("d[ir]\n");
			printf("q[uit]\n");
			printf("hi[story]\n");
			printf("du[mp] [start, end]\n");
			printf("e[dit] address, value\n");
			printf("f[ill] start, end, value\n");
			printf("reset\n");
			printf("opcode mnemonic\n");
			printf("opcodelist\n");
			printf("assemble filename\n");
			printf("type filename\n");
			printf("symbol\n");
			printf("progaddr address\n");
			printf("loader filenames\n");
			printf("bp\n");
			printf("bp clear\n");
			printf("bp address\n");
			printf("run\n");
		}
		//디렉터리 파일 목록 출력
		else if((!strcmp(c, "d") || !strcmp(c, "dir")) && arg_flag==1){
			AddHistory(s);
			Directory();
		}
		//file을 디렉터리에서 읽어서 출력
		else if(!strcmp(c, "type")){
			if(arg_flag != 2){
				printf("Usage : type filename\n");
				continue;
			}
			//파일 내용 출력에 성공한 경우
			if(PrintFile(args[0])!=-1){
				AddHistory(s);
			}
			else{
				printf("file open failed\n");
			}
		}
		//사용한 명령어 목록 출력
		else if((!strcmp(c, "hi") || !strcmp(c, "history")) && arg_flag==1){
			AddHistory(s);
			PrintHistory();
		}
		
		//dump
		else if(!strcmp(c, "du") || !strcmp(c, "dump")){ //du로 시작
			if(arg_flag == 1){ //명령어 'dump'일 경우
				AddHistory(s);
				last_addr = Dump(last_addr);
			}
			else if(arg_flag == 2){ //명령어 'dump start'일 경우
				start = strtol(args[0], NULL, 16);//16진수 변환
				if(start>0xFFFFF || start<0x00000){
					RangeError();
					continue;
				}
				AddHistory(s);
				last_addr = DumpStart(start);
			}
			else if(arg_flag == 3){ //명령어 'dump start, end'일 경우
				start = strtol(args[0], NULL, 16);
				end = strtol(args[1], NULL, 16);
				if(start>end || end>0xFFFFF){
					RangeError();
					continue;
				}
				AddHistory(s);
				last_addr = DumpStartEnd(start, end);
			}
		}
		//edit
		else if(!strcmp(c, "e") || !strcmp(c, "edit")) {
			if(arg_flag != 3){ //arg 개수가 다를 때
				printf("Usage : e[dit] address, value\n");
				continue;
			}
			//address 추출
			address = strtol(args[0], NULL, 16);
			if(address>0xFFFFF || address<0x00000){
				RangeError();
				continue;
			}
			//value 추출
			value = strtol(args[1], NULL, 16);
			if(value>0xFF || value<0x00){
				ValueError();
				continue;
			}
			AddHistory(s);
			Edit(address, value);
		}
		//fill
		else if(!strcmp(c, "f") || !strcmp(c, "fill")){
			if(arg_flag != 4){ //arg 개수가 다를 때
				printf("Usage : f[ill] start, end, value\n");
				continue;
			}
			//start, end 값 추출
			start = strtol(args[0], NULL, 16);
			end = strtol(args[1], NULL, 16);
			//start, end 범위가 잘못되었을 경우
			if(start>0xFFFFF || start<0x00000 || end>0xFFFFF || end<0x00000 || start>end){
				RangeError();
				continue;
			} 
			
			//value 추출
			value = strtol(args[2], NULL, 16);
			if(value>0xFF || value<0x00){
				ValueError();
				continue;
			}
			AddHistory(s);
			Fill(start, end, value);
		}
		//opcode list 출력
		else if(!strcmp(c, "opcodelist") && arg_flag==1){
			AddHistory(s);
			PrintOpcode();
		}
		//입력받은 mnemonic에 알맞은 opcode 출력
		else if(!strcmp(c, "opcode")){
			if(arg_flag != 2){
				printf("Usage : opcode mnemonic\n");
				continue;			
			}
			hash_pointer ptr = FindOpcode(args[0]);
			if(ptr != NULL){ //유효한 opcode를 찾았을 때만
				printf("opcode is %X\n", ptr->opcode);
				AddHistory(s); //history에 추가
			}
			else{
				printf("can't find opcode\n");
			}
		}
		//memory space reset
		else if(!strcmp(c, "reset") && arg_flag==1){
			AddHistory(s);
			memset(memory, 0, sizeof(memory));
		}
		//file assemble
		else if(!strcmp(c, "assemble")){
			if(arg_flag != 2){
				printf("Usage : assemble filename\n");
				continue;
			}
			if(Assemble(args[0]) == 1){
				AddHistory(s);
			}
			else{ //lst, obj 파일 삭제
				int file_len = strlen(args[0]);
				char lst_file[30] = {0, }, obj_file[30] = {0, };
				strncpy(lst_file, args[0], file_len-3);
				strncpy(obj_file, args[0], file_len-3);
				strcat(lst_file, "lst");
				strcat(obj_file, "obj");
				remove(lst_file);
				remove(obj_file);
			}
		}
		//print symbol table 
		else if(!strcmp(c, "symbol") && arg_flag==1){
			PrintFile("symbol.txt");
			AddHistory(s);
		}
		//progaddr 지정
		else if(!strcmp(c, "progaddr")){
			if(arg_flag != 2){
				printf("Usage : progaddr address\n");
				continue;
			}
			progaddr = strtol(args[0], NULL, 16);
			if(progaddr>=0 && progaddr<0xFFFFF){
				PC = progaddr;
				AddHistory(s);
			}
			else{
				RangeError();
				continue;
			}
		}
		//linking loader
		else if(!strcmp(c, "loader")){
			if(LinkingLoader(args, arg_flag-1) == 1){
				AddHistory(s);
			}
		}
		//run
		else if(!strcmp(c, "run") && arg_flag==1){
			if(Run()==1){
				AddHistory(s);
			}
		}
		//break point
		else if(!strcmp(c, "bp")){
			if(arg_flag == 1){ //bp 출력
				PrintBreakPoint();
				AddHistory(s);
			}
			else if(arg_flag == 2){
				if(!strcmp(args[0], "clear")){ //bp clear
					ClearBreakPoint();
					AddHistory(s);
				}
				else { //bp 지정
					int new_bp = strtol(args[0], NULL, 16);
					if(AddBreakPoint(new_bp) == 1){ //success
						AddHistory(s);
					}
				}
			}
			else{ //error
				printf("Usage : bp [][clear][address]\n");
			}
		}
		//other command
		else{
			printf("invalid command : refer to help\n");
		}
	}
	FreeMemory();
	InitAssemble();

	return 0;
}

void AddHistory(char* cmd){
	//linked list node 생성
	if(len==0){ //node 처음 생성시
		first = (node_pointer)malloc(sizeof(node));
		strcpy(first->cmd, cmd);
		first->num = ++len;
		first->link = NULL;
		last = first;
	}
	else{
		node_pointer tmp = (node_pointer)malloc(sizeof(node));
		strcpy(tmp->cmd, cmd);
		tmp->num = ++len;
		tmp->link = NULL;
		last->link = tmp;
		last = tmp;
	}
	return;
} 

void PrintHistory(){
	//차례대로 node 출력
	node_pointer tmp = first;
	for(int i=0; i<len; i++){
		printf("%-4d %s\n", tmp->num, tmp->cmd);
		tmp = tmp->link; 
	}
	return;
}

void Directory(){
	DIR *dir;
	struct dirent *direntp;
	struct stat st;
	char path[BUFSIZ];

	dir = opendir("."); //현재 폴더
	
	while((direntp = readdir(dir))){
		//현재 directory에 있는 파일 탐색
		if (direntp->d_name[0] == '.') continue;
		sprintf(path, "%s/%s", ".",direntp->d_name);
		stat(path, &st);
		printf("%s", direntp->d_name);

		if(S_ISDIR(st.st_mode)) printf("/"); //directory
		else if(st.st_mode & S_IXUSR) printf("*"); //실행파일
		
		printf("\n");
	}
	closedir(dir); //close directory
}

int PrintFile(char* filename){
	FILE *fp = fopen(filename, "r");
	char ch;
	if(fp == NULL){
		return -1;
	}
	//한글자씩 읽어서 출력
	while((ch=fgetc(fp))!=EOF){
		putchar(ch);
	}
	fclose(fp);
	return 1;
}

int MakeHashTable(){
	int op, hashcode;
	hash_pointer tmp;
	char arg1[MNEMONIC_MAX], arg2[MNEMONIC_MAX];
	//initialization
	for(int i=0 ;i<HASH_TABLE_SIZE; i++)
		hashTable[i] = NULL;

	//open opcode.txt file
	FILE *fp = fopen("opcode.txt", "r");
	if(!fp){
		printf("failed to read opcode.txt file");
		return -1;
	}
	//node 생성해 hashtable에 넣기
	while(fscanf(fp, "%x %s %s", &op, arg1, arg2) != EOF){
		hashcode = HashFunction(arg1, HASH_TABLE_SIZE); //해시함수 호출
		tmp = (hash_pointer)malloc(sizeof(hash));
		strcpy(tmp->mnemonic, arg1);
		strcpy(tmp->format, arg2);
		tmp->opcode = op;
		tmp->link = hashTable[hashcode];
		hashTable[hashcode] = tmp;
	}
	return 1;
}

int HashFunction(char* s, int n){
	int len = strlen(s);
	int sum = 0;
	//string의 각각 charcter의 아스키코드값을 더함
	for(int i=0; i<len; i++)
		sum+=s[i];
	return sum%n;
}

void PrintOpcode(){
	//hashtable 출력
	hash_pointer ptr;
	for(int i=0; i<HASH_TABLE_SIZE; i++){
		ptr = hashTable[i];
		printf("%d : ", i);
		for(; ptr; ptr=ptr->link){
			printf("[%s,%2X]", ptr->mnemonic, ptr->opcode);
			if(ptr->link != NULL){
				printf(" -> ");
			}
		}
		printf("\n");
	}	
}

hash_pointer FindOpcode(char* m){
	int flag=0;//hashtable에서 mnemonic 찾으면 1로 바뀜
	int hashcode = HashFunction(m, HASH_TABLE_SIZE);
	hash_pointer ptr = hashTable[hashcode];
	for(; ptr; ptr=ptr->link){
		if(!strcmp(m, ptr->mnemonic)){
			flag = 1;
			break;
		}
	}
	if(flag == 0){ //mnemonic을 잘못 입력받았을 경우
		return NULL;
	}
	else{ //success
		return ptr;
	}
}

void FreeMemory(){
	//메모리 해제
	node_pointer node_next, node_cur = first;
	hash_pointer hash_next, hash_cur;

	for(int i=0; i<len; i++){
		node_next = node_cur->link;
		free(node_cur);
		node_cur = node_next;
	}
	
	for(int i=0; i<HASH_TABLE_SIZE; i++){
		hash_cur = hashTable[i];
		while(hash_cur!=NULL){
			hash_next = hash_cur->link;
			free(hash_cur);
			hash_cur = hash_next;
		}
	}
	free(modify_record);
}

void InitAssemble() {
	//initialize symbol table
	symbol_pointer next, cur;
	for (int i = 0; i < 26; i++) {
		cur = symbolTable[i];
		while (cur != NULL) {
			next = cur->link;
			free(cur);
			cur = next;
		}
		symbolTable[i] = NULL;
	}
	
	//initialize text record
	record_pointer rc_cur = rc_first, rc_next;
	if (rc_cur != NULL) {
		rc_next = rc_cur->link;
		free(rc_cur);
		rc_cur = rc_next;
	}
	rc_first = NULL;
}

int Dump(int last_addr){
	int start, end;
	if(last_addr == 0xFFFFF)//0xFFFFF가 마지막 adress일때 
		start = 0;//0부터 다시 시작
	else 
		start = last_addr+1;
	
	end = (start+159);//10라인 출력
	if(end > 0xFFFFF) end = 0xFFFFF;//주소를 넘어갈 경우 0xFFFF까지
	return DumpStartEnd(start, end);
}

int DumpStart(int start){
	int end;
	end = start+159;//10라인 출력
	if(end> 0xFFFFF)
		end = 0xFFFFF;

	return DumpStartEnd(start, end);
}

int DumpStartEnd(int start, int end)
{
	int startline, endline;
	//라인의 시작과 끝지점 지정
	startline = start/16*16;
	endline = end/16*16;
	
	for(int i=startline; i<=endline; i+=16){
		printf("%05X ", i);
		
		//메모리번지 16진수로 출력
		for(int j=i; j<i+16; j++){
			if(j<start || j>end)
				printf("   ");
			else
				printf("%02X ", memory[j]);
		}
		printf("; ");
		
		//아스키코드값을 char로 변환해 출력
		for(int j=i; j<i+16; j++){
			if(j<start || j>end) //j가 범위 밖인 경우
				printf("."); //'.' 출력
			else if(memory[j]<32 || memory[j]>127)
				printf(".");
			else
				printf("%c", memory[j]);
		}
		printf("\n");
	}
	return end;
}

void Edit(int address, int value){
	//address번지 값을 value에 지정된 값으로 변경
	memory[address] = value;
}

void Fill(int start, int end, int value){
	//start부터 end번지까지 값을 value에 지정된 값으로 변경
	for(int i=start; i<=end; i++){
		memory[i] = value;
	}
}

void RangeError(){
	printf("Address Range Error: 00000~FFFFF / start<end\n");	
}
void ValueError(){
	printf("Value Range Error: 00~FF\n");
}
