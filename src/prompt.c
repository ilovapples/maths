#include "mml/prompt.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>

#include "old_std_compat.h"

#include "mml/config.h"
#include "mml/expr.h"
#include "mml/eval.h"
#include "mml/parser.h"
#include "dvec/dvec.h"

#define NSEC_IN_SEC 1000000000ULL
#define PROMPT_STR "\033[1;33m>>\033[0m "
#define PROMPT_STR_LEN 3
#define LINE_MAX_LEN 4096

#define time_blck(elapsed_p, ...) { \
	const clock_t start = clock(); \
	{ __VA_ARGS__; }; \
	*(elapsed_p) = (uint64_t)((double)(clock() - start) / CLOCKS_PER_SEC * NSEC_IN_SEC); \
}

static struct termios orig_termios;

void term_restore(void) {
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
	raw.c_lflag &= ~(ECHO | ICANON);
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

static inline void insert_char(char *buf, size_t len, size_t *cursor, size_t *end, char c) {
	if (*end >= len - 1) return;
	memmove(buf + *cursor + 1, buf + *cursor, *end - *cursor);
	buf[(*cursor)++] = c;
	(*end)++;
}

static inline void delete_char(char *buf, size_t *cursor, size_t *end) {
	if (*cursor == 0) return;
	memmove(buf + *cursor - 1, buf + *cursor, *end - *cursor);
	(*cursor)--;
	(*end)--;
}

static void redraw_line(const char *buf, size_t cursor) {
    printf("\x1b[?25l");
    printf("\r\x1b[2K");
    printf("%s%s", PROMPT_STR, buf);
    printf("\r\x1b[%zuC", PROMPT_STR_LEN + cursor);
    printf("\x1b[?25h");
    fflush(stdout);
}

#define isABCD(c) ((c) >= 'A' && (c) <= 'D')
#define UP_C "A"
#define DOWN_C "B"
#define RIGHT_C "C"
#define LEFT_C "D"
#define START_ANSI "\x1b["
#define CTRL_ANSI "\x1b[1;5"
#define ALT_ANSI "\x1b[1;3"
static bool handle_escape_seq(const char *seq, size_t seq_len, size_t *cursor, char *out, size_t *line_len) {

	if (seq_len == 3 && strncmp(seq, START_ANSI LEFT_C, seq_len) == 0) { // if LEFT
		if (*cursor > 0) (*cursor)--;
	} else if (seq_len == 3 && strncmp(seq, START_ANSI RIGHT_C, seq_len) == 0) { // if RIGHT
		if (*cursor < *line_len) (*cursor)++;
	} else if ((seq_len == 3 && strncmp(seq, START_ANSI UP_C, seq_len) == 0) // if UP
			|| (seq_len == 6 && strncmp(seq, ALT_ANSI LEFT_C, seq_len) == 0) // or ALT+LEFT
			|| (seq_len == 6 && strncmp(seq, CTRL_ANSI LEFT_C, seq_len) == 0)) { // or CTRL+LEFT
		*cursor = 0;
	} else if ((seq_len == 3 && strncmp(seq, START_ANSI DOWN_C, seq_len) == 0) // if DOWN
			|| (seq_len == 6 && strncmp(seq, ALT_ANSI RIGHT_C, seq_len) == 0) // or ALT+RIGHT
			|| (seq_len == 6 && strncmp(seq, CTRL_ANSI RIGHT_C, seq_len) == 0)) { // or CTRL+RIGHT
		*cursor = (line_len == 0) ? 0 : *line_len-1;
	} else if ((seq_len == 4 && strncmp(seq, START_ANSI "3~", seq_len) == 0)) { // if DEL
		char *const cursor_ptr = out + *cursor;
		if (*line_len - *cursor > 0) { // if in the middle of the line
			memmove(cursor_ptr, cursor_ptr+1, (*line_len)-- - *cursor);
		} else if (*line_len > 0) { // if at end of the line
			out[--*line_len] = '\0';
			*cursor = *line_len;
		}
	} else {
		return false;
	}

	return true;
}

ssize_t get_prompt_line(char *out, size_t len) {
	size_t cursor = 0;
	size_t line_len = 0;
	out[line_len] = '\0';

	redraw_line(out, cursor);

	char seq[8] = {0};
	char *seq_last = seq;
	unsigned char c;

	while (line_len < len) {
		if (read(STDIN_FILENO, &c, 1) <= 0)
			return -1;
		
		// check if the input character is part of an escape sequence
		const bool empty_esc_seq = seq_last == seq;
		const bool second_char_is_correct = 
			   (seq_last == seq+1 && c == '[')
			|| (seq_last > seq+1 && seq[1] == '[');
		const bool is_esc_seq_char = (empty_esc_seq && c == 0x1b) || second_char_is_correct;
		if (is_esc_seq_char) {
			*seq_last++ = c;

			const size_t seq_len = seq_last - seq;
			const bool is_short_seq = (seq_len == 3 && isABCD(seq[2])); // arrow keys
			const bool is_medium_seq = (seq_len == 4 && seq[2] == '3'); // delete
			const bool is_long_seq = (seq_len == 6 && seq[2] == '1'); // (alt or ctrl) + arrow keys
			if ((is_short_seq || is_medium_seq || is_long_seq)
					&& seq[0] == 0x1b && seq[1] == '[') {
				handle_escape_seq(seq, seq_len, &cursor, out, &line_len);
				*(seq_last = seq) = '\0';
				redraw_line(out, cursor);
				continue;
			}

			if (seq_last >= seq + sizeof(seq) - 1) seq_last = seq+1;
			continue;
		}

		if (c == '\n') {
			putchar('\n');
			break;
		} else if (c == 0x04) {
			return -1;
		} else if (c == 0x7f || c == 0x08) {
			delete_char(out, &cursor, &line_len);
		} else if (isprint(c)) {
			insert_char(out, len, &cursor, &line_len, c);
		}

		out[line_len] = '\0';
		redraw_line(out, cursor);
	}

	out[line_len] = '\0';
	return (ssize_t)line_len;
}

void MML_run_prompt(MML_state *state) {
	term_raw_mode();

	char line_in[LINE_MAX_LEN + 1] = {0};
	MML_value cur_val = VAL_INVAL;

	printf("-- MML Interactive REPL v0.0.3 --\n");
	printf("Type \033[1mexit\033[0m or press \033[1mCtrl+D\033[0m to quit.\n\n");
	fflush(stdout);

	ssize_t n_read = 0;

	while (!(cur_val.type == Invalid_type && cur_val.i == MML_QUIT_INVAL)) {
		memset(line_in, 0, sizeof(n_read));
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

		MML_expr **cur;

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
