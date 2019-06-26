#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

  tokens = realloc(tokens, (index+1)*sizeof(char *));
  tokens[index] = malloc(1);
  tokens[index] = NULL;

  *numOfTokens = index;

  return tokens;
}

void error() {
  char error_message[30] = "An error has occurred\n";
  write(STDERR_FILENO, error_message, strlen(error_message)); 
}

void runExit(char **input, size_t numOfTokens) {
  if(numOfTokens > 1) {
    error();
  }
  else {
    exit(1);
  }
}

void runCd(char **input, size_t numOfTokens) {
  if(numOfTokens != 2) {
    error();
  }
  else {
    int flag = chdir(input[1]);

    if(flag != 0) {
      error();
    }
  }
}

void runPath(char **input, size_t numOfTokens, char ***path, size_t *pathLength) {
  size_t numPathArguments = numOfTokens-1;
  *pathLength = numPathArguments;
  *path = realloc(*path, numPathArguments*sizeof(char *));
  
  for(size_t i = 0; i < numPathArguments; i++) {
    (*path)[i] = malloc(strlen(input[i+1])+1);
    strcpy((*path)[i], input[i+1]);
  }
}

void executeProgram(char **input, char *filePath) {
  int rc = fork();
  if(rc < 0) {
    error();
  }
  else if(rc == 0) {
    execv(filePath, input); 
    error();
    exit(1);
  }
  else {
    int rc_wait = wait(NULL);
  }
}

void runCommandFromPath(char **input, size_t numOfTokens, char **path, size_t pathLength) {
  char *filePath = NULL;
  for(size_t i = 0; i < pathLength; i++) {
    filePath = realloc(filePath, strlen(path[i])+strlen(input[0])+2);
    filePath[0] = '\0';
    strcat(filePath, path[i]);
    strcat(filePath, "/");
    strcat(filePath, input[0]);

    if(access(filePath, X_OK) == 0) {
      executeProgram(input, filePath);
      free(filePath);
      return;
    }
  }
  free(filePath);

  error();
}

void runCommand(char **input, size_t numOfTokens, char ***path, size_t *pathLength) {
  char *name = input[0];

  if(strncmp(name, "exit", 4) == 0) {
    runExit(input, numOfTokens);
  }
  else if(strncmp(name, "cd", 2) == 0) {
    runCd(input, numOfTokens);
  }
  else if(strncmp(name, "path", 4) == 0) {
    runPath(input, numOfTokens, path, pathLength);
  }
  else {
    runCommandFromPath(input, numOfTokens, *path, *pathLength);
  }
}

int main(int argc, char *argv[]) {
  FILE *stream = getStream(argc, argv);

  char **path = calloc(1, sizeof(char *));
  path[0] = "/bin";
  size_t pathLength = 1;

  size_t numOfTokens = 0;

  char *line = NULL;
  size_t linecap = 0;
  ssize_t linelength;
  printf("wish> ");
  while((linelength = getline(&line, &linecap, stream)) > 0) {
    line[strcspn(line, "\n")] = 0;

    char **input = tokenize(line, linelength, &numOfTokens);

    runCommand(input, numOfTokens, &path, &pathLength);

    free(input);

    printf("wish> ");
  }
  free(line);
  free(path);

  if(argc == 2) {
    fclose(stream);
  }

  return 0;
}
