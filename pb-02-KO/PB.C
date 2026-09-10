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
	long pe[PROC_MAX];

	int cs[CALL_STK_MAX];
	int csp = CALL_STK_MAX;

	int i;
	char op;

	for (i = 0; i < MEM_MAX; i++) {
		m[i] = 0;
	}
	for (i = 0; i < CALL_STK_MAX; i++) {
		cs[i] = 0;
	}
	for (i = 0; i < PROC_MAX; i++) {
		ps[i] = -1;
		pe[i] = -1;
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
		long comment_start;
		long match;
		long id;
		int val;
		int x_coord;
		int y_coord;

		op = c[cp];

		/* Handle comments */
		if (op == '#') {
			while (cp < cn && c[cp] != '\n' && c[cp] != '\0') {
				cp++;
			}
			continue;
		}
		if (op == '/' && cp + 1 < cn && c[cp + 1] == '/') {
			cp++;
			while (cp < cn && c[cp] != '\n' && c[cp] != '\0') {
				cp++;
			}
			continue;
		}
		if (op == '/' && cp + 1 < cn && c[cp + 1] == '*') {
			comment_start = cp;
			cp += 2;
			while (cp < cn) {
				if (c[cp] == '*' && cp + 1 < cn && c[cp + 1] == '/') {
					cp++;
					break;
				}
				cp++;
			}
			if (cp >= cn) {
				print_error(c, comment_start, "Unterminated multi-line comment");
				free(c);
				return 1;
			}
			cp++;
			continue;
		}

		switch (op) {
			case '>':
				if (mp + 1 < MEM_MAX) {
					mp++;
				} else {
					print_error(c, cp, "Memory pointer overflow");
					free(c);
					return 1;
				}
				break;
			case '<':
				if (mp > 0) {
					mp--;
				} else {
					print_error(c, cp, "Memory pointer underflow");
					free(c);
					return 1;
				}
				break;
			case '+':
				m[mp]++;
				break;
			case '-':
				m[mp]--;
				break;
			case '.':
				putchar(m[mp]);
				fflush(stdout);
				break;
			case ',':
				val = getchar();
				if (val == EOF) {
					m[mp] = 0;
				} else {
					m[mp] = (unsigned char)val;
				}
				break;
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
				if (id < 0 || id >= PROC_MAX) {
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
				pe[id] = match;
				cp = match;
				break;
			case ')':
				if (csp < CALL_STK_MAX) {
					cp = cs[csp++];
				} else {
					print_error(c, cp, "Unmatched ')'");
					free(c);
					return 1;
				}
				break;
			case ':':
				id = m[mp];
				if (id < 0 || id >= PROC_MAX || ps[id] == -1) {
					print_error(c, cp, "Calling undefined procedure");
					free(c);
					return 1;
				}
				if (csp <= 0) {
					print_error(c, cp, "Call stack overflow");
					free(c);
					return 1;
				}
				cs[--csp] = cp;
				cp = ps[id] - 1;
				break;
			case ';':
				if (csp < CALL_STK_MAX) {
					cp = cs[csp++];
				} else {
					print_error(c, cp, "Return outside of procedure call");
					free(c);
					return 1;
				}
				break;
			case '!':
				switch(m[mp]) {
					case 0:
						clrscr();
						break;
					case 1:
						m[mp] = (unsigned char)getch();
						break;
					case 2:
						m[mp] = (unsigned char)kbhit();
						break;
					case 3:
						x_coord = (int)(mp + 1 < MEM_MAX ? m[mp + 1] : 0);
						y_coord = (int)(mp + 2 < MEM_MAX ? m[mp + 2] : 0);
						gotoxy(x_coord, y_coord);
						break;
				}
				break;
			case '@':
				free(c);
				return 0;
			default:
				break;
		}
		cp++;
	}

	free(c);
	return 0;
}
