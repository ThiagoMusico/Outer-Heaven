/* Engenharia de Computação - 016
   Organização de Computadores Digitais - Trabalho 2
   Guilherme Lima Blatt		9771470
   Lucas Akira Morishita	9771320
   Thiago Músico			9771567
   Tiago Toledo Junior		9771309
*/

/* code-c - Student's code for mmcpu

   Copyright 2017  Monaco F. J.   <monaco@icmc.usp.br>

   This file is part of Muticlycle Mips CPU (MMCPU)

   MMCPU is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


/* After you have implemented your functions, you may safely remove these lines. */
#include <stdio.h>
#include <stdlib.h>
#include "mask.h"
#include "cpu.h"


// Função que simula a ULA. Baseado em alu_op, realiza a operação necessária
void alu( int a, int b, char alu_op, int *result_alu, char *zero, char *overflow) {
	
	switch(alu_op){

		// Operação de Soma
		case 0b0010:
			*result_alu = a + b;
			if(a + b == 0)
				*zero = ativa_bit_zero;
			break;

		// Operação de Subtração
		case 0b0110:
			*result_alu = a - b;
			if(a - b == 0)
				*zero = ativa_bit_zero;
			break;

		// Operação AND
		case 0b0000:
			*result_alu = a & b;
			if(a & b == 0)
				*zero = ativa_bit_zero;
			break;

		// Operação OR
		case 0b0001:
			*result_alu = a | b;
			if(a | b == 0)
				*zero = ativa_bit_zero;
			break;

		// Operação Set on Less Than
		case 0b0111:
			if(a < b){
				*result_alu = 0x01;
				*zero = desativa_bit_zero;
			}
			else{
				*result_alu = 0x00;
				if(a == b && b == 0){
					*zero = ativa_bit_zero;
				}
				else *zero = 0	;
			}
			break;
	}
}


void control_unit(int IR, short int *sc) {

	// Se estamos no estado -1 (primeira execução), setamos os bits de controle para acessar o estado 0
	if(IR == -1){
		*sc = 0b1001010000001000;
		return; 
	}

	// Controla o retorno para o estado 0. Seta os bits de controle para acessar o estado 1
	if(*sc == ((short int) 0b0000001010100100) || *sc == ((short int) 0b0000000000000011) || 
		*sc == 0b0000010100000000 ||  *sc == ((short int) 0b0010100000000000) || 
		*sc == ((short int) 0b0100000000000010)) {
			*sc = 0b1001010000001000;
			return; 
	}

	// Se os bits de controlem equivalem ao estado 1, habilita a entrada no bloco que decide
	// se vamos para o estado 2,6,8 ou 9
	if(*sc == ((short int) 0b1001010000001000)){
		*sc = 0b0000000000011000;
		return;
	}

	// Se viemos do estado 1, definimos, a partir do opcode qual será o próximo estado.
	if(*sc == ((short int) 0b0000000000011000)){
		switch(IR & separa_cop){
			case 0x00000000:
				// Tipo-R - Estado 6
				*sc = 0b0000000001000100;
				break;

			case 0x08000000:
				// Jump - Estado 9
				*sc = 0b0000010100000000;
				break;

			case 0x10000000:
				// Branch - Estado 8
				*sc = 0b0000001010100100;
				break;

			default:
				// LW ou SW - Estado 2
				*sc = 0b0000000000010100;
				break;
		}
		return;
	}

		// Se viemos do estado 6, setamos os bits de controle para o estado 7
		if(*sc == ((short int) 0b0000000001000100)){
			*sc = 0b0000000000000011;
			return;
		}

		// Se viemos do estado 2, setamos os bits de controle para o estado 3 ou 5
		if(*sc == ((short int) 0b0000000000010100)){

			switch((IR & separa_cop) >> 26){
				case 0x23:
					// Load Word
					*sc = 0b0001100000000000;
					break;

				case 0x2b:
					// Store Word
					*sc = 0b0010100000000000;
					break;
			}
			return;
		}

		// Se viemos do estado 3, setamos os bits de controle para o estado 4
		if(*sc == ((short int) 0b0001100000000000)){
			*sc = 0b0100000000000010;
			return;
		}

}



void instruction_fetch(short int sc, int PC, int ALUOUT, int IR, int* PCnew, int* IRnew, int* MDRnew) {
	char zero, overflow;
	//Verifica se o estado a ser executado é um Instruction Fetch
	if(sc == ((short int) 0b1001010000001000)){
		*IRnew = memory[PC];
		
		//Soma 1 em PC
		alu(PC, 1, alu_control(IR, sc), PCnew, &zero, &overflow);
		*MDRnew = memory[PC];
		
		// Condição de parada da execução do programa
		if(*IRnew == 0){
			loop = 0;
			return;
		}
	}
	else return;
}


void decode_register(short int sc, int IR, int PC, int A, int B, int *Anew, int *Bnew, int *ALUOUTnew) {
	
	char zero, overflow;
	int temp, i, mask = 0x0000001;
	//Verifica se o estado a ser executado é um Decode Register
	if(sc == ((short int) 0b0000000000011000)){
		short int aux = (IR & separa_imediato);
		
		//Joga os valores dos registradores para os Buffers A e B
		*Anew = reg[(IR & separa_rs) >> 21]; 
		*Bnew = reg[(IR & separa_rt) >> 16];
		//Soma em Pc o valor do imediato (Para Branches)

		// Caso o imediato seja negativo
		if((aux) & 0x00008000){
			
			// Retira o bit de sinal para encontrar o imediato
			aux = aux & 0x0fff ;
			aux = (-1) * aux;
		} 

		alu(PC, (aux), alu_control(IR, sc), ALUOUTnew, &zero, &overflow);
	}
	else return;
}


void exec_calc_end_branch(short int sc, int A, int B, int IR, int PC, int ALUOUT, int *ALUOUTnew, int *PCnew){
	
	char zero, overflow;

	if(sc == ((short int) 0b0000000000010100)){
		// Caso a instrução seja uma LW ou SW
		alu(A, (IR & separa_imediato) , alu_control(IR,sc), ALUOUTnew, &zero, &overflow);
		return;
	}

	else if(sc == ((short int) 0b0000000001000100)){
		// Caso a instrução seja um Tipo-R
		alu(A, B, alu_control(IR, sc), ALUOUTnew, &zero, &overflow);
		return;
	}

	else if(sc == ((short int) 0b0000001010100100)){
		// Caso a instrução seja uma Branch
		alu(A, B, alu_control(IR, sc), ALUOUTnew, &zero, &overflow);
		if(zero == 1){
			*PCnew = ALUOUT;
			return;
		}
		else return;
	}

	else if(sc == ((short int) 0b0000010100000000)){
		// Caso a instrução seja uma Jump
		*PCnew = ((PC & separa_4bits_PC) | ( (IR & separa_endereco_jump)));
		return;
	}

	else return;
}


void write_r_access_memory(short int sc, int B, int IR, int ALUOUT, int PC, int *MDRnew, int *IRnew) {

	if(sc == ((short int) 0b0001100000000000)){
		// Caso seja uma LW
		*MDRnew = memory[ALUOUT];
		return;
	}

	else if(sc == ((short int) 0b0010100000000000)){
		// Caso seja uma SW
		memory[ALUOUT] = reg[(IR & separa_rt) >> 16];
		return;
	}

	else if(sc == ((short int) 0b0000000000000011)){
		// Caso seja um Tipo-R
		reg[(IR & separa_rd) >> 11] = ALUOUT;
		return;
	}

	else return;
}


void write_ref_mem(short int sc, int IR, int MDR, int ALUOUT) {
	//Verifica se o estado a ser executado é Write Ref Mem	
	if(sc == ((short int) 0b0100000000000010)){
		// Caso seja uma LW
		reg[(IR & separa_rt) >> 16] = MDR;
	}
	else return;

}


// Função responsável por fazer a tabela de ALU-CONTROL
char alu_control(int IR, int sc){
	switch(((sc & separa_ALUOp0) | (sc & separa_ALUOp1)) >> 5){
		
		// Load Word e Store Word
		case 0x0:
		
			return 0b0010;
			break;

		// Branch
		case 0x1:
			
			return 0b0110;
			break;

		// Tipo-R
		case 0x2:
			//Separa os ultimos 4 bits da Registrador de Instrução (São os bits Usados no Campo de Funções)
			switch((IR & 0x0F)){
				
				// Add
				case 0x00:
					return 0b0010;
					break;

				// Sub
				case 0x02:
					return 0b0110;
					break;

				// And
				case 0x04:
					return 0b0000;
					break;

				// Or
				case 0x05:
					return 0b0001;
					break;	

				// Set on Less Than
				case 0x0a:
					return 0b0111;
					break;
			}
	}
}
