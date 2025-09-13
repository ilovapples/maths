#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "eval.h"
#include "expr.h"
#include "config.h"

const char *const TOK_STRINGS[] = {
	"OP_FUNC_CALL",
	"OP_DOT",
	"OP_AT_TOK",

	"OP_POW",

	"OP_MUL", "OP_DIV", "OP_MOD",
	"OP_ADD", "OP_SUB",

	"OP_LESS", "OP_GREATER",
	"OP_LESSEQ", "OP_GREATEREQ",
	"OP_EQ", "OP_NOTEQ",
	
	"OP_ASSERT_EQUAL",
	
	"OP_NOT",
	"OP_NEGATE",
	"OP_UNARY_NOTHING",
	"NOT_OP",


	"IDENT_TOK",
	"INSERTED_IDENT_TOK",
	"NUMBER_TOK",

	"DIGIT_TOK",
	"LETTER_TOK",
	"UNDERSCORE_TOK",

	"OPEN_PAREN_TOK",
	"CLOSE_PAREN_TOK",

	"OPEN_BRAC_TOK",
	"CLOSE_BRAC_TOK",

	"OPEN_BRACKET_TOK",
	"CLOSE_BRACKET_TOK",

	"DQUOTE_TOK",
	"SQUOTE_TOK",
	"BACKTICK_TOK",

	"COLON_TOK",
	"SEMICOLON_TOK",
	"COMMA_TOK",

	"HASHTAG_TOK",
	"QUESTION_TOK",
	"BACKSLASH_TOK",
	"DOLLAR_TOK",
	"AMPER_TOK",
	"PIPE_TOK",
	"TILDE_TOK",

	"INVALID_TOK",
	"EOF_TOK",
};

const char *const EXPR_TYPE_STRINGS[] = {
	"operation",
	"real number",
	"complex number",
	"boolean",
	"identifier",
	"inserted identifier",
	"vector",
	"invalid",
};


void print_indent(uint32_t indent)
{
	printf("%*s", indent, "");
}

TypedValue print_typedval(struct evaluator_state *, TypedValue *val)
{
	if (val == nullptr)
	{
		printf("(null)");
		return VAL_INVAL;
	}
	switch (val->type) {
	case RealNumber_type:
		printf("%.*f", global_config.precision, val->v.n);
		break;
	case ComplexNumber_type:
		printf("%.*f%+.*fi",
				global_config.precision, creal(val->v.cn),
				global_config.precision, cimag(val->v.cn));
		break;
	case Boolean_type:
		if (FLAG_IS_SET(BOOLS_PRINT_NUM))
			printf("%.*f",
					global_config.precision, (val->v.b) ? 1.0 : 0.0);
		else
			printf("%s", (val->v.b) ? "true" : "false");
		break;
	case Identifier_type:
	case InsertedIdentifier_type:
		printf("%.*s", (int)val->v.s.len, val->v.s.s);
		break;
	case Vector_type:
		print_vec(&val->v.v);
		break;
	default:
		printf("(null)");
		break;
	}

	return VAL_NUM(NAN);
}

inline TypedValue println_typedval(struct evaluator_state *state, TypedValue *val)
{
	TypedValue ret = print_typedval(state, val);
	fputc('\n', stdout);
	return ret;
}
TypedValue print_typedval_multiargs(struct evaluator_state *state, VecN *args)
{
	for (size_t i = 0; i < args->n; ++i)
	{
		TypedValue cur_val = eval_expr(state, args->ptr[i]);
		print_typedval(state, &cur_val);
		if (i < args->n-1) fputc(' ', stdout);
	}

	return VAL_NUM(NAN);
}
TypedValue println_typedval_multiargs(struct evaluator_state *state, VecN *args)
{
	for (size_t i = 0; i < args->n; ++i)
	{
		TypedValue cur_val = eval_expr(state, args->ptr[i]);
		println_typedval(state, &cur_val);
	}
	if (args->n == 0)
		fputc('\n', stdout);

	return VAL_NUM(NAN);
}

void print_expr(Expr *expr, uint32_t indent)
{
	print_indent(indent);
	if (expr == nullptr)
	{
		printf("(null)\n");
		return;
	}
	switch (expr->type) {
	case Operation_type:
		printf("Operation(%s):\n", TOK_STRINGS[expr->u.o.op]);
		print_indent(indent+2);

		printf("Left:\n");
		print_expr(expr->u.o.left, indent+4);
		if (expr->u.o.right)
		{
			fputc('\n', stdout);
			print_indent(indent+2);
			printf("Right:\n");
			print_expr(expr->u.o.right, indent+4);
		}
		break;
	case RealNumber_type:
		printf("RealNumber(%.*f)", global_config.precision, expr->u.v.n);
		break;
	case ComplexNumber_type:
		printf("ComplexNumber(%.*f%+.*fi)",
				global_config.precision, creal(expr->u.v.cn),
				global_config.precision, cimag(expr->u.v.cn));
		break;
	case Boolean_type:
		if (FLAG_IS_SET(BOOLS_PRINT_NUM))
			printf("Boolean(%.*f)",
					global_config.precision, (expr->u.v.b) ? 1.0 : 0.0);
		else
			printf("Boolean(%s)", (expr->u.v.b) ? "true" : "false");
		break;
	case Identifier_type:
		printf("Identifier('%.*s')", (int)expr->u.v.s.len, expr->u.v.s.s);
		break;
	case InsertedIdentifier_type:
		printf("InsertedIdentifier('%.*s')", (int)expr->u.v.s.len, expr->u.v.s.s);
		break;
	case Vector_type:
		printf("Vector(n=%zu):\n", expr->u.v.v.n);
		for (size_t i = 0; i < expr->u.v.v.n; ++i)
		{
			print_expr(expr->u.v.v.ptr[i], indent+2);
			if (i < expr->u.v.v.n - 1) fputc('\n', stdout);
		}
		break;
	default:
		printf("Invalid()");
		break;
	}
}

inline void print_exprh(Expr *expr)
{
	print_expr(expr, 0);
	fputc('\n', stdout);
}

void free_expr(Expr **e)
{
	if (*e == nullptr) return;
	if (--(*e)->num_refs > 0)
	{
		*e = nullptr;
		return;
	}
	if ((*e)->type == Operation_type)
	{
		free_expr(&(*e)->u.o.left);
		free_expr(&(*e)->u.o.right);
	} else if ((*e)->type == Identifier_type)
	{
		if ((*e)->u.v.s.allocd)
			free((*e)->u.v.s.s);
	} else if ((*e)->type == Vector_type)
	{
		if (!(*e)->should_free_vec_block)
			free_vec(&(*e)->u.v.v);
		else
		{
			free((*e)->u.v.v.ptr[0]);
			free((*e)->u.v.v.ptr);
			(*e)->u.v.v.ptr = nullptr;
		}
	}

	free(*e);
	*e = nullptr;
}

#ifndef NDEBUG
VecN new_vec_debug(size_t n, struct call_info call)
#else
VecN new_vec(size_t n)
#endif
{
#ifndef NDEBUG
	static size_t n_vecs_allocd = 0;
	if (call.filename != nullptr)
		printf("alloc'd vector #%zu (%s:%zu)\n",
			++n_vecs_allocd,
			call.filename, call.line_n);
#endif
	return (VecN) {
		.ptr = calloc(n, sizeof(Expr *)),
		.n = 0,
		.allocd_size = n,
	};
}

#ifndef NDEBUG
void free_vec_debug(VecN *vec, struct call_info call)
#else
void free_vec(VecN *vec)
#endif
{
	for (size_t i = 0; i < vec->n; ++i)
		free_expr(&vec->ptr[i]);
	free(vec->ptr);
#ifndef NDEBUG
	static size_t vector_free_count = 0;
	if (call.filename != nullptr)
		printf("freed vector #%zu (%s:%zu)\n",
			++vector_free_count,
			call.filename, call.line_n);
#endif
}

VecN construct_vec(size_t n, ...)
{
	VecN ret = {
		.ptr = calloc(n, sizeof(Expr *)),
		.n = n,
		.allocd_size = n
	};
	TypedValue cur;
	va_list args;
	va_start(args, n);
	for (size_t i = 0; i < n; ++i)
	{
		cur = va_arg(args, TypedValue);
		ret.ptr[i] = calloc(1, sizeof(Expr));
		ret.ptr[i]->type = cur.type;
		ret.ptr[i]->u.v = cur.v;
	}

	va_end(args);

	return ret;
}

int32_t push_to_vec(VecN *vec, Expr *elem)
{
	if (vec->allocd_size < (vec->n + 1))
	{
		Expr **tmp = realloc(vec->ptr,
				(vec->allocd_size*=2) * sizeof(Expr *));
		if (tmp == NULL)
		{
			fprintf(stderr, "could not resize vector memory\n");
			free_vec(vec);
			return 1;
		}
		vec->ptr = tmp;
	}
	vec->ptr[vec->n++] = elem;
	++elem->num_refs;
	return 0;
}

inline Expr **peek_top_vec(VecN *vec)
{
	Expr **ret = &vec->ptr[vec->n-1];
	++(*ret)->num_refs;
	return ret;
}

inline Expr *pop_from_vec(VecN *vec)
{
	return vec->ptr[--vec->n];
}

inline void print_vec(VecN *vec)
{
	print_expr(&(Expr) { Vector_type, 0, .u.v.v = *vec }, 0);
}
inline void println_vec(VecN *vec)
{
	print_expr(&(Expr) { Vector_type, 0, .u.v.v = *vec }, 0);
	fputc('\n', stdout);
}

inline void free_pp(void *p)
{
	free(*(void **)p);
}

inline double get_number(TypedValue *v)
{
	if (!VAL_IS_NUM(*v) || v->type == ComplexNumber_type)
		return NAN;
	return (v->type == RealNumber_type)
		? v->v.n
		: ((v->v.b) ? 1.0 : 0.0);
}
inline _Complex double get_complex(TypedValue *v)
{
	if (!VAL_IS_NUM(*v))
		return NAN;
	return (v->type == ComplexNumber_type)
		? v->v.cn
		: get_number(v) + 0.0*I;
}
