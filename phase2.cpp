#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <cstdlib>
using namespace std;

// Define tokens' grammer types
#define NULLTOK 0
#define BUILT_IN 1
#define CMD_NAME 2
#define ARG 3
#define PIPE 4
#define REDIR_IN 5
#define REDIR_OUT 6
#define IN_FILE	7 
#define OUT_FILE 8
#define UNDEF 9

// Max length of cmdline
#define CMDLEN 300

char tokens[CMDLEN][CMDLEN];
int numTok;
int grammer[CMDLEN];

int isAbsPath(char *str){
	return !strncmp(str, "/", 1) || !strncmp(str, "./", 2) || !strncmp(str, "../", 3);
}

int whichGrammer(char *str){
	int i;
	char builtIn[4][5] = {"cd", "exit", "fg", "jobs"};
	char redirIn[1][3] = {"<"};
	char redirOut[2][3] = {">", ">>"};

	// Check if is built-in cmd
	for (i = 0; i < 4; i++){
		if (strcmp(builtIn[i], str) == 0)
			return BUILT_IN;
	}

	// Check if is pipe
	if (strcmp("|", str) == 0)
		return PIPE;

	// Check if is redir symbol	
	for (i = 0; i < 2; i++){
		if (strcmp(redirOut[i], str) == 0)
			return REDIR_OUT;
	}
	if ( (strcmp(redirIn[0], str)) == 0 )
		return REDIR_IN;

	return UNDEF;
}

int isLegalCMDName(char *str){
	char key[10] = {62, 60, 124, 42, 33, 96, 39, 34};
	return !(strcspn(str, key) < strlen(str));
}

int isLegalArg(char *str){
	char key[10] = {62, 60, 124, 33, 96, 39, 34};
	return !(strcspn(str, key) < strlen(str));
}

int isLegalFileName(char *str){
	char key[10] = {62, 60, 124, 42, 33, 96, 39, 34};
	return !(strcspn(str, key) < strlen(str));
}

int fsm(int numTok, char tokens[CMDLEN][CMDLEN], int grammer[CMDLEN]){
	int i, break_loop;
	typedef enum
	{
		state_0, state_1, state_2, state_3, state_4,
		state_5, state_6, state_7, state_8, state_9,
		state_10, state_11, state_12, state_13, state_14
	} my_state_t;

	int accept[15] = 
	{ 1, 1, 1, 1, 0,
		1, 0, 1, 0, 1,
		0, 1, 0, 1, 0 
	};
	my_state_t state = state_0;
	break_loop = 0;
	for (i = 0; i < numTok; i++){
		//printf("tokens = %s\tgrammer = %d\tstate = %d\n", tokens[i], grammer[i], state);
		switch (state)
		{
			case state_0:
				if (grammer[i] == BUILT_IN){
					state = state_1;
					break;
				}
				if (  isLegalCMDName(tokens[i]) ){
					grammer[i] = CMD_NAME; 
					state = state_3;
					break;
				}
				state = state_14;
				break;
			case state_1:
				if ( isLegalArg(tokens[i]) ){
					grammer[i] = ARG;
					state = state_2;
					break;
				}
				state = state_14;
				break;
			case state_2:
				if ( isLegalArg(tokens[i]) ){
					grammer[i] = ARG;
					state = state_2;
					break;
				}
				state = state_14;
				break;
			case state_3:
				if ( grammer[i] == REDIR_IN ){
					state = state_4;
					break;
				}
				if ( grammer[i] == PIPE ){
					state = state_6;
					break;
				}
				if ( grammer[i] == REDIR_OUT ){
					state = state_10;
					break;
				}
				if ( isLegalArg(tokens[i]) ){
					grammer[i] = ARG;
					state = state_3;
					break;
				}
				state = state_14;
				break;
			case state_4:
				if ( isLegalFileName(tokens[i]) ){
					grammer[i] = IN_FILE;
					state = state_5;
					break;
				}
				state = state_14;
				break;
			case state_5:
				if ( grammer[i] == REDIR_OUT ){
					state = state_8;
					break;
				}
				if ( grammer[i] == PIPE ){
					state = state_6;
					break;
				}
				state = state_14;
				break;
			case state_6:
				if ( isLegalCMDName(tokens[i]) ){
					grammer[i] = CMD_NAME;
					state = state_7;
					break;
				}
				state = state_14;
				break;
			case state_7:
				if ( grammer[i] == PIPE ){
					state = state_6;
					break;
				}
				if ( grammer[i] == REDIR_OUT ){
					state = state_8;
					break;
				}
				if ( isLegalArg(tokens[i]) ){
					grammer[i] = ARG;
					state = state_7;
					break;
				}
				state = state_14;
				break;
			case state_8:
				if ( isLegalFileName(tokens[i]) ){
					grammer[i] = OUT_FILE;
					state = state_9;
					break;
				}
				state = state_14;
				break;
			case state_9:
				state = state_14;
				break;
			case state_10:
				if ( isLegalFileName(tokens[i]) ){
					grammer[i] = OUT_FILE;
					state = state_11;
					break;
				}
				state = state_14;
				break;
			case state_11:
				if ( grammer[i] == REDIR_IN ){
					state = state_12;
					break;
				}
				state = state_14;
				break;
			case state_12:
				if ( isLegalFileName(tokens[i]) ){
					grammer[i] = IN_FILE;
					state = state_13;
					break;
				}
				state = state_14;
				break;
			case state_13:
				state = state_14;
				break;
			case state_14:
				break_loop = 1;
				break;
		}
		if ( break_loop )
			break;
	}
	//test
	//printf("final state = %d, accept state = %d\n", state, accept[state]);
	return accept[state];
}

void create_process(){
	int i, j, k;
	char ** arg;
	char pathList[3][15] = {"/bin/", "/usr/bin/", "./"};
	arg = (char **) malloc(sizeof(char *) * numTok);
	for (i = 0; i < numTok; i++){
		if (grammer[i] == CMD_NAME || grammer[i] == BUILT_IN){
			memset(arg, 0, sizeof(arg));
			j = 0;
			arg[0] = tokens[i];
			
			while (grammer[i + j + 1] == ARG){
				arg[j + 1] = tokens[i + j + 1];
				j++;
			}

			if (fork() == 0){
				if (isAbsPath(arg[0]))
					execv(arg[0], arg);
				else{
					for (k = 0; k < 3; k++){
						execv(strcat(pathList[k], arg[0]), arg);
						if (errno != 2)
							break;
					}
				}
				if (errno == 2){
					printf("%s: command not found\n", arg[0]);
				}
				else{
					printf("%s: unknown error\n", arg[0]);
				}
				exit(-1);
			}
			wait(NULL);
		}
	}
}

int main(){
	char cmdline[CMDLEN];
	char path[999];
	int i;
	int numPipe;
	while (1){
		memset(cmdline, 0, sizeof(CMDLEN));
		memset(tokens, 0, sizeof(tokens));
		memset(grammer, 0, sizeof(grammer));
		memset(path, 0, sizeof(path));
		numPipe = 0;

		getcwd(path, sizeof(path));
		printf("[3150 shell:%s]$ ", path);

		fgets(cmdline, CMDLEN, stdin);
		// break if EOF is found, return non-zero if EOF
		if ( feof(stdin) != 0 ){
			printf("\n");
			break;
		}
		if (cmdline[strlen(cmdline) - 1] == '\n')
			cmdline[strlen(cmdline) - 1] = '\0';

		// create a list of tokens using strtok
		char *cptr = strtok(cmdline, " ");
		i = 0;
		while (cptr != NULL){
			strcpy(tokens[i], cptr); 
			grammer[i] = whichGrammer(tokens[i]);
			if (grammer[i] == PIPE)
				numPipe++;
			cptr = strtok(NULL, " ");
			i++;	
		}
		numTok = i;

		char grammerType[20][30] = {
			"", "Built-in Command", "Command Name",
			"Argument", "Pipe", "Redirect Input",
			"Redirect Output", "Input Filename", "Output Filename",
			""
		};
		// test
		//while(i--){	printf("%d %s\n", grammer[i], tokens[i]);	}
		if ( numPipe <= 2 && fsm(numTok, tokens, grammer) ){
			create_process();
		}
		else
			printf("Error: invalid input command line\n");

		//	printf("%s\n", fsm(numTok, tokens, grammer)?"VALID":"NOT VALID");

	}

	return 0;
}

