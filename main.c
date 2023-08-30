#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/wait.h>
#include<sys/types.h>
#include<unistd.h>


#define BYTE_TOK_BUFSIZE 64
#define BYTE_TOK_DELIM " \t\r\n\a"

/*
  Function Declarations for builtin shell commands:
*/
int byte_cd(char **args);
int byte_help(char **args);
int byte_exit(char **args);
int byte_bg(char **args);
int byte_history(char **args);

/*
  List of builtin commands, followed by their corresponding functions.
 */
char *builtin_str[] = {"cd","help","exit","bg","history"};
int (*builtin_func[]) (char **) = {&byte_cd, &byte_help,&byte_exit, &byte_bg , &byte_history};

int byte_num_builtins() {
  return sizeof(builtin_str) / sizeof(char *);
}
/*
  Builtin function implementations.
*/
int byte_cd(char **args)
{
  if (args[1] == NULL) {
    fprintf(stderr, "byte: expected argument to \"cd\"\n");
  } 
  else{
    if(chdir(args[1]) != 0){
      perror("byte");
    }
  }
  return 1;
}

int byte_help(char **args){
  int i;
  printf("Welcome to Byteshell\n");
  printf("Type program names and arguments, and hit enter.\n");
  printf("The following are built in:\n");

  for (i = 0; i < byte_num_builtins(); i++) {
    printf("  %s\n", builtin_str[i]);
  }

  printf("Use the man command for information on other programs.\n");
  return 1;
}

int byte_exit(char **args){
  return 0;
}

int byte_bg(char **args){
  ++args;
  char *firstCmd = args[0]; // echo
  int childpid = fork();
  if (childpid >= 0){
    if (childpid == 0){
      if (execvp(firstCmd, args) < 0){
        perror("Error on execvp\n");
        exit(0);
      }
    }
  }
  else{
    perror("fork() error");
  }
  return 1;
}

struct Node{
  char *str;
  struct Node* next;
};

struct Node* head = NULL;
struct Node* cur = NULL;

//implementing utility functions for history:
char* strAppend(char* str1, char* str2){
	char* str3 = (char*)malloc(sizeof(char*)*(strlen(str1)+strlen(str2)));
  strcpy(str3, str1);
  strcat(str3, str2);
	return str3;
}

void add_to_hist(char **args){
  if(head==NULL){
    head = (struct Node *)malloc(sizeof(struct Node));
    head->str = (char *)malloc(0x1000);
    char *str1 = " ";
    if (args[1] == NULL) 
      strcpy(head->str,strAppend(args[0],str1));

    else{  
      strcpy(head->str,strAppend(args[0],str1));
      strcpy(head->str, strAppend(head->str, args[1]));
    } 
    head->next = NULL;
    cur = head;
  }
  else{
    struct Node *ptr = (struct Node *)malloc(sizeof(struct Node));
    cur->next = ptr;
    ptr->str = (char *)malloc(0x1000);
    char *str1 = " ";
    if (args[1] == NULL) 
     strcpy(ptr->str,strAppend(args[0],str1));
    else{  
      strcpy(ptr->str,strAppend(args[0],str1));
      strcpy(ptr->str, strAppend(ptr->str, args[1]));
    }
    ptr->next = NULL;
    cur = ptr;
  }
}

int byte_history(char **args){  
  struct Node* ptr = head;
  int i = 1;
  while (ptr != NULL){
    printf(" %d %s\n",i++,ptr->str);
    ptr = ptr->next;
  }
  return 1; 
}

int byte_launch(char **args)
{
  pid_t pid, wpid;
  int status;
  pid = fork();
  if(pid == 0){
    // Child process
    if(execvp(args[0], args) == -1){
      perror("byte");
    }
    exit(EXIT_FAILURE);
  } 
  else if(pid < 0){
    // Error forking
    perror("byte");
  } 
  else{
    // Parent process
    do{
      wpid = waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }
  return 1;
}

int byte_execute(char **args)
{
  int i;
  if (args[0] == NULL) {
    // An empty command was entered.
    return 1;
  }

  for (i = 0; i < byte_num_builtins(); i++) {
    if (strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func[i])(args);
    }
  }
  return byte_launch(args);
}

char **byte_split_line(char *line){
  int bufsize = BYTE_TOK_BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token;

  if (!tokens) {
    fprintf(stderr, "byte: allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, BYTE_TOK_DELIM);
  while (token != NULL) {
    tokens[position] = token;
    position++;

    if(position >= bufsize) {
      bufsize += BYTE_TOK_BUFSIZE;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) {
        fprintf(stderr, "byte: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, BYTE_TOK_DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}

char* byte_read_line(void){
  char *line = NULL;
  ssize_t bufsize = 0; // have getline allocate a buffer for us
  if (getline(&line, &bufsize, stdin) == -1){
    if(feof(stdin)){
      exit(EXIT_SUCCESS);  // We recieved an EOF
    } 
    else{
      perror("readline");
      exit(EXIT_FAILURE);
    }
  }
  return line;
}

void byte_loop(void){
  char *line;
  char **args;
  int status;
  do{
    printf("> ");
    line = byte_read_line();
    args = byte_split_line(line);
    status = byte_execute(args);

    free(line);
    free(args);
  } while (status);
}

int main(){
    byte_loop();
    return 0;
}