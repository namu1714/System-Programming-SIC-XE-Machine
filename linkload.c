#include "20171101.h"

int LinkingLoader(char args[][ARG_MAX_LEN], int file_num){
	int cslth; //control section length
	int addr, text_len, value, refernum;
	int mdf_len, refaddr;
	int byte, line;
	char buf1[80], buf2[20], tmp[20];
	char* token;
	int referArr[10]; //reference number
	char op;
	InitLinkingLoader(); //initialization

	//pass1
	csaddr = progaddr;
	for(int csnum=0;csnum<file_num;csnum++){
		FILE* fp = fopen(args[csnum], "r");
		if(!fp){
			printf("file open error\n");
			return -1;
		}
		line = 1;		
		//read header record
		fscanf(fp, "%s", buf1);
		fscanf(fp, "%s", buf2);

		//search estab for control section name
		if(FindExternal(buf1+1) != NULL){ //external symbol exist
			printf("loader 1 error: duplicate external symbol\n");
			printf("%s(line: %d)[%s]\n", args[csnum], line, buf1+1);
			printf("*%s\n", buf1);
			return -1;
		}
		cslth = strtol(buf2+6, NULL, 16);
		InsertExternal(buf1+1, csaddr, cslth, 'c'); //insert control section into ESTAB 
		
		fscanf(fp, "%s", buf1);
		while(buf1[0]!='E'){ //until end
			line++;
			//external symbol definition
			if(buf1[0]=='D'){ 
				strcpy(buf1, buf1+1);
				while(1){ //D record line 입력받아 처리
					fscanf(fp, "%s", buf2);
					strncpy(tmp, buf2, 6); //length 문자열
					tmp[6] = '\0';
					addr = csaddr + strtol(tmp, NULL, 16);
					//search estab for symbol
					if(FindExternal(buf1) != NULL){
						printf("loader 1 error: duplicate exernal symbol\n");
						printf("%s(line: %d)[%s]\n", args[csnum], line, buf1);
						return -1;
					}
					//insert symbol into ESTAB
					InsertExternal(buf1, addr, 0, 's');
					if(buf2[6]=='\0') break;
					strcpy(buf1, buf2+6);
				}
			}		
			fscanf(fp, "%s", buf1);			
		}
		fclose(fp);
		csaddr += cslth; //start addr for next control section
	}

	//pass2
	char c;
	csaddr = progaddr;
	execaddr = progaddr;
	for(int csnum=0; csnum<file_num; csnum++){
		//referArr 초기화
		for(int i=0;i<10;i++){
			referArr[i] = 0;
		}
			
		FILE* fp = fopen(args[csnum], "r");
		line = 1;
		//read header record
		fscanf(fp, "%s", buf1);
		fscanf(fp, "%s\n", buf2);
		referArr[1] = FindExternal(buf1+1)->address;
		cslth = strtol(buf2+6, NULL, 16);
		
		c = fgetc(fp);
		while(c!='E'){
			line++;
			//external reference
			if(c=='R'){
				fgets(buf1, 80, fp);
				buf1[strlen(buf1)-1] = '\0';
				token = strtok(buf1, " ");
				//referArr에 reference number 저장
				while(token != NULL){
					strncpy(tmp, token, 2);
					tmp[2] = '\0';
					refernum = strtol(tmp, NULL, 16);
					referArr[refernum] = FindExternal(token+2)->address;
					token = strtok(NULL, " ");
				}
			}
			//text record
			else if(c=='T'){
				fgets(buf1, 80, fp);
				//get start address
				strncpy(tmp, buf1, 6);
				tmp[6] = '\0';
				addr = csaddr + strtol(tmp, NULL, 16);
				//get length
				strncpy(tmp, buf1+6, 2);
				tmp[2] = '\0';
				text_len = strtol(tmp, NULL, 16);

				//move to location
				strcpy(buf1, buf1+8);
				for(int loc=0; loc<text_len; loc++){
					strncpy(tmp, buf1+loc*2, 2);
					tmp[2] = '\0';
					value = strtol(tmp, NULL, 16);
					Edit(addr+loc, value);
				}			
			}
			//modification record
			else if(c=='M'){
				fgets(buf1, 80, fp);
				sscanf(buf1, "%6X%2X%1c%2X", &addr, &mdf_len, &op, &refernum);
				addr += csaddr;
				//get external symbol address
				refaddr = referArr[refernum]; 
				if(refaddr == 0){ //error
					printf("loader 2 error: undefined external symbol\n");
					printf("%s(line: %d)\n", args[csnum], line);
					return -1;
				}	
				byte = (op=='+'? refaddr : -refaddr);
				if(mdf_len==6){ //address length to be modified 6
					byte = byte+(memory[addr]<<16)+(memory[addr+1]<<8)+memory[addr+2];
					if(byte>0xFFFFFF) byte -= 0x1000000;
					if(byte<0x000000) byte += 0x1000000;
					Edit(addr+2, byte&0x0000FF);
					byte = byte>>8;
					Edit(addr+1, byte&0x00FF);
					byte = byte>>8;
					Edit(addr, byte); //로딩된 메모리주소 수정
				}
				else if(mdf_len==5){ //length 5
					byte = byte+((memory[addr]&0x0F)<<16)+(memory[addr+1]<<8)+memory[addr+2];
					if(byte>0xFFFFF) byte -= 0x100000;
					if(byte<0x00000) byte += 0x100000;
					Edit(addr+2, byte&0x0000FF);
					byte = byte >> 8;	
					Edit(addr+1, byte&0x00FF);
					byte = (byte >> 8) + (memory[addr]&0xF0);
					Edit(addr, byte);
				}
				
			}
			else{
				fgets(buf1, 80, fp); //other input
			}
			c = fgetc(fp);
		} 
		fclose(fp);
		csaddr += cslth; //start address of next control section
	}
	PrintExternal();

	//initialize for run 
	memset(regi_arr, 0, sizeof(regi_arr));
	regi_arr[2] = prog_total_len; //L register 프로그램 길이로 초기화
	PC = progaddr;
	end_addr = progaddr + prog_total_len;

	return 1;
}

void InsertExternal(char* s, int addr, int len, char type){
	int hashcode = HashFunction(s, ESTAB_SIZE); //find hashcode
	//insert into ESTAB
	estab_pointer tmp = (estab_pointer)malloc(sizeof(ex_node));
	strcpy(tmp->name, s);
	tmp->address = addr;
	tmp->length = len;
	tmp->type = type;
	tmp->link = ESTAB[hashcode];
	ESTAB[hashcode] = tmp;
}

estab_pointer FindExternal(char* s){
	int flag = 0; //find flag
	int hashcode = HashFunction(s, ESTAB_SIZE);
	estab_pointer ptr = ESTAB[hashcode];
	//search ESTAB
	for(; ptr; ptr=ptr->link){
		if(!strcmp(s, ptr->name)){
			flag = 1;
			break;
		}
	}
	if(flag == 0)
		return NULL;
	else
		return ptr;
}

void InitLinkingLoader(){
	//initialize external symbol table
	estab_pointer next, cur;
	for(int i=0; i<ESTAB_SIZE; i++){
		cur = ESTAB[i];
		while(cur!=NULL){
			next = cur->link;
			free(cur);
			cur = next;
		}
		ESTAB[i] = NULL;
	}
}

void PrintExternal(){
	prog_total_len = 0;
	estab_pointer ptr;
	printf("control symbol address length\n");
	printf("section name\n");
	printf("-----------------------------\n");
	for(int i=0; i<ESTAB_SIZE; i++){
		ptr = ESTAB[i];
		for(; ptr; ptr=ptr->link){
			//control section
			if(ptr->type=='c'){
				printf("%s\t\t%04X   %04X\n", ptr->name, ptr->address, ptr->length);
				prog_total_len += ptr->length;
			}
			//symbol
			else if(ptr->type=='s'){
				printf("\t%s\t%04X\n", ptr->name, ptr->address);
			}
		}
	}
	printf("-----------------------------\n");
	printf("\t  total length %04X\n", prog_total_len);
}
