#include <stdio.h>
#include <string.h>

#include "writer.h"
#include "util.h"


#define INDENT do { \
  int i;\
  for(i = 0; i < indent; i ++ ) \
    fprintf(fout, " "); \
  }\
  while(0)


static char* filename;
static FILE* fout;


static int indent = 4;
static int indent_step = 4;

void writer_set_filename( char* fn) {
  if (filename == NULL) {
    filename = fn;
  }
}

int writer_ready() {
  if ( NULL != fout ) return 1;
  if (filename == NULL) return 0;
  
  if( (fout = fopen(filename, "w")) != NULL )
    return 1;

  return 0;
}


void writer_set_file(FILE* file) { fout = file; }
FILE* writer_get_file() { return fout ; }

void close_writer() {
  if ( NULL != fout )
    fclose(fout);
}



int get_indent_step() { return indent_step; }
int get_indent() { return indent; }

void add_indent(int step ) { indent += step; TEST_Z(indent >= 0);} 
void incr_indent() { indent += indent_step; TEST_Z(indent >= 0);}

void sub_indent(int step ) { indent -= step; TEST_Z(indent >= 0);}
void decr_indent() { indent -= indent_step; TEST_Z(indent >= 0);}

void new_line() {
  TEST_Z(fout != NULL);
  fprintf(fout, "\n");
}

void write_buffer(const char* buffer ) {
  TEST_Z(fout != NULL);
  INDENT;
  fprintf(fout, "%s", buffer);
}

void write_bufferln(const char* buffer ) {
  write_buffer(buffer);
  new_line();
}


void smart_write_buffer(const char* buffer ) {
  TEST_Z(buffer != NULL);
  
#define WHITESPACE(x) ((x) == ' ' || (x) == '\t' || (x) == '\n' || (x) == '\v')
  int i;
  for(i = strlen(buffer) - 1; WHITESPACE(buffer[i]); i -- )
    ;
  if(buffer[i] == '{') {
    write_bufferln(buffer);
    incr_indent();
    return;
  }
   
  for(i = 0; WHITESPACE(buffer[i]); i ++)
    ;

  if (buffer[i] == '}') {
    decr_indent();
    write_bufferln(buffer);
    return;
  }

  write_buffer(buffer);
}
