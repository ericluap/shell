#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct strarray strarray;
struct strarray {
  char **array;
  size_t length;
};

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

void runCommandFromPath(strarray input, strarray path) {
  char *filePath = NULL;
  for(size_t i = 0; i < path.length; i++) {
    filePath = realloc(filePath, strlen(path.array[i])+strlen(input.array[0])+2);
    filePath[0] = '\0';
    strcat(filePath, path.array[i]);
    strcat(filePath, "/");
    strcat(filePath, input.array[0]);

    if(access(filePath, X_OK) == 0) {
      executeProgram(input.array, filePath);
      free(filePath);
      return;
    }
  }
  free(filePath);

  error();
}

void runCommand(strarray input, strarray *path) {
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
    runCommandFromPath(input, *path);
  }
}

int main(int argc, char *argv[]) {
  FILE *stream = getStream(argc, argv);

  strarray path = {.array = calloc(1, sizeof(char *)),
                   .length = 1};
  path.array[0] = "/bin";

  size_t numOfTokens = 0;

  char *line = NULL;
  size_t linecap = 0;
  ssize_t linelength;
  printf("wish> ");
  while((linelength = getline(&line, &linecap, stream)) > 0) {
    line[strcspn(line, "\n")] = 0;

    strarray input = {.array = tokenize(line, linelength, &numOfTokens),
                      .length = numOfTokens};

    runCommand(input, &path);

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
