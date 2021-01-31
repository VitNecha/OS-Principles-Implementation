
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define STR_BUFFER_SIZE 16
#define SEC_MEM_BUFFER_SIZE 10000
#define TOKEN_DELIMITERS " \t\n\r\a" // DELIMITERS - To ignore different tags when parser tokenizes the command


// Page structure
typedef struct Page {
	struct Page * prev, * next;
	char data[STR_BUFFER_SIZE];
	int id, r_bit;
	double m;
}Page;

// Page creating
Page* create_page(char * str, int index) {
	Page * p = (Page*)malloc(sizeof(Page));
	p->prev = NULL;
	p->next = NULL;
	strcpy(p->data,str);
	p->id = index;
	p->r_bit = 0;
	p->m = 0;
}
// Main memory queue structure
typedef struct Queue {
	struct Page * front, * rear;
	int curr_size, max_size;
}Queue;

// Queue creation
Queue* create_queue(int max_size) {
	Queue* q = (Queue*)malloc(sizeof(Queue));
	q->max_size = max_size;
	q->curr_size = 0;
	q->front = NULL;
	q->rear = NULL;
	return q;
}
// Queue current size 
int queue_size(Queue * q) {
	return q->curr_size;
}

// Queue maximum size
int queue_max_size(Queue * q) {
	return q->max_size;
}


// Add to the queue
void fill_queue(Queue * q, char ** str_arr){
	for (int i = 0; i < queue_max_size(q); i++) {
		Page * p = create_page(str_arr[i], i);
		if (queue_size(q) == 0){
			q->rear = p;
			q->front = p;
			q->curr_size++;
		}
		else {
			p->prev = q->front;
			p->prev->next = p;
			q->front = p;
			q->curr_size++;
		}
	}
}

// Print secondary memory
void print_sec_mem(char ** secondary, int len){
	printf("Secondary memory = {");
	for (int i = 0; i < len; i++) printf("%s, ",secondary[i]);
	printf("\n");
}
// Print singe page
void print_page(Page * p) {
	printf("Page:: Index: %d, str: %s, R-bit: %d\n", p->id, p->data, p->r_bit);
}

// Print single queue
void print_queue(Queue * q) {
	Page * temp_p = q->front;
	printf("Queue size: %d\n", queue_size(q));
	printf("Queue max size: %d\n", queue_max_size(q));
	while (temp_p != NULL) {
		print_page(temp_p);
		temp_p = temp_p->prev;
	}
}

//Function checks if the page exists in the queue (by the index of memory segment)
int page_exists(Queue * q, int index){
	Page * temp_p = q->front;
	while (temp_p != NULL) {
		if (temp_p->id == index) return 1;			
		temp_p = temp_p->prev;
	}
	return 0;
}

// Function that finds and returns specific page within the main memory (by index)
Page * find_page(Queue * q, int index) {
	Page * p = q->front;
	while (p != NULL) {
		if (p->id == index) return p;
		p = p->prev;
	}
	return q->front; // Preventing NULL return (in case anomaly accures)
}

// Function adds a single character to the page data
void write_to_page(Page * p, char c) {
	int temp_len = strlen(p->data);
	p->data[temp_len] = c;
	p->data[temp_len + 1] = '\0';
}

// Function allocates main memory from secondary memory (moves for secondary to main) according to FIFO - Second Chance (R bit)
void fifo_second_chance(Queue * q, char ** secondary, int index) {
	Page * temp_p = q->rear;

	// First Out (R bit = 0) page removal
	while (temp_p->r_bit != 0) {
		q->rear = temp_p->next;
		q->rear->prev = NULL;
		q->front->next = temp_p;
		temp_p->prev = q->front;
		temp_p->next = NULL;
		q->front = temp_p;
		q->front->r_bit = 0;
		temp_p = q->rear;
	}
	
	// First In page addition to the queue
	q->rear = temp_p->next;
	q->rear->prev = NULL;
	strcpy(secondary[temp_p->id],temp_p->data);
	temp_p = NULL;
	temp_p = create_page(secondary[index],index);
	temp_p->prev = q->front;
	q->front->next = temp_p;
	q->front = temp_p;
}

// Function updates queue accroding to lru after page (referred by index) has being used
void lru(Queue * q, int index) {
	Page * temp_p = find_page(q, index);
	
	if (temp_p == q->front) return; // If page is a front of queue - no changes needed
	else if (temp_p == q->rear) { 
		q->rear = temp_p->next;
		q->rear->prev = NULL;
	}
	else {
		temp_p->next->prev = temp_p->prev;
		temp_p->prev->next = temp_p->next;
	}
	temp_p->next = NULL;
	temp_p->prev = NULL;
	temp_p->prev = q->front;
	q->front->next = temp_p;
	q->front = temp_p;
}

// Function allocates main memory from secondary memory according to standard FIFO method (as part of LRU method in this case)
void fifo_standard(Queue * q, char ** secondary, int index) {
	Page * temp_p = q->rear;
	q->rear = temp_p->next;
	q->rear->prev = NULL;
	strcpy(secondary[temp_p->id],temp_p->data);
	temp_p = NULL;
	temp_p = create_page(secondary[index],index);
	temp_p->prev = q->front;
	q->front->next = temp_p;
	q->front = temp_p;
}
// Function writes a single character into page (method = memeory management: 0 - FIFO Second Chance, 1 = LRU)
void memory_write(Queue * q, char ** secondary, int index, char c, int method) {
	Page * p;
	int seek_res = page_exists(q, index); // Result of page search (main memory), later used to determine R bit
	// Page not found in the main memory -> Pull out of secondary memory
	if (seek_res == 0) {
		if (method == 0) fifo_second_chance(q, secondary, index);
		else if (method == 1) fifo_standard(q, secondary, index);
	}
	// Extract page to procceed writing
	p = find_page(q, index);

	if (seek_res == 1) {
		if (method == 0) p->r_bit = 1; // Determine R bit (if page found originally - set 1)
		else if (method  == 1) lru(q, index);
	}
	
	// Write into page
	write_to_page(p, c);

	printf("Writed char %c in index %d\n", c, p->id);
} 

// Function reads the data from a single page in main memory
void memory_read(Queue * q, char ** secondary, int index, int method) {
	Page * p;
	int seek_res = page_exists(q, index); // Result of page search (main memory), later used to determine R bit
	
	// Page not found in the main memory -> Pull out of secondary memory
	if (seek_res == 0) {
		if (method == 0) fifo_second_chance(q, secondary, index);
		else if (method == 1) fifo_standard(q, secondary, index);
	}
	
	// Extract page to procceed reading
	p = find_page(q, index);

	// Determine R bit (if page found originally - set 1)
	if (seek_res == 1) {
		if (method == 0) p->r_bit = 1; // Determine R bit (if page found originally - set 1)
		else if (method  == 1) lru(q, index);
	}
	
	printf("Data at index %d is [%s]\n",p->id, p->data);
}
/////////////////////////////////////////////////

// Function converts digit character to integer
int char_to_int(char c) {
	int res = c - '0';
	if (res >= 0 && res < 10) return res;
	return -1;
}

// Function converts numeric string to integer
int str_to_int(char * str) {
	int res = 0;
	int d;
	if (str == NULL) return -1;
	for (int i = 0; i < strlen(str); i++) {
		d = char_to_int(str[i]);
		if (d == -1) return -1;
		res = res * 10 + d;
	}
	return res;
}
// Function converts secondary memory into one char array (string)
char * sec_mem_to_str(char ** secondary, int size){
	int i;
	char * str = (char*)malloc(sizeof(char) * SEC_MEM_BUFFER_SIZE);
	str[0] = '\0';
	strcat(str, "Secondary memory={ ");
	for (i = 0; i < size - 1; i++) {
		strcat(str,secondary[i]);
		strcat(str,", ");
	}
	strcat(str,secondary[i]);
	strcat(str,"}\n\0");
	return str;
}

// Function parses command line into tokens (for execution)
char ** commandParser(char * commandLine) {
	int pos = 0;
	char * token;
	char ** tokens = (char**)malloc(sizeof(char*) * 64);
	token = strtok(commandLine, TOKEN_DELIMITERS);
	while (token != NULL) {
		tokens[pos] = token;
		pos++;
		token = strtok(NULL, TOKEN_DELIMITERS);
	}

	tokens[pos] = NULL;
	return tokens;
}

// Function analyzes user input command (returns 3 values: command integer, input character and index)
int * analyze_command(char * command) {
	int * res = (int*)malloc(sizeof(int) * 3);
	char ** tokens;
	int temp_i;
	tokens = commandParser(command);
	if (strcmp(tokens[0], "write") == 0) {
		temp_i = str_to_int(tokens[1]);
		if (temp_i == -1 || tokens[2] == NULL || strlen(tokens[2]) > 1 || tokens[3] != NULL) { 
			res[0] = -1;
			return res;
		}
		res[0] = 1;
		res[2] = (int) tokens[2][0];
		res[1] = temp_i;
	}
	else if (strcmp(tokens[0], "read") == 0) {
		temp_i = str_to_int(tokens[1]);
		if (temp_i == -1 || tokens[2] != NULL) { 
			res[0] = -1;
			return res;
		}
		res[0] = 0;
		res[1] = temp_i;		
	}
	return res;
}


// Main
int main (int argc, int **argv) {
	FILE * in_file_ptr, * out_file_ptr;
	int main_mem_size, sec_mem_size, method, r_w[3];
	char * command = (char*)malloc(sizeof(char) * 16);
	command[0] = '\0';
	size_t len = 0;
	int * res = (int*)malloc(sizeof(int) * 3);

	// User input & arguments extraction
	// ---------------------------------
	
	// Not enough arguments
	if (argv[1] == NULL || argv[2] == NULL || argv[3] == NULL || argv[4] == NULL || argv[5] == NULL ) {
		printf("Too few arguments.\n");
		printf("\tPlease enter ./memoryManagement <method> <input file> <output file> <main memory size> <secondary memory size>\n");
		printf("\tMethod: 1 - LRU, other integer (positive) value - FIFO Second Chance.\n");
		printf("\tMain memory size < Secondary memory size.\n");
		return 0;
	}
	
	// Invalid method input
	method = str_to_int((char*) argv[1]);
	if (method == -1) {
		printf("Illegal method value. Integer required.\n");
		return 0;
	}
	else if (method != 1) method = 0; //Default method
	
	// Main memory size must be larger than 4
	main_mem_size = str_to_int((char*) argv[5]);
	if (main_mem_size < 5) {
		printf("Main memory size must be integer larger than 4.\n");
		return 0; 
	}
	
	// Secondary memory size must be larger than 9
	sec_mem_size = str_to_int((char*) argv[4]);
	if (sec_mem_size < 10) {
		printf("Secondary memory size must be integer larger than 9.\n");
		return 0; 
	}
	
	// Input file check
	in_file_ptr = fopen((char*) argv[2],"r");
	if (in_file_ptr == NULL) {
		printf("Failure to create/find a file under the name: %s\n", (char*) argv[2]);
		return 0;
	}
	
	// Output file check
	out_file_ptr = fopen((char*) argv[3],"w");
	if (in_file_ptr == NULL) {
		printf("Failure to create/find a file under the name: %s\n", (char*) argv[3]);
		fclose(in_file_ptr);
		return 0;
	}
	
	// Init
	char ** secondary = (char**)malloc(sizeof(char*) * sec_mem_size);
	Queue * q = create_queue(main_mem_size);

	for (int i = 0; i < sec_mem_size; i++){
		secondary[i] = (char*)malloc(sizeof(char) * STR_BUFFER_SIZE);
		strcpy(secondary[i],"");
	}	

	fill_queue(q, secondary);
	
	// Commands extraction out of an input file
	while(getline(&command, &len, in_file_ptr) != -1) {
		if (command != NULL && strlen(command) != 0 && strlen(command) != 1) {
			command[strlen(command) - 1] = '\0';
			if (strcmp(command,"print") == 0) {
				fputs(sec_mem_to_str(secondary,sec_mem_size), out_file_ptr);
			}
			else {
				res = analyze_command(command);
				if (res[0] == -1) printf("Illegal command\n");
				if (res[1] >= sec_mem_size) printf("Illegal memory index : %d\n", res[1]);
				else if (res[0] == 1) memory_write(q, secondary, res[1], (char) res[2], method);
				else memory_read(q, secondary, res[1], method);
			}
		}
	}
	
	print_queue(q);

	// Memory release & Close files
	fclose(in_file_ptr);
	fclose(out_file_ptr);
	free(secondary);
	free(q);
	
	// End
	return 0;
}




