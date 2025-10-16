#include "mml/prompt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>

#include "mml/config.h"
#include "mml/expr.h"
#include "mml/eval.h"
#include "mml/parser.h"
#include "cvi/dvec/dvec.h"

#define NSEC_IN_SEC 1000000000ULL
#define PROMPT_STR "\033[1;34mMML\033[0m \033[1;33m»\033[0m "
#define LINE_MAX_LEN 4096

#define time_blck(elapsed_p, ...) { \
	const clock_t start = clock(); \
	{ __VA_ARGS__; }; \
	*(elapsed_p) = (uint64_t)((double)(clock() - start) / CLOCKS_PER_SEC * NSEC_IN_SEC); \
}

static struct termios orig_termios;

static void term_restore(void) {
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
	printf("\x1b[0 q");
	printf("\x1b[?25h");
	fflush(stdout);
}

static void term_raw_mode(void) {
	struct termios raw;
	tcgetattr(STDIN_FILENO, &orig_termios);
	atexit(term_restore);
	raw = orig_termios;
	raw.c_lflag &= ~(ECHO | ICANON | ISIG);
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

static inline void insert_char(char* buf, size_t len, size_t *cursor, size_t *end, char c) {
	if (*end >= len - 1) return;
	memmove(buf + *cursor + 1, buf + *cursor, *end - *cursor);
	buf[(*cursor)++] = c;
	(*end)++;
}

static inline void delete_char(char* buf, size_t *cursor, size_t *end) {
	if (*cursor == 0) return;
	memmove(buf + *cursor - 1, buf + *cursor, *end - *cursor);
	(*cursor)--;
	(*end)--;
}

static void redraw_line(const char* buf, size_t cursor) {
    printf("\x1b[?25l");
    printf("\r\x1b[2K");
    printf("%s%s", PROMPT_STR, buf);
    printf("\r\x1b[%zuC", 6 + cursor);
    printf("\x1b[?25h");
    fflush(stdout);
}

static bool handle_escape_seq(const char* seq, size_t *cursor, size_t end) {
	if (strcmp(seq, "\x1b[D") == 0) {
		if (*cursor > 0) (*cursor)--;
		return true;
	} else if (strcmp(seq, "\x1b[C") == 0) {
		if (*cursor < end) (*cursor)++;
		return true;
	} else if (strcmp(seq, "\x1b[A") == 0 || strcmp(seq, "\x1b[B") == 0) {
		return true;
	}

	return false;
}

ssize_t get_prompt_line(char* out, size_t len) {
	size_t cursor = 0;
	size_t end = 0;
	out[0] = '\0';

	redraw_line(out, cursor);

	char seq[8] = {0};
	int seq_len = 0;
	unsigned char c;

	while (1) {
		if (read(STDIN_FILENO, &c, 1) <= 0)
			return -1;
		
		if (seq_len > 0 || c == 0x1b) {
			seq[seq_len++] = c;
			seq[seq_len] = '\0';

			if ((seq_len == 3 && seq[0] == 0x1b && seq[1] == '[')) {
				handle_escape_seq(seq, &cursor, end);
				seq_len = 0;
				redraw_line(out, cursor);
				continue;
			}

			if (seq_len >= (int)sizeof(seq) - 1) seq_len = 0;
			continue;
		}

		if (c == '\n') {
			putchar('\n');
			break;
		} else if (c == 0x04 && end == 0) {
			return -1;
		} else if (c == 0x7f || c == 0x08) {
			delete_char(out, &cursor, &end);
		} else {
			insert_char(out, len, &cursor, &end, c);
		}

		out[end] = '\0';
		redraw_line(out, cursor);
	}

	out[end] = '\0';
	return (ssize_t)end;
}

void MML_run_prompt(MML_state* state) {
	term_raw_mode();

	char line_in[LINE_MAX_LEN + 1] = {0};
	MML_value cur_val = VAL_INVAL;

	printf("\033[1;36m╔════════════════════════════╗\033[0m\n");
	printf("\033[1;36m║  MML Interactive REPL v1.0 ║\033[0m\n");
	printf("\033[1;36m╚════════════════════════════╝\033[0m\n");
	printf("Type \033[1;33mexit\033[0m or press \033[1;33mCtrl+D\033[0m to quit.\n\n");
	fflush(stdout);

	ssize_t n_read = 0;

	while (!(cur_val.type == Invalid_type && cur_val.i == MML_QUIT_INVAL)) {
		memset(line_in, 0, sizeof(line_in));
		n_read = get_prompt_line(line_in, LINE_MAX_LEN);

		if (n_read == -1) break;
		if (n_read == 0) continue;

		uint64_t nsecs = 0;
		MML_expr_dvec exprs;

		if (!FLAG_IS_SET(DBG_TIME))
			exprs = MML_parse_stmts(line_in);
		else {
			time_blck(&nsecs, exprs = MML_parse_stmts(line_in));
			MML_log_dbg("parsed in %.6fs\n", (double)nsecs / NSEC_IN_SEC);
		}

		MML_expr** cur;

		if (!FLAG_IS_SET(DBG_TIME)) {
			dv_foreach(exprs, cur)
				if (*cur != NULL)
					cur_val = MML_eval_expr(state, *cur);
		} else {
			time_blck(&nsecs,
				dv_foreach(exprs, cur)
					if (*cur != NULL)
						cur_val = MML_eval_expr(state, *cur));
			
			MML_log_dbg("evaluted in %.6fs\n", (double)nsecs / NSEC_IN_SEC);
		}

		if (!state->config->last_print_was_newline)
			puts("\033[7m%\033[0m");
		
		state->config->last_print_was_newline = true;

		if (cur_val.type != Invalid_type) {
			printf("\033[2m────────────\033[0m\n");
			MML_println_typedval(state, &cur_val);
			printf("\033[2m────────────\033[0m\n");
			state->last_val = cur_val;
		} else {
			if (cur_val.i == MML_CLEAR_INVAL)
				printf("\033[2J\033[H");
		}

		dv_destroy(exprs);
		fflush(stdout);
	}

	term_restore();
}