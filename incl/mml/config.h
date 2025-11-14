#ifndef CONFIG_H
#define CONFIG_H

#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>

#include "old_std_compat.h"
#include "cpp_compat.h"

#include "mml/token.h"

MML__CPP_COMPAT_BEGIN_DECLS

#define BIT(n) (1<<n)
enum {
	DEBUG		= BIT(0),
	PRINT		= BIT(1),
	READ_STDIN	= BIT(2),
	BOOLS_PRINT_NUM = BIT(3),
	NO_EVAL	= BIT(4),
	RUN_PROMPT	= BIT(5),
	DBG_TIME	= BIT(6),
};

#define SET_FLAG(f) (MML_global_config.runtime_flags |= (f))
#define CLEAR_FLAG(c_p, f) (MML_global_config.runtime_flags &= ~(f))
#define FLAG_IS_SET(f) ((MML_global_config.runtime_flags & (f)) != 0)

#define CSET_FLAG(c_p, f) ((c_p)->runtime_flags |= (f))
#define CCLEAR_FLAG(c_p, f) ((c_p)->runtime_flags &= ~(f))
#define CFLAG_IS_SET(c_p, f) (((c_p)->runtime_flags & (f)) != 0)

typedef struct MML_state MML_state;

struct MML_config {
	char *PROG_NAME;
	uint32_t precision;
	uint32_t runtime_flags;
	MML_state *eval_state;
	bool last_print_was_newline;
	bool full_prec_floats;
};
extern struct MML_config MML_global_config;

void MML_print_usage(void);
void MML_arg_parse(int32_t argc, char **argv);

strbuf MML_read_string_from_stream(FILE *stream);
strbuf strbuf_dup(strbuf buf);

enum LOG_TYPE {
	MML_LOG_DEBUG,
	MML_LOG_ERROR,
	MML_LOG_WARN,
};
static inline void MML_dbg_print_func(
		const char *filename,
		size_t line_n,
		FILE *stream,
		enum LOG_TYPE log_type,
		char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	const char *log_type_str = "DEBUG";

	switch (log_type) {
	case MML_LOG_ERROR:
		log_type_str = "\x1b[38;5;160mERROR\x1b[0m";
		break;
	case MML_LOG_WARN:
		log_type_str = "\x1b[38;5;214mWARN\x1b[0m";
		break;
	case MML_LOG_DEBUG:
		if (!FLAG_IS_SET(DEBUG))
			return;
		break;
	}

	fprintf(stream, "[%s %s:%zu] ", log_type_str, filename, line_n);
	vfprintf(stream, fmt, args);

	fflush(stream);

	va_end(args);
}
#define MML_log(log_type, fmt, ...) (MML_dbg_print_func(__FILE_NAME__, __LINE__, stderr, log_type, fmt, ##__VA_ARGS__))
#define MML_log_dbg(fmt, ...) (MML_dbg_print_func(__FILE_NAME__, __LINE__, stderr, MML_LOG_DEBUG, fmt, ##__VA_ARGS__))
#define MML_log_err(fmt, ...) (MML_dbg_print_func(__FILE_NAME__, __LINE__, stderr, MML_LOG_ERROR, fmt, ##__VA_ARGS__))
#define MML_log_warn(fmt, ...) (MML_dbg_print_func(__FILE_NAME__, __LINE__, stderr, MML_LOG_WARN, fmt, ##__VA_ARGS__))

MML__CPP_COMPAT_END_DECLS

#endif /* CONFIG_H */
