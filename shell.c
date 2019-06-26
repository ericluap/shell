#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

  *numOfTokens = index;

  return tokens;
}

int main(int argc, char *argv[]) {
  FILE *stream = getStream(argc, argv);

  char *path[] = {"/bin"};

  size_t numOfTokens = 0;

  char *line = NULL;
  size_t linecap = 0;
  ssize_t linelength;
  while((linelength = getline(&line, &linecap, stream)) > 0) {
    char **input = tokenize(line, linelength, &numOfTokens);

    for(size_t i = 0; i < numOfTokens; i++) {
      printf("token: %s\n", input[i]);
    }

    free(input);
  }
  free(line);

  return 0;
}
