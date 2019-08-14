#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define DEBUG 0

#define MAX_ROW_LENGTH 10000

#define ALPHABET_SIZE 53
#define NUMBER_OF_STATES 100
#define MAX_COLLISIONS 5
#define TM_SIZE (ALPHABET_SIZE)*(NUMBER_OF_STATES)*(MAX_COLLISIONS) 
#define MAX_NUMBER_OF_ACCEPTED_STATES 100

//definisco limiti stringa

#define LIMIT '?'

//definisco errori

#define NO_ERR 0
#define ERR_CONFLICT 1
#define ERR_INSTR_NOT_EXIST 2
#define ERR_TAPE 3

struct step_result{
    
    unsigned char error;
    int next_state;
    char tape_movement;

};

struct tm_instruction {
    
    int initial_state;
    int final_state;

    char input;
    char output;

    char direction;
    
};

struct tm_row {
    
    struct tm_instruction instruction;

    char valid;    

    char conflict;
    int conflict_address;

    int step;

    int last_nd_address;
    
    char used;
};

int change_c_in_int (char input){
    int offset;
    
    if (input == '_') {
        offset = 0;
    } else if ((input >= 'a') && (input <= 'z')){
        offset = input - 96;
    } else if ((input >= 'A') && (input <= 'Z')){
        offset = input - 64;
    }
    
    return offset;
}

int direction_to_movement(char direction) {
    if(direction == 'R') return 1;
    if(direction == 'S') return 0;
    if(direction == 'L') return -1;
    return 0;
}

int tm_hashing(int state, char input) {
    return (state % NUMBER_OF_STATES) * ALPHABET_SIZE * MAX_COLLISIONS + change_c_in_int(input) * MAX_COLLISIONS;
    //return ((state + input*input) << 3) % TM_SIZE;
}

void print_tm(struct tm_row *tm) {
    int i = 0;
    for(i = 0; i < TM_SIZE; i++) {
        if(tm[i].valid == 1) {
            printf("%6d: %d %c %c %c %d | %d %d\n", i,  tm[i].instruction.initial_state,
                                              tm[i].instruction.input, 
                                              tm[i].instruction.output, 
                                              tm[i].instruction.direction, 
                                              tm[i].instruction.final_state,
                                                tm[i].conflict,
                                                  tm[i].conflict_address);
        }
    }
}

void initialize_tm(struct tm_row* tm) {
    int i = 0;
    for(i = 0; i < TM_SIZE; i++) {
        tm[i].valid = 0;
    }
}

int add_instruction_to_tm_with_index(struct tm_row *tm, struct tm_instruction *instruction, int index) {
    tm[index].valid = 1;

    tm[index].conflict = 0;

    tm[index].instruction.initial_state = instruction->initial_state;
    tm[index].instruction.final_state = instruction->final_state;

    tm[index].instruction.input = instruction->input;
    tm[index].instruction.output = instruction->output;

    tm[index].instruction.direction = instruction->direction; 
}

int add_instruction_to_tm(struct tm_row *tm, struct tm_instruction *instruction) {
    int index = tm_hashing(instruction->initial_state, instruction->input);
    if(tm[index].valid == 0) {
        add_instruction_to_tm_with_index(tm, instruction, index);
        return 1;
    } else if(tm[index].valid == 1) {
#if DEBUG == 1
        printf("***GESTISCI CONFLITTO!***\n");
#endif
        int i = index; 
 
        while(i < TM_SIZE) {
            i++;
            if(tm[i].valid == 0) {
                add_instruction_to_tm_with_index(tm, instruction, i);
                tm[index].conflict_address = i;
                tm[index].conflict = 1;
                return 1;
            }
        }
        
        return 0;

    } else {
        printf("***MERDA!***\n");
        //merda

        return 0;
    }
}


int parse_instruction(  struct tm_instruction *instruction, 
                        char *row ) {
    sscanf(row, "%d %c %c %c %d", &(instruction->initial_state), 
                                  &(instruction->input), 
                                  &(instruction->output), 
                                  &(instruction->direction), 
                                  &(instruction->final_state) );
    //controllare che l'istruzione sia stata scritta correttamente
    return 1;
}

int is_accepted_state(int *accepted_states, int number_of_accepted_states, int state) {
    int i = 0;
    for(i = 0; i < number_of_accepted_states; i++) {
        if(state == accepted_states[i]) {
            return 1;
        }
    }
    return 0;
}

char* patch_string(char* input_string){          //delimito la stringa a sinistra mettendo un blank
    int counter = 0;
    input_string = input_string - 1;
    *input_string = '?';
    
    while (*input_string != '\0'){
        counter++;
        input_string++;
    }
    *input_string = '?';
    *(input_string + 1) = '\0';
    
    return input_string - counter;
}

struct step_result next_step(struct tm_row* tm, int current_state, char c_read) {
    struct step_result result;
  
  	if (c_read == LIMIT) {                                                                                      //se supera il limite del nastro di input ERRORE
        result.error = ERR_TAPE;
        result.next_state = NULL;
        result.tape_movement = NULL;
        return result;
    }
  
    int address = tm_hashing(current_state, c_read); 
	while(tm[address].used == 1) {
    	address = tm[address].conflict_address;
    }  

    if (tm[address].valid == 0) {  //se non c'e un'istruzione valida, ERRORE
        address = tm_hashing(current_state, '_');
#if DEBUG == 1
        printf("provo con _\t|\t%d -> %d, %d\n", current_state, address, tm[address].valid);
#endif
        if (tm[address].valid == 0){
            result.error = ERR_INSTR_NOT_EXIST;
            result.next_state = NULL;
            result.tape_movement = NULL;
            return result;
        } 
        
        result.error = NO_ERR;    //nessun errore
        result.next_state = tm[address].instruction.final_state;
        result.tape_movement = direction_to_movement(tm[address].instruction.direction);
        return result;
    }
    
  	/*
  	 *  controllo che l'indice dell'istruzione che ho trovato corrisponda
  	 *	effettivamente ad una istruzione con lo stato e il
     *  carattere che mi sono stati passati
    */
  	if (tm[address].instruction.input != c_read ||
       	tm[address].instruction.initial_state != current_state) {
        result.error = ERR_INSTR_NOT_EXIST;
      	result.next_state = NULL;
        result.tape_movement = NULL;
        return result;
    }
    
    if (tm[address].conflict == 1){  //se c'e conflitto ERRORE DI CONFLITTO
        result.error = ERR_CONFLICT;
        result.next_state = tm[address].instruction.final_state;
        result.tape_movement = direction_to_movement(tm[address].instruction.direction);
        return result;
    }
    
    
    
    result.error = NO_ERR;    //nessun errore
    result.next_state = tm[address].instruction.final_state;
    result.tape_movement = direction_to_movement(tm[address].instruction.direction);
    return result;
}

/*
 * 0:  0
 * 1:  1
 * -1: U
 */
int run_tm( struct tm_row *tm, 
            char *tape, 
            int max_number_of_steps,
            int *accepted_states, 
            int number_of_accepted_states,
            int starting_index) {
 	
  	struct step_result tmp_result;
  	int i;
  	int current_index;
  	int tmp_result_run = 0, result_run = 0;
	int tmp_index, used_index;
  	char *new_tape;
    char *tmp_tape;

    unsigned int go_forward_index = 0;

  	int tmp_state = tm[starting_index].instruction.initial_state; // 1

    //printf("%d %d %s\n", starting_index, tmp_state, tape);
  
  	if (tape[0]== '?'){
    	tape++;
    }
  
  	for(i = 0; i < max_number_of_steps; i++) {
    	tmp_result = next_step(tm, tmp_state, tape[0]);
      	current_index = i == 0 ? starting_index : tm_hashing(tmp_state, tape[0]);
#if DEBUG == 1
        printf("%d\t%d\t%s\n", i, current_index, tape);
#endif
      	if (is_accepted_state(accepted_states, number_of_accepted_states, tmp_result.next_state)){
          return 1;
        }
        
      	switch(tmp_result.error) {
        	case NO_ERR:
            	*tape = tm[current_index].instruction.output;
            	tmp_state = tmp_result.next_state;
            	tape += tmp_result.tape_movement;
            	break;
            
          	case ERR_CONFLICT:
                tmp_index = current_index;
#if DEBUG == 1
            	printf("entrata\n");
#endif
                while(tm[tmp_index].conflict != 0) { 

            	    tmp_tape = tape;
                    while(*tmp_tape != '?') {
                        tmp_tape--;
                        go_forward_index++;
                    }	
                    new_tape = strdup(tmp_tape); 
                    new_tape += go_forward_index;

                    used_index = tmp_index;                    

                    tm[used_index].used = 1; 
                    
                    tmp_index = tm[tmp_index].conflict_address;
                    tmp_result_run = run_tm(tm, 
                                 			new_tape,
                                 			max_number_of_steps - i,
                                 			accepted_states,
                                 			number_of_accepted_states,
                                 			tmp_index);
                    tm[used_index].used = 0;
                  	
                    if(tmp_result_run == 1)
                        return 1;
                }
#if DEBUG == 1
                printf("uscita\n");
#endif
            	*tape = tm[current_index].instruction.output;
            	tmp_state = tmp_result.next_state;
            	tape += tmp_result.tape_movement;
            	break;
			
          	case ERR_INSTR_NOT_EXIST:
#if DEBUG == 1
                printf("ISTRUZIONE NON ESISTE!\n");
#endif
            	return 0;
            	break;
          		
          	case ERR_TAPE:
            	return 0;
            	break;
          	default:
            	return 0;
        }
    }
  	return -1;  
}

int main() {
    struct tm_row tm[TM_SIZE]; 
    initialize_tm(tm);

    char r[MAX_ROW_LENGTH];

    int accepted_states[MAX_NUMBER_OF_ACCEPTED_STATES];
    int a_s_i = 0;

    int max_number_of_steps = 0;

    int j = 0;

    char *tape;
    /*
     * -1: invalid phase
     * 0: starting phase
     * 1: tr
     * 2: acc
     * 3: max
     * 4: run
    */
    int phase = 0;
#if DEBUG == 1
    printf("--- %d ---\n", phase);
#endif
    while(fgets(r, MAX_ROW_LENGTH, stdin) != NULL) {
        
        char *tmp;
        tmp = r;
        while(*tmp != '\n') tmp++;
        *tmp = '\0';

        switch(phase) {
            case 0: // starting phase
                if( r[0] == 't' && 
                    r[1] == 'r') {
                    phase = 1;     
                } else {
                    phase = -1;
                } 
                break;

            case 1:
                if( r[0] == 'a' &&
                    r[1] == 'c' &&
                    r[2] == 'c') {
                    phase = 2;
                } else if ( r[0] < 48 ||
                            r[0] > 57 ) {
                    phase = -1;
                } else {
                    struct tm_instruction current_instruction;
                    parse_instruction(&current_instruction, r);
                    add_instruction_to_tm(tm, &current_instruction);
#if DEBUG == 1

                    print_tm(tm);
#endif
                }
                break;

            case 2:
                if( r[0] == 'm' &&
                    r[1] == 'a' &&
                    r[2] == 'x') {
                    phase = 3;
                } else if ( r[0] < 48 ||
                            r[0] > 57 ) {
                    phase = -1;
                } else {
                    sscanf(r, "%d", &(accepted_states[a_s_i]));
                    a_s_i++;
                }
                break;

            case 3:
#if DEBUG == 1
                for(j=0; j<a_s_i; j++){
                    printf("%d\n", accepted_states[j]);
                }
#endif
                if( r[0] == 'r' &&
                    r[1] == 'u' &&
                    r[2] == 'n') {
                    phase = 4;
                } else if ( r[0] < 48 ||
                            r[0] > 57 ) {
                    phase = -1;
                } else {
                    sscanf(r, "%d", &max_number_of_steps);
                }
                break;
        
            case 4:
#if DEBUG == 1
                printf("%s\n", r);
                print_tm(tm); 
#endif
                tape = patch_string(r);
                
                int starting_index = tm_hashing(0, tape[1]);
                
                printf("%s\n", tape);
    
 
                printf("%d\n", run_tm(tm, tape, max_number_of_steps, accepted_states, a_s_i, starting_index));
                phase = -1; 
                break;

            case -1:
                printf("ERRORE!!!\n");
                return 1;
                break;

            default:
                printf("ERRORE GRAVE!!!\n");
        }
#if DEBUG == 1 
        printf("--- %d ---\n", phase);
#endif
    }
}


/*
 * 
0 a c R 1
0 b c R 2
1 a a R 1
1 a d L 3
1 b b R 1
2 a a R 2
2 b b R 2
2 b d L 3
3 a a L 3
3 b b L 3
3 c c R 4
4 d d R 10
4 a c R 5
4 b c R 6
5 a a R 5
5 b b R 5
5 d d R 7
6 a a R 6
6 b b R 6
6 d d R 8
7 d d R 7
7 a d L 9 
8 d d R 8
8 b d L 9
9 d d L 9
9 a a L 3
9 b b L 3
9 c c R 10
10 d d R 10
10 _ _ S 11




     5: 0 a c R 1 | 0 0
    10: 0 b c R 2 | 0 0
   270: 1 a a R 1 | 1 271
   271: 1 a d L 3 | 0 0
   275: 1 b b R 1 | 0 0
   535: 2 a a R 2 | 0 0
   540: 2 b b R 2 | 1 541
   541: 2 b d L 3 | 0 0
   800: 3 a a L 3 | 0 0
   805: 3 b b L 3 | 0 0
   810: 3 c c R 4 | 0 0
  1065: 4 a c R 5 | 0 0
  1070: 4 b c R 6 | 0 0
  1080: 4 d d R 10 | 0 0
  1330: 5 a a R 5 | 0 0
  1335: 5 b b R 5 | 0 0
  1345: 5 d d R 7 | 0 0
  1595: 6 a a R 6 | 0 0
  1600: 6 b b R 6 | 0 0
  1610: 6 d d R 8 | 0 0
  1860: 7 a d L 9 | 0 0
  1875: 7 d d R 7 | 0 0
  2130: 8 b d L 9 | 0 0
  2140: 8 d d R 8 | 0 0
  2390: 9 a a L 3 | 0 0
  2395: 9 b b L 3 | 0 0
  2400: 9 c c R 10 | 0 0
  2405: 9 d d L 9 | 0 0
  2650: 10 _ _ S 11 | 0 0
  2670: 10 d d R 10 | 0 0

*/
    
