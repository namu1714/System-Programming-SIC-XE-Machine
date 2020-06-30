#include "20171101.h"

int Run(){
	int target_addr, target_value, disp, opcode, e, nextPC;
	int regi1, regi2;
	hash_pointer op_ptr;
	
	while(PC < end_addr){
		//find opcode
		opcode = memory[PC] >> 2 << 2;
		op_ptr = FindOpcodebyNum(opcode);
		
		if(op_ptr == NULL){
			printf("error: unknown opcode [PC:%X]\n", PC);
			return -1;
		}

		//format
		if(op_ptr->format[0] == '1'){
			nextPC = PC + 1;
		}
		else if(op_ptr->format[0] == '2'){
			nextPC = PC + 2;
			regi1 = memory[PC+1]>>4;
			regi2 = memory[PC+1]&0x0F;
		}
		else{ //format 3 or 4
			e = (memory[PC+1]>>4) & 1;
			if(e == 0){ //format 3
				nextPC = PC + 3;
				disp = ((memory[PC+1]&0x0F)<<8) + memory[PC+2];
				if(((memory[PC+1]>>5)&1) == 1){ //PC relative
					if(disp>2047) disp -= 0xFFF;
					target_addr = nextPC + disp;
				}
				else if(((memory[PC+1]>>6)&1) == 1){ //base relative
					target_addr = regi_arr[3] + disp;
				}
				else{
					target_addr = disp;
				}
			}
			else{ //format 4
				nextPC = PC + 4;
				target_addr = ((memory[PC+1]&0x0F)<<16) + (memory[PC+2]<<8) + (memory[PC+3]);
			}

			if((memory[PC]&3) == 2){ //indirect
				target_addr = (memory[target_addr]<<16) + (memory[target_addr+1]<<8) + (memory[target_addr+2]);
			}
			if((memory[PC+1]>>7) == 1){ //indexed X
				target_addr += regi_arr[1];
			}
				
			if((memory[PC]&3) == 1){ //immediate
				target_value = target_addr;
			}
			else{
				target_value = (memory[target_addr]<<16) + (memory[target_addr+1]<<8) + memory[target_addr+2];
			}
			
		}
		
		switch(opcode){
		case 0x00: //LDA
			regi_arr[0] = target_value;
			break;
		case 0x68: //LDB
			regi_arr[3] = target_value;
			break;
		case 0x74: //LDT
			regi_arr[5] = target_value;
			break;
		case 0x50: //LDCH
			regi_arr[0] = regi_arr[0]>>8<<8;
			regi_arr[0] = target_value>>16;
			break;
		case 0x0C: //STA
			memory[target_addr] = regi_arr[0]>>16;
			memory[target_addr+1] = regi_arr[0]>>8;
			memory[target_addr+2] = regi_arr[0];
			break;
		case 0x14: //STL
			memory[target_addr] = regi_arr[2]>>16;
			memory[target_addr+1] = regi_arr[2]>>8;
			memory[target_addr+2] = regi_arr[2];
			break;
		case 0x10: //STX
			memory[target_addr] = regi_arr[1]>>16;
			memory[target_addr+1] = regi_arr[1]>>8;
			memory[target_addr+2] = regi_arr[1];
			break;
		case 0x54: //STCH
			memory[target_addr] = regi_arr[0]&0x0000FF;
			break;
		case 0x48: //JSUB
			regi_arr[2] = nextPC; //store return address in L
			nextPC = target_addr;
			break;
		case 0x4C: //RSUB
			nextPC = regi_arr[2];
			break;
		case 0x3C: //J
			nextPC = target_addr;
			break;
		case 0x30: //JEQ
			if(CC==0) nextPC = target_addr;
			break;
		case 0x38: //JLT
			if(CC<0) nextPC = target_addr;
			break;
		case 0x28: //COMP
			CC = regi_arr[0] - target_value;
			break;
		case 0xA0: //COMPR
			CC = regi_arr[regi1]-regi_arr[regi2];
			break;
		case 0xB8: //TIXR
			CC = ++regi_arr[1] - regi_arr[regi1];
			break;
		case 0xB4: //CLEAR
			regi_arr[regi1] = 0;
			break;
		case 0xE0: //TD
			CC = -1;
			break;
		case 0xD8: //RD
			break;
		case 0xDC: //WD
			break;
		}
	
		PC = nextPC;

		//breakpoint check
		for(int i=0; i<bp_size; i++){
			if(break_point[i] == PC){
				PrintRegisters();
				printf("\tStop at checkpoint[%X]\n", PC);
				return 1;
			}
		}
	}
	PrintRegisters();
	printf("\tEnd Program\n");
	return 1;
}

void PrintRegisters(){
	printf("A : %06X  X : %06X\n", regi_arr[0], regi_arr[1]);
	printf("L : %06X PC : %06X\n", regi_arr[2], PC);
	printf("B : %06X  S : %06X\n", regi_arr[3], regi_arr[4]);
	printf("T : %06X\n", regi_arr[5]);
}

void PrintBreakPoint(){
	printf("\tbreakpoint\n");
	printf("\t----------\n");
	for(int i=0;i<bp_size;i++){
		printf("\t%X\n", break_point[i]);
	}
}

int AddBreakPoint(int bp){
	//중복체크
	for(int i=0;i<bp_size;i++){
		if(break_point[i] == bp) return -1;
	}

	break_point = (int*)realloc(break_point, sizeof(int)*(bp_size+1));
	break_point[bp_size++] = bp;
	printf("\t[ok] create breakpoint %04X\n", bp);
	return 1;
}

void ClearBreakPoint(){
	bp_size = 0;
	free(break_point);
	break_point = (int*)malloc(sizeof(int));
	printf("\t[ok] clear all breakpoints\n");
}

hash_pointer FindOpcodebyNum(int opcode){
	hash_pointer ptr;
	for(int i=0;i<HASH_TABLE_SIZE;i++){
		ptr = hashTable[i];
		for(; ptr; ptr=ptr->link){
			if(opcode == ptr->opcode)
				return ptr;
		}
	}
	return NULL; //opcode가 없을 경우
}
