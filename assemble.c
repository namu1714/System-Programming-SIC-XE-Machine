#include "20171101.h"

int Assemble(char* filename) {
	char c, esc;
	char str[100], label[10], mnemonic[10], operand[20], operand2[10];
	char ni[2], xbpe[4];
	int disp, addr, regi1, regi2, op;
	int format, plus, num_flag, new_line_flag = 0, line_len = 0;
	int pc, lc, line, program_len;
	hash_pointer ptr;
	record_pointer rc_tmp;

	//initialization
	InitAssemble();
	modify_size = 0;

	//filename 생성
	int file_len = strlen(filename);
	char lst_file[30] = { 0, }, obj_file[30] = { 0, };
	strncpy(lst_file, filename, file_len - 3);
	strncpy(obj_file, filename, file_len - 3);
	strcat(lst_file, "lst");
	strcat(obj_file, "obj");

	//new file open
	FILE* list = fopen(lst_file, "w");
	FILE* object = fopen(obj_file, "w");

	for (int pass = 1; pass <= 2; pass++) {
		pc = 0, lc = 0, line = 5;

		FILE* fr = fopen(filename, "r");

		if (fr == NULL) { //file open failed
			printf("file open failed\n");
			return -1;
		}

		fscanf(fr, "%s%s%s%c", label, mnemonic, operand, &esc);
		lc = start_adr = atoi(operand);
		if (pass == 2) {
			fprintf(list, "%-4d    %04X    %-6s %-6s %-s\n", line, lc, label, mnemonic, operand);
			fprintf(object, "H%-6s%06X%06X\n", label, lc, program_len); //header record
			rc_first = (record_pointer)malloc(sizeof(t_record));
			rc_first->address = lc;
			strcpy(rc_first->text, "");
			rc_last = rc_first;
		}

		while (c!=EOF) {
			//배열 초기화
			memset(label, 0, sizeof(label));
			memset(mnemonic, 0, sizeof(mnemonic));
			memset(operand, 0, sizeof(operand));
			memset(operand2, 0, sizeof(operand2));
			memset(xbpe, '0', sizeof(xbpe));

			num_flag = 0;
			line += 5;

			//한줄씩 입력받아서 변수에 저장
			c = fgetc(fr);
			if (c == '.') {
				ungetc(c, fr);
				fgets(str, 100, fr);
				if (pass == 2) fprintf(list, "%s", str);
				continue;
			}

			else if (c == ' ') { //not symbol
				fscanf(fr, "%s", mnemonic);

				if (!strcmp(mnemonic, "BASE")) {
					fscanf(fr, "%s%c", operand, &esc);
					if (pass == 2) {
						base = FindSymbol(operand); //base register에 주소값 저장
						if (base == -1) {
							printf("assemble error : undefined symbol(line:%d)\n", line);
							return -1;
						}
						fprintf(list, "%-4d            %-6s %-6s %-s\n", line, label, mnemonic, operand);
					}
					continue;
				}
				if (!strcmp(mnemonic, "END")) {
					fscanf(fr, "%s%c", operand, &esc);
					if(FindSymbol(operand) == -1){
						printf("assemble error : undefined start operand(line:%d)\n", line);
						return -1; 
					}
					program_len = lc;
					if (pass == 2)
						fprintf(list, "%-4d            %-6s %-6s %-s\n", line, label, mnemonic, operand);
					break;
				}
				else if (!strcmp(mnemonic, "RSUB")) {
					fscanf(fr, "%c", &esc);
				}
				else {
					fscanf(fr, "%s", operand);
					if (operand[strlen(operand) - 1] == ',') { //operand2 존재시
						fscanf(fr, "%s", operand2);
						operand[strlen(operand) - 1] = '\0';
					}
					fscanf(fr, "%c", &esc);
				}
			}
			else { //label exist
				ungetc(c, fr);
				fscanf(fr, "%s%s%s%c", label, mnemonic, operand, &esc);
				if (pass == 1) {
					if (FindSymbol(label) != -1) { //duplicate symbol error
						printf("assemble error : duplicate symbol(line:%d)\n", line);
						return -1;
					}
					InsertSymbol(label, pc);
				}
			}

			//pc 증가, opcode 읽기
			if (mnemonic[0] == '+') { //format 4
				op = FindOpcode(mnemonic + 1)->opcode;
				format = 4;
				plus = format;
				xbpe[3] = '1'; //e = 1
			}
			else {
				ptr = FindOpcode(mnemonic);
				if (ptr) {
					if (ptr->format[0] == '1')
						format = 1;
					else if (ptr->format[0] == '2')
						format = 2;
					else
						format = 3;
					op = ptr->opcode; //opcode
					plus = format;
				}
				//plus PC
				else if (!strcmp(mnemonic, "RESW")) {
					plus = 3 * atoi(operand);
				}
				else if (!strcmp(mnemonic, "RESB")) {
					plus = 1 * atoi(operand);
				}
				else if (!strcmp(mnemonic, "WORD")) {
					plus = 3;
				}
				else if (!strcmp(mnemonic, "BYTE")) {
					if (operand[0] == 'C') {
						plus = strlen(operand) - 3;
					}
					if (operand[0] == 'X') {
						plus = (strlen(operand) - 3) / 2;
					}
				}
				else {
					printf("assemble error : invalid operation code(line:%d)\n", line);
					return -1;
				}
			}
			pc = pc + plus;

			if (pass == 2) {
				//object program line 나누기
				if (!strcmp(mnemonic, "RESW") || !strcmp(mnemonic, "RESB")) {
					new_line_flag = 1;
				}
				else { //new line
					if (line_len + plus > OBJ_LINE_MAX || new_line_flag == 1) {
						rc_last->len = line_len;
						rc_tmp = (record_pointer)malloc(sizeof(t_record));
						rc_tmp->address = lc;
						rc_tmp->link = NULL;
						strcpy(rc_tmp->text, "");
						rc_last->link = rc_tmp;
						rc_last = rc_tmp;
						line_len = plus;
					}
					else
						line_len = line_len + plus;
					new_line_flag = 0;
				}

				//ni
				if (operand[0] == '#') { //immediate addressing
					ni[0] = '0'; ni[1] = '1';
				}
				else if (operand[0] == '@') { //indirect addressing
					ni[0] = '1'; ni[1] = '0';
				}
				else { //simple addressing
					ni[0] = '1'; ni[1] = '1';
				}

				//x
				if (operand2[0] == 'X')
					xbpe[0] = '1';

				//bp & address/dist field
				if (!strcmp(mnemonic, "RSUB")) {
					disp = 0;
					addr = 0;
				}
				else if (!strcmp(mnemonic, "BYTE") || !strcmp(mnemonic, "WORD") 
					|| !strcmp(mnemonic, "RESW") || !strcmp(mnemonic, "RESB")) {}
				
				//addressing field
				else if (format == 3) {
					int symbolLoc;
					if (operand[0] == '#') {
						disp = atoi(operand + 1); 
						if (disp != 0 || operand[1] == '0') { //#0일때
							num_flag = 1;
						}
					}
					if (num_flag == 0) {
						if (operand[0] == '@' || operand[0] == '#')
							symbolLoc = FindSymbol(operand + 1);
						else
							symbolLoc = FindSymbol(operand);

						if (symbolLoc == -1) { //symbol이 존재하지 않음
							printf("assemble error : undefined symbol(line:%d)\n", line);
							return -1;
						}

						disp = symbolLoc - pc; //pc relative
						if (disp < -2048 || disp > 2047) { //base relative
							xbpe[1] = '1';
							disp = symbolLoc - base;
						}
						else {
							xbpe[2] = '1';
							if (disp < 0)
								disp = disp + 0x1000;
						}
					}

				}
				else if (format == 4) {
					if (operand[0] == '#') {
						addr = atoi(operand + 1);
						if (addr != 0 || operand[1] == '0') { //#0일때
							num_flag = 1;
						}
					}
					if (num_flag == 0) {
						if (operand[0] == '@' || operand[0] == '#')
							addr = FindSymbol(operand + 1);
						else
							addr = FindSymbol(operand);
						if (addr == -1) {
							printf("assemble error : undefined symbol(line:%d)\n", line);
							return -1;
						}
						//modification record
						modify_record = (int*)realloc(modify_record, sizeof(int)*(modify_size + 1));
						modify_record[modify_size++] = lc + 1;
					}
				}

				//operand 합친 string 생성
				char tempop[20];
				strcpy(tempop, operand);
				if (strcmp(operand2, "")) {
					strcat(tempop, ",");
					strcat(tempop, operand2);
				}


				//리스트파일에 쓰기
				fprintf(list, "%-4d    %04X    %-6s %-6s %-15s", line, lc, label, mnemonic, tempop);

				//object code 출력
				if (!strcmp(mnemonic, "BYTE")) { //byte
					int oplen = strlen(operand);
					if (operand[0] == 'X') { //16진수 
						for (int i = 2; i < oplen - 1; i++) {
							fprintf(list, "%c", operand[i]);
							sprintf(str, "%c", operand[i]);
							strcat(rc_last->text, str);
						}
					}
					if (operand[0] == 'C') { //CHAR
						for (int i = 2; i < oplen - 1; i++) {
							fprintf(list, "%02X", operand[i]);
							sprintf(str, "%02X", operand[i]);
							strcat(rc_last->text, str);
						}
					}
				}
				else if (!strcmp(mnemonic, "WORD")) {
					fprintf(list, "%06X", atoi(operand));
					sprintf(str, "%06X", atoi(operand));
					strcat(rc_last->text, str);
				}
				else if (!strcmp(mnemonic, "RESB") || !strcmp(mnemonic, "RESW")) {}
				else if (format == 1) {
					fprintf(list, "%02X", op);
					sprintf(str, "%02X", op);
					strcat(rc_last->text, str);
				}
				else if (format == 2) {
					regi1 = GetRegiNum(operand);
					regi2 = GetRegiNum(operand2);
					if (regi1 == -1 || regi2 == -1) {
						printf("assemble error : invalid register name(line:%d)\n", line);
						return -1;
					}
					fprintf(list, "%02X%01X%01X", op, regi1, regi2);
					sprintf(str, "%02X%01X%01X", op, regi1, regi2);
					strcat(rc_last->text, str);
				}
				else if (format == 3) {
					fprintf(list, "%02X%01X%03X", op + (unsigned int)strtol(ni, NULL, 2), (unsigned int)strtol(xbpe, NULL, 2), disp);
					sprintf(str, "%02X%01X%03X", op + (unsigned int)strtol(ni, NULL, 2), (unsigned int)strtol(xbpe, NULL, 2), disp);
					strcat(rc_last->text, str);
				}
				else { //format4
					fprintf(list, "%02X%01X%05X", op + (unsigned int)strtol(ni, NULL, 2), (unsigned int)strtol(xbpe, NULL, 2), addr);
					sprintf(str, "%02X%01X%05X", op + (unsigned int)strtol(ni, NULL, 2), (unsigned int)strtol(xbpe, NULL, 2), addr);
					strcat(rc_last->text, str);
				}
				fprintf(list, "\n");
			} //pass2

			lc = pc; //location counter 증가
		} //while
		fclose(fr);
	} //for
	
	//write text record
	rc_last->len = line_len;
	rc_last->link = NULL;
	rc_tmp = rc_first;
	while (rc_tmp != NULL) {
		fprintf(object, "T%06X%02X%s\n", rc_tmp->address, rc_tmp->len, rc_tmp->text);
		rc_tmp = rc_tmp->link;
	}
	//write modification record
	for (int i=0; i < modify_size; i++)
		fprintf(object, "M%06X05\n", modify_record[i]);
	//write end record
	fprintf(object, "E%06X\n", start_adr);
	
	//파일에 완성된 symbol table 쓰기
	FILE *f_sym = fopen("symbol.txt", "w");
	symbol_pointer s_tmp;
	for(int i=0; i<26; i++){
		s_tmp = symbolTable[i];
		while(s_tmp!=NULL){
			fprintf(f_sym, "\t%-8s%04X\n", s_tmp->symbol, s_tmp->line);
			s_tmp = s_tmp->link;
		}
	}
	fclose(f_sym);

	fclose(list);
	fclose(object);

	printf("\033[0;32mSuccessfully\033[0m assemble %s\n", filename);
	return 1;
}

int FindSymbol(char* s) {
	int symbolLoc = -1;
	symbol_pointer ptr = symbolTable[s[0]-'A']; //symbol의 첫글자 key
	while (ptr != NULL) {
		if (!strcmp(ptr->symbol, s)) {
			symbolLoc = ptr->line;
			break;
		}
		ptr = ptr->link;
	}
	return symbolLoc;
}

void InsertSymbol(char* s, int line){
	int alpha = s[0] - 'A'; //alphabetical order
	symbol_pointer tmp;
	symbol_pointer ptr = (symbol_pointer)malloc(sizeof(symbol_node));
	ptr->line = line;
	strcpy(ptr->symbol, s);
	
	tmp = symbolTable[alpha];
	//배열의 해당 index가 비어있을 때
	if(tmp == NULL || strcmp(s, tmp->symbol)<0){ //내림차순 정렬
		ptr->link = tmp;
		symbolTable[alpha] = ptr;
		return;
	}

	while(1){
		if(tmp->link==NULL || strcmp(s, tmp->link->symbol)<0){ //내림차순 정렬
			ptr->link = tmp->link;
			tmp->link = ptr;
			break;
		}
		tmp = tmp->link;
	}
	return;
}

int GetRegiNum(char* regi) {
	if (!strcmp(regi, "A") || !strcmp(regi, ""))
		return 0; //0
	else if (!strcmp(regi, "X"))
		return 1;
	else if (!strcmp(regi, "L"))
		return 2;
	else if (!strcmp(regi, "B"))
		return 3;
	else if (!strcmp(regi, "S"))
		return 4;
	else if (!strcmp(regi, "T"))
		return 5;
	else if (!strcmp(regi, "F"))
		return 6;
	else if (!strcmp(regi, "PC"))
		return 8;
	else if (!strcmp(regi, "SW"))
		return 9;
	else
		return -1;
}


