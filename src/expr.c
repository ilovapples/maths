#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "arena/arena.h"
#include "mml/parser.h"
#include "mml/eval.h"
#include "mml/expr.h"
#include "mml/config.h"

#define PRINT_INDENT(_i) printf("%*s", (_i), "")

MML_value MML_print_typedval(MML_state *state, const MML_value *val)
{
	if (val == nullptr)
	{
		printf("(null)");
		return NOTHING_VAL;
	}
	switch (val->type) {
	case Nothing_type: break;
	case Integer_type:
		printf("%" PRIi64, val->i);
		break;
	case RealNumber_type:
		if (state->config->full_prec_floats)
			printf("%.*f", state->config->precision, val->n);
		else
			printf("%.*g", state->config->precision, val->n);
		break;
	case ComplexNumber_type:
		if (state->config->full_prec_floats)
			printf("%.*f%+.*fi",
					state->config->precision, creal(val->cn),
					state->config->precision, cimag(val->cn));
		else
			printf("%.*g%+.*gi",
					state->config->precision, creal(val->cn),
					state->config->precision, cimag(val->cn));
		break;
	case Boolean_type:
		if (FLAG_IS_SET(BOOLS_PRINT_NUM))
		{
			if (state->config->full_prec_floats)
				printf("%.*f",
						state->config->precision, (val->b) ? 1.0 : 0.0);
			else
				printf("%.*g",
						state->config->precision, (val->b) ? 1.0 : 0.0);
		} else
			printf("%s", (val->b) ? "true" : "false");
		break;
	case Identifier_type:
		printf("%.*s", (int)val->s.len, val->s.s);
		break;
	case Vector_type:
		fputc('[', stdout);
		MML_value cur_val;
		for (size_t i = 0; i < val->v.n; ++i)
		{
			cur_val = MML_eval_expr(state, val->v.ptr[i]);
			MML_print_typedval(state, &cur_val);
			if (i < val->v.n-1)
				fputs(", ", stdout);
		}
		fputc(']', stdout);
		break;
	case FuncObject_type:
		fputs("FuncObject", stdout);
		break;
	default:
		fputs("(null)", stdout);
		break;
	}

	state->config->last_print_was_newline = false;
	return NOTHING_VAL;
}

inline MML_value MML_println_typedval(MML_state *state, const MML_value *val)
{
	MML_value ret = MML_print_typedval(state, val);
	fputc('\n', stdout);
	state->config->last_print_was_newline = true;
	return ret;
}
MML_value MML_print_typedval_multiargs(MML_state *state, MML_expr_vec *args)
{
	for (size_t i = 0; i < args->n; ++i)
	{
		MML_value cur_val = MML_eval_expr(state, args->ptr[i]);
		MML_print_typedval(state, &cur_val);
		if (i < args->n-1) fputc(' ', stdout);
	}

	return NOTHING_VAL;
}
MML_value MML_println_typedval_multiargs(MML_state *state, MML_expr_vec *args)
{
	for (size_t i = 0; i < args->n; ++i)
	{
		MML_value cur_val = MML_eval_expr(state, args->ptr[i]);
		MML_println_typedval(state, &cur_val);
	}
	if (args->n == 0)
		fputc('\n', stdout);

	return NOTHING_VAL;
}

void MML_print_expr(struct MML_config *config, const MML_expr *expr, uint32_t indent)
{
	PRINT_INDENT(indent);
	if (expr == nullptr)
	{
		printf("(null)\n");
		return;
	}
	switch (expr->type) {
	case Operation_type:
		printf("Operation(%s,\n", TOK_STRINGS[expr->o.op]);

		MML_print_expr(config, expr->o.left, indent+4);
		putchar(',');
		if (expr->o.right)
		{
			putchar('\n');
			MML_print_expr(config, expr->o.right, indent+4);
			putchar(',');
		}
		putchar('\n');
		PRINT_INDENT(indent);
		putchar(')');
		break;
	case Nothing_type: fputs("Nothing", stdout); break;
	case Integer_type:
		printf("Integer(%" PRIi64 ")", expr->i);
		break;
	case RealNumber_type:
		if (config->full_prec_floats)
			printf("Real(%.*f)", config->precision, expr->n);
		else
			printf("Real(%.*g)", config->precision, expr->n);
		break;
	case ComplexNumber_type:
		if (config->full_prec_floats)
			printf("Complex(%.*g%+.*gi)",
					config->precision, creal(expr->cn),
					config->precision, cimag(expr->cn));
		else
			printf("Complex(%.*g%+.*gi)",
					config->precision, creal(expr->cn),
					config->precision, cimag(expr->cn));
		break;
	case Boolean_type:
		if (FLAG_IS_SET(BOOLS_PRINT_NUM))
		{
			if (config->full_prec_floats)
				printf("Boolean(%.*f)",
						config->precision, (expr->b) ? 1.0 : 0.0);
			else
				printf("Boolean(%.*g)",
						config->precision, (expr->b) ? 1.0 : 0.0);
		} else
			printf("Boolean(%s)", (expr->b) ? "true" : "false");
		break;
	case Identifier_type:
		printf("Identifier('%.*s')", (int)expr->s.len, expr->s.s);
		break;
	case Vector_type:
		printf("Vector(n=%zu,\n", expr->v.n);
		for (size_t i = 0; i < expr->v.n; ++i)
		{
			MML_print_expr(config, expr->v.ptr[i], indent+4);
			fputs(",\n", stdout);
		}
		PRINT_INDENT(indent);
		putchar(')');
		break;
	case FuncObject_type:
		fputs("FuncObject(params=[", stdout);
		for (size_t i = 0; i < expr->fo.params.len; ++i)
		{
			const strbuf cur_param_name = expr->fo.params.ptr[i];
			printf("'%.*s'%s",
					(int)cur_param_name.len,
					cur_param_name.s,
					(i < expr->fo.params.len-1) ? ", " : "");
		}
		fputs("], body=", stdout);
		MML_print_expr(config, expr->fo.body, indent);
		PRINT_INDENT(indent);
		putchar(')');
		break;
	default:
		printf("Invalid()");
		break;
	}

	config->last_print_was_newline = false;
}

inline void MML_print_exprh(const MML_expr *expr)
{
	MML_print_expr(&MML_global_config, expr, 0);
	fputc('\n', stdout);
	MML_global_config.last_print_was_newline = true;
}
MML_value MML_print_exprh_tv_func(MML_state *state, MML_expr_vec *args)
{
	MML_print_expr(state->config, args->ptr[0], 0);
	fputc('\n', stdout);
	state->config->last_print_was_newline = true;

	return NOTHING_VAL;
}

inline void MML_free_pp(void *p)
{
	free(*(void **)p);
}

inline double MML_get_number(const MML_value *v)
{
	if (!VAL_IS_NUM(*v) || v->type == ComplexNumber_type)
		return NAN;
	return (v->type == RealNumber_type)
		? v->n
		: ((v->b) ? 1.0 : 0.0);
}
inline _Complex double MML_get_complex(const MML_value *v)
{
	if (!VAL_IS_NUM(*v))
		return NAN;
	return (v->type == ComplexNumber_type)
		? v->cn
		: MML_get_number(v) + 0.0*I;
}
