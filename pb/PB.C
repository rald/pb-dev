#include <stdio.h>
#include <stdlib.h>
#include <conio.h>

#define MEM_MAX 30000
#define CALL_STK_MAX 256
#define PROC_MAX 256

char *slurp(const char *filename, long *file_size) {
  FILE *fp;
  char *buffer;

  fp = fopen(filename, "rb");
  if (fp == NULL) {
    return NULL;
  }

  fseek(fp, 0L, SEEK_END);
  *file_size = ftell(fp);
  rewind(fp);

  buffer = (char *)malloc(*file_size + 1);
  if (buffer == NULL) {
    fclose(fp);
    return NULL;
  }
  
  fread(buffer, sizeof(char), *file_size, fp);
  buffer[*file_size] = '\0';
  
  fclose(fp);
  return buffer;
}

void print_error(const char *buf, long pos, const char *msg) {
  long line = 1;
  long col = 1;
  long i;
  for (i = 0; i < pos && buf[i] != '\0'; i++) {
    if (buf[i] == '\n') {
      line++;
      col = 1;
    } else {
      col++;
    }
  }
  fprintf(stderr, "Error at %ld:%ld: %s\n", line, col, msg);
}

long find_matching_forward(const char *buf, long start, char open_char, char close_char) {
  int depth = 1;
  long i = start;
  while (buf[i] != '\0') {
    if (buf[i] == open_char) {
      depth++;
    } else if (buf[i] == close_char) {
      depth--;
      if (depth == 0) return i;
    }
    i++;
  }
  return -1;
}

long find_matching_backward(const char *buf, long start, char open_char, char close_char) {
  int depth = 1;
  long i = start;
  while (i >= 0) {
    if (buf[i] == close_char) {
      depth++;
    } else if (buf[i] == open_char) {
      depth--;
      if (depth == 0) return i;
    }
    i--;
  }
  return -1;
}

int main(int argc, char *argv[]) {
  char *c = NULL;
  long cn = 0;
  long cp = 0;

  unsigned char m[MEM_MAX];
  long mp = 0;

  long ps[PROC_MAX];

  long cs[CALL_STK_MAX];
  int csp = 0;

  int i;
  char op;
  char ch;
	int depth;
  unsigned char pocket = 0;

  for (i = 0; i < MEM_MAX; i++) {
    m[i] = 0;
  }
  for (i = 0; i < CALL_STK_MAX; i++) {
    cs[i] = 0;
  }
  for (i = 0; i < PROC_MAX; i++) {
    ps[i] = -1;
  }

  if (argc < 2) {
    fprintf(stderr, "Usage: %s <source.pb>\n", argv[0]);
    return 1;
  }

  c = slurp(argv[1], &cn);
  if (c == NULL) {
    fprintf(stderr, "Error: Could not open or read file '%s'\n", argv[1]);
    return 1;
  }

  while (cp < cn && c[cp] != '\0') {
		long match;
		long id;

  	op=c[cp];
  	switch(op) {
  		case '.': putchar(m[mp]); break;
  		case ',': ch=getchar(); m[mp]=(unsigned char)(ch==EOF?0:ch); break;
  		case '+': m[mp]++; break;
  		case '-': m[mp]--; break;
  		case '<': mp--; break;
  		case '>': mp++; break;
  		case '[':
				if (m[mp] == 0) {
          match = find_matching_forward(c, cp + 1, '[', ']');
          if (match == -1) {
            print_error(c, cp, "Unmatched '['");
            free(c);
            return 1;
          }
          cp = match;
        }  			
        break;
  		case ']':
				if (m[mp] != 0) {
          match = find_matching_backward(c, cp - 1, '[', ']');
          if (match == -1) {
            print_error(c, cp, "Unmatched ']'");
            free(c);
            return 1;
          }
          cp = match;
        }
				break;
			case '(':
				id = m[mp];
        if (id >= PROC_MAX) {
          print_error(c, cp, "Procedure ID out of bounds");
          free(c);
          return 1;
        }
        ps[id] = cp + 1;
        match = find_matching_forward(c, cp + 1, '(', ')');
        if (match == -1) {
          print_error(c, cp, "Unmatched '('");
          free(c);
          return 1;
        }
        cp = match; /* Skip over procedure definition during normal flow */
        break;
			case ':': 
				id = m[mp];
        if (id >= PROC_MAX || ps[id] == -1) {
          print_error(c, cp, "Calling undefined procedure");
          free(c);
          return 1;
        }
        if (csp >= CALL_STK_MAX) {
          print_error(c, cp, "Call stack overflow");
          free(c);
          return 1;
        }
        cs[csp++] = cp;
        cp = ps[id] - 1;
 				break;
      case ')':
			case ';':
				if (csp > 0) {
          cp = cs[--csp];
        } else {
          print_error(c, cp, "Return outside of procedure call");
          free(c);
          return 1;
        }
				break;
      case '$':
        if(pocket==0) {
          pocket=m[mp];
        } else {
          m[mp]=pocket;
        }
        break;
  		case '@':
  			free(c);
  			return m[mp];
  		default:
  			break;
  	}
    cp++;
  }

  free(c);
  return 0;
}
