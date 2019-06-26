#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

typedef struct strarray strarray;
struct strarray {
  char **array;
  size_t length;
};

void printStrarray(strarray input) {
  for(size_t i = 0; i < input.length; i++) {
    printf("%s\n", input.array[i]);
  }
}

typedef struct command_t command_t;
struct command_t {
  strarray input;
  char *output;
};

void printCommand(command_t command) {
  printf("command: \n");
  printStrarray(command.input);
  printf("%s\n", command.output);
  printf("--------------\n");
}

void printCommands(command_t *commands, size_t numOfCommands) {
  for(size_t i = 0; i < numOfCommands; i++) {
    printCommand(commands[i]);
  }
}

FILE* getStream(int argc, char *argv[]) {
  if(argc > 2) {
    exit(1);
  }
  else if(argc == 1) {
    return stdin;
  }
  else {
    FILE *stream = fopen(argv[1], "r");

    if(stream == NULL) {
      exit(1);
    }

    return stream;
  }
}

char** tokenize(char *line, size_t linelength, size_t *numOfTokens) {
  char **tokens;
  char *token;
  size_t index = 0;
  while((token = strsep(&line, " ")) != NULL) {
    tokens = realloc(tokens, (index+1)*sizeof(char *));
    tokens[index] = malloc(strlen(token)+1);
    strcpy(tokens[index], token);
    index++;
  }

  //tokens = realloc(tokens, (index+1)*sizeof(char *));
  //tokens[index] = malloc(1);
  //tokens[index] = NULL;

  *numOfTokens = index;

  return tokens;
}

void appendCommand(command_t **commands, size_t *numOfCommands, strarray commandString, char *output) {
  strarray *inputArray = malloc(sizeof(strarray));
  inputArray->array = malloc((commandString.length+1)*sizeof(char*));
  for(size_t i = 0; i < commandString.length; i++) {
    inputArray->array[i] = malloc(strlen(commandString.array[i])+1);
    strcpy(inputArray->array[i], commandString.array[i]);
  }

  inputArray->array[commandString.length] = malloc(1);
  inputArray->array[commandString.length] = NULL;

  inputArray->length = commandString.length;
  
  command_t *command = malloc(sizeof(command_t));
  command->input = *inputArray;
  command->output = output;
  *commands = realloc(*commands, (*numOfCommands+1)*sizeof(command_t));
  (*commands)[*numOfCommands] = *command;
  (*numOfCommands)++;
}

command_t* parseInput(strarray input, size_t *numOfCommands) {
  strarray commandString = {.array = calloc(1, sizeof(char)), .length = 0};
  command_t *commands = calloc(1, 1);
  *numOfCommands = 0;
  for(size_t i = 0; i < input.length; i++) {
    if(strncmp(input.array[i], "&",  1) == 0) {
       if(commandString.length != 0) {
          appendCommand(&commands, numOfCommands, commandString, "stdout");
          commandString.array = realloc(commandString.array, 0);
          commandString.length = 0;
       }
    }
    else if(strncmp(input.array[i], ">", 1) == 0) {
      // TODO: check i+1>input.length and throw error
      // TODO: also if input.array[i+2] does not exit or is != to "&" this is an error
      char *file = input.array[i+1];
      
      appendCommand(&commands, numOfCommands, commandString, file);

      commandString.array = realloc(commandString.array, 0);
      commandString.length = 0;
      
      i++;
    }
    else {
      commandString.array = realloc(commandString.array, (commandString.length+1)*sizeof(char *));
      commandString.array[commandString.length] = malloc(strlen(input.array[i])+1);
      strcpy(commandString.array[commandString.length], input.array[i]);
      commandString.length++;
    }
  }

  if(commandString.length != 0) {
    appendCommand(&commands, numOfCommands, commandString, "stdout");
    commandString.array = realloc(commandString.array, 0);
    commandString.length = 0;
  }

  return commands;
}

void error() {
  char error_message[30] = "An error has occurred\n";
  write(STDERR_FILENO, error_message, strlen(error_message)); 
}

void runExit(strarray input) {
  if(input.length > 1) {
    error();
  }
  else {
    exit(1);
  }
}

void runCd(strarray input) {
  if(input.length != 2) {
    error();
  }
  else {
    int flag = chdir(input.array[1]);

    if(flag != 0) {
      error();
    }
  }
}

void runPath(strarray input, strarray *path) {
  size_t numPathArguments = input.length-1;
  path->length = numPathArguments;
  path->array = realloc(path->array, numPathArguments*sizeof(char *));
  
  for(size_t i = 0; i < numPathArguments; i++) {
    path->array[i] = malloc(strlen(input.array[i+1])+1);
    strcpy(path->array[i], input.array[i+1]);
  }
}

void executeProgram(command_t command, char *filePath) {
  int rc = fork();
  if(rc < 0) {
    error();
  }
  else if(rc == 0) {
    if(strncmp(command.output, "stdout", 6) != 0) {
      close(STDERR_FILENO);
      open(command.output, O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU);
      close(STDOUT_FILENO);
      open(command.output, O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU);
    }
    execv(filePath, command.input.array); 
    error();
    exit(1);
  }
  else {
    int rc_wait = wait(NULL);
  }
}

void runCommandFromPath(command_t command, strarray path) {
  char *filePath = NULL;
  for(size_t i = 0; i < path.length; i++) {
    strarray input = command.input;
    filePath = realloc(filePath, strlen(path.array[i])+strlen(input.array[0])+2);
    filePath[0] = '\0';
    strcat(filePath, path.array[i]);
    strcat(filePath, "/");
    strcat(filePath, input.array[0]);

    if(access(filePath, X_OK) == 0) {
      executeProgram(command, filePath);
      free(filePath);
      return;
    }
  }
  free(filePath);

  error();
}

void runCommands(command_t *commands, size_t numOfCommands, strarray *path) {
  for(size_t i = 0; i < numOfCommands; i++) {
    strarray input = commands[i].input;
    char *name = input.array[0];

    if(strncmp(name, "exit", 4) == 0) {
      runExit(input);
    }
    else if(strncmp(name, "cd", 2) == 0) {
      runCd(input);
    }
    else if(strncmp(name, "path", 4) == 0) {
      runPath(input, path);
    }
    else {
      runCommandFromPath(commands[i], *path);
    }
  }
}

int main(int argc, char *argv[]) {
  FILE *stream = getStream(argc, argv);

  strarray path = {.array = calloc(1, sizeof(char *)),
                   .length = 1};
  path.array[0] = "/bin";

  size_t numOfTokens = 0;
  size_t numOfCommands = 0;

  char *line = NULL;
  size_t linecap = 0;
  ssize_t linelength;
  printf("wish> ");
  while((linelength = getline(&line, &linecap, stream)) > 0) {
    line[strcspn(line, "\n")] = 0;

    strarray input = {.array = tokenize(line, linelength, &numOfTokens),
                      .length = numOfTokens};

    command_t *commands = parseInput(input, &numOfCommands); 

    runCommands(commands, numOfCommands, &path);

    free(input.array);

    printf("wish> ");
  }
  free(line);
  free(path.array);

  if(argc == 2) {
    fclose(stream);
  }

  return 0;
}
