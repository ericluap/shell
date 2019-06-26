#include <stdio.h>
#include <stdlib.h>

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

int main(int argc, char *argv[]) {
  FILE *stream = getStream(argc, argv);

  char *line = NULL;
  size_t linecap = 0;
  ssize_t linelength;
  while((linelength = getline(&line, &linecap, stream)) > 0) {
  }

  return 0;
}
