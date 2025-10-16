#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <inttypes.h>

#include "mml/eval.h"
#include "mml/expr.h"
#include "mml/parser.h"
#include "mml/config.h"
#include "mml/prompt.h"
#include "cvi/dvec/dvec.h"

extern strbuf expression;

void sig_handler(int32_t signum)
{
	MML_cleanup_state(MML_global_config.eval_state);
	term_restore();
	fprintf(stderr, "\n\nterminated with signal %d\n", signum);
	exit(2);
}

#define print_vec(p, n, fmt) { \
fputc('[', stdout); \
for (size_t i = 0; i < (n); ++i) \
	printf(fmt "%s", (p)[i], (i<(n)-1) ? ", " : ""); \
fputc(']', stdout); }

int32_t main(int32_t argc, char **argv)
{
	signal(SIGINT, sig_handler);
	signal(SIGTERM, sig_handler);
	signal(SIGQUIT, sig_handler);

	MML_global_config.eval_state = MML_init_state();
	MML_arg_parse(argc, argv);

	if (FLAG_IS_SET(RUN_PROMPT))
	{
		MML_run_prompt(MML_global_config.eval_state);
		MML_cleanup_state(MML_global_config.eval_state);
		return 0;
	}

	if (FLAG_IS_SET(READ_STDIN))
		expression = MML_read_string_from_stream(stdin);

	//Expr *expr = parse(expression.s);
	//eval_push_expr(&eval_state, expr);
	MML_expr_dvec exprs = MML_parse_stmts(expression.s);

	if (!FLAG_IS_SET(NO_EVAL))
	{
		MML_expr **cur;
		dv_foreach(exprs, cur)
		{
			MML_value val = MML_eval_expr(
					MML_global_config.eval_state,
					*cur);

			if ((size_t)(cur - _dv_ptr(exprs)) == dv_n(exprs)-1 && FLAG_IS_SET(PRINT))
				MML_print_typedval(MML_global_config.eval_state, &val);
		}
	}
	dv_destroy(exprs);

	//if (expression.allocd)
	//	free(expression.s);
	MML_cleanup_state(MML_global_config.eval_state);

	return 0;
}
