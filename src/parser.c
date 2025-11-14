#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>

#include "mml/parser.h"
#include "mml/eval.h"
#include "mml/expr.h"
#include "mml/token.h"
#include "mml/config.h"
#include "arena/arena.h"
#include "dvec/dvec.h"

const char *const TOK_STRINGS[] = {
	"OP_FUNC_CALL",
	"OP_DOT",
	"OP_AT_TOK",

	"OP_POW",
	"OP_ROOT",

	"OP_MUL", "OP_DIV", "OP_MOD",
	"OP_ADD", "OP_SUB",

	"OP_LESS", "OP_GREATER",
	"OP_LESSEQ", "OP_GREATEREQ",
	"OP_EQ", "OP_NOTEQ",
	"OP_EXACT_EQ", "OP_EXACT_NOTEQ",
	
	"OP_ASSERT_EQUAL",
	
	"OP_NOT",
	"OP_NEGATE",
	"OP_UNARY_NOTHING",
	"TILDE_TOK",
	"NOT_OP",


	"IDENT_TOK",
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

	"INVALID_TOK",
	"WHITESPACE_TOK",
	"EOF_TOK",
};

const char *const EXPR_TYPE_STRINGS[] = {
	"invalid",
	"operation",
	"integer",
	"real number",
	"complex number",
	"boolean",
	"identifier",
	"vector",
	"function object",
};


// Gets the next token and advances the string pointer.
static MML_token get_next_token(const char **s, struct parser_state *state)
{
	MML_token ret = nToken(MML_INVALID_TOK, NULL, 0);

	if (state->has_peeked) {
		state->has_peeked = false;
		*s = state->saved_s;
		return state->peeked_tok;
	}

	const char *cached_s = *s;
	while (isspace(*cached_s)) ++cached_s;

	MML_token_type type = TOK_BY_CHAR[(unsigned char)*cached_s];
	switch (type) {
	case MML_EOF_TOK:
		*s = cached_s;
		return state->current_tok = nToken(MML_EOF_TOK, NULL, 0);
	case MML_OP_DOT_TOK:
	case MML_OP_AT_TOK:
	case MML_OP_POW_TOK:
	case MML_OP_MUL_TOK:
	case MML_OP_DIV_TOK:
	case MML_OP_MOD_TOK:
	case MML_OP_ADD_TOK:
	case MML_OP_SUB_TOK:
	case MML_OPEN_PAREN_TOK:
	case MML_CLOSE_PAREN_TOK:
	case MML_OPEN_BRAC_TOK:
	case MML_CLOSE_BRAC_TOK:
	case MML_OPEN_BRACKET_TOK:
	case MML_CLOSE_BRACKET_TOK:
	case MML_COMMA_TOK:
	case MML_PIPE_TOK:
	case MML_SEMICOLON_TOK:
	case MML_TILDE_TOK:
		ret = nToken(type, cached_s, 1);
		++cached_s;
		break;
	case MML_OP_LESS_TOK:
		if ((cached_s)[1] == '=')
		{
			ret = nToken(MML_OP_LESSEQ_TOK, cached_s, 2);
			cached_s += 2;
			break;
		}
		ret = nToken(MML_OP_LESS_TOK, cached_s, 1);
		++cached_s;
		break;
	case MML_OP_GREATER_TOK:
		if ((cached_s)[1] == '=')
		{
			ret = nToken(MML_OP_GREATEREQ_TOK, cached_s, 2);
			cached_s += 2;
			break;
		}
		ret = nToken(MML_OP_GREATER_TOK, cached_s, 1);
		++cached_s;
		break;
	case MML_OP_EQ_TOK:
		if (cached_s[1] != '=')
		{
			ret = nToken(MML_OP_ASSERT_EQUAL, cached_s, 1);
			++cached_s;
		} else if (cached_s[2] != '=')
		{
			ret = nToken(MML_OP_EQ_TOK, cached_s, 2);
			cached_s += 2;
		} else
		{
			ret = nToken(MML_OP_EXACT_EQ, cached_s, 3);
			cached_s += 3;
		}
		break;
	case MML_OP_NOT_TOK:
		if ((cached_s)[1] != '=')
		{
			ret = nToken(MML_OP_NOT_TOK, cached_s, 1);
			++cached_s;
		} else if ((cached_s)[2] != '=')
		{
			ret = nToken(MML_OP_NOTEQ_TOK, cached_s, 2);
			cached_s += 2;
		} else
		{
			ret = nToken(MML_OP_EXACT_NOTEQ, cached_s, 3);
			cached_s += 3;
		}
		break;
	case MML_DIGIT_TOK: {
		const char *const start = cached_s;
		bool has_dot = false;
		bool has_exp = false;
		bool has_underscore = false;

		while (true)
		{
			if (isdigit(*cached_s)) {
				++cached_s;
			} else if (*cached_s == '_' && isdigit(cached_s[1])) {
				cached_s += 2;
				has_underscore = true;
			} else if (!state->looking_for_int && *cached_s == '.' && !has_dot) {
				has_dot = true;
				++cached_s;
			} else if (!state->looking_for_int && !has_exp && (*cached_s == 'e' || *cached_s == 'E')) {
				has_exp = true;
				++cached_s;
				if (*cached_s == '+' || *cached_s == '-') ++cached_s;
				if (!isdigit(*cached_s)) break;
				while (isdigit(*cached_s)) ++cached_s;
			} else
				break;
		}

		size_t raw_len = cached_s - start;

		if (!has_underscore) {
			ret = nToken(MML_NUMBER_TOK, start, raw_len);
			break;
		}
		
		char *buf = arena_alloc_T(MML_global_arena, raw_len, char);
		char *dst = buf;

		for (const char *src = start; src < cached_s && (size_t)(dst - buf) < raw_len - 1; ++src)
			if (*src != '_') *dst++ = *src;

		ret = nToken(MML_NUMBER_TOK, buf, dst - buf);
		break;
	}
	case MML_LETTER_TOK:
	case MML_UNDERSCORE_TOK: {
		const char *const start = (char *)cached_s;
		do {
			++cached_s;
		} while (isalnum(*cached_s) || *cached_s == '_');
		ret = nToken(MML_IDENT_TOK, start, cached_s - start);
		break;
	}
	default:
		MML_log_warn("invalid token starts at '%.5s'\n", cached_s++);
		break;
	}


	state->current_tok = ret;
	*s = cached_s;
	return ret;
}

// Peeks at the next token without advancing the string pointer.
static MML_token peek_token(const char **s, struct parser_state *state)
{
	if (!state->has_peeked)
	{
		const char *s_copy = *s;
		state->peeked_tok = get_next_token(&s_copy, state);
		state->saved_s = s_copy;
		state->has_peeked = true;
	}
	return state->peeked_tok;
}

#define PARSER_MAX_PRECED 15

static bool op_is_unary(MML_token_type op)
{
	return (op == MML_TILDE_TOK || (op >= MML_OP_NOT_TOK && op <= MML_OP_UNARY_NOTHING));
}

static bool op_is_right_associative(MML_token_type op)
{
	return op == MML_OP_POW_TOK || op_is_unary(op);
}

static bool in_pipe_block = false;

static MML_expr *parse_expr(const char **s, uint32_t max_preced, struct parser_state *state)
{
	MML_token tok = get_next_token(s, state);

	MML_expr *left = arena_alloc_T(MML_global_arena, 1, MML_expr);
	left->type = Invalid_type;

	if (tok.type == MML_OP_SUB_TOK || tok.type == MML_OP_ADD_TOK
			|| op_is_unary(tok.type))
	{
		MML_token_type new_token_type = tok.type;
		if (new_token_type == MML_OP_ADD_TOK)
			new_token_type = MML_OP_UNARY_NOTHING;
		else if (new_token_type == MML_OP_SUB_TOK)
			new_token_type = MML_OP_NEGATE;
		
		MML_expr *operand = parse_expr(s, PRECEDENCE[new_token_type], state);

		left->type = Operation_type;
		left->o.op = new_token_type;
		left->o.left = operand;
		left->o.right = NULL;
	} else if (tok.type == MML_IDENT_TOK)
	{
		MML_token ident = tok;
		MML_token next_tok = peek_token(s, state);

		if (tok.type == MML_IDENT_TOK && next_tok.type == MML_OPEN_BRAC_TOK)
		{
			MML_expr *name = arena_alloc_T(MML_global_arena, 1, MML_expr);
			name->type = Identifier_type;
			name->s = strbuf_dup(ident.buf);

			left->type = Operation_type;
			left->o.left = name;
			left->o.op = MML_OP_FUNC_CALL_TOK;

			get_next_token(s, state);

			left->o.right = arena_alloc_T(MML_global_arena, 1, MML_expr);
			left->o.right->type = Vector_type;
			// temporary dvec because we don't know how many elements it'll have
			MML_expr_dvec temp = DVEC_INIT;
			do
			{
				if (**s == '\0' || peek_token(s, state).type == MML_CLOSE_BRAC_TOK)
					break;
				MML_expr *next_expr = parse_expr(s, PARSER_MAX_PRECED, state);
				dv_push(temp, next_expr);
				//if (next_expr != nullptr)
				//	--next_expr->num_refs;
			} while (get_next_token(s, state).type == MML_COMMA_TOK);
			left->o.right->v.ptr = arena_alloc_T(MML_global_arena, dv_n(temp), MML_expr *);
			left->o.right->v.n = dv_n(temp);
			// copy `temp` into the actual vector
			memcpy(
				left->o.right->v.ptr,
				_dv_ptr(temp),
				dv_n(temp) * sizeof(MML_expr *));
			dv_destroy(temp);

			if (state->current_tok.type != MML_CLOSE_BRAC_TOK)
			{
				get_next_token(s, state);
			/*	fprintf(stderr, "expected closing brace for function call, got %s\n",
						TOK_STRINGS[current_tok.type]);
				MML_free_expr(left);
				return nullptr;*/
			}
		} else
		{
			left->type = Identifier_type;
			left->s = strbuf_dup(ident.buf);
		}
	} else if (tok.type == MML_OPEN_PAREN_TOK)
	{
		left = parse_expr(s, PARSER_MAX_PRECED, state);
		MML_log_dbg("left type = %s\n", EXPR_TYPE_STRINGS[left->type]);
		MML_token close_paren_tok = get_next_token(s, state);
		if (close_paren_tok.type != MML_CLOSE_PAREN_TOK)
			get_next_token(s, state);
	} else if (tok.type == MML_OPEN_BRACKET_TOK)
	{
		MML_expr_dvec temp = DVEC_INIT;
		while (tok.type != MML_CLOSE_BRACKET_TOK)
		{
			tok = peek_token(s, state);
			if (tok.type == MML_CLOSE_BRACKET_TOK)
				break;

			MML_expr *e = parse_expr(s, PARSER_MAX_PRECED, state);
			dv_push(temp, e);

			tok = get_next_token(s, state);
			if (tok.type != MML_CLOSE_BRACKET_TOK
			 && tok.type != MML_COMMA_TOK)
			{
				MML_log_err("unexpected token %s found after element"
						" in vector literal (expected CLOSE_BRACKET_TOK or COMMA_TOK)\n",
					 TOK_STRINGS[tok.type]);
				dv_destroy(temp);

				return NULL;
			}
		}
		
		left->type = Vector_type;
		left->v.ptr = arena_alloc_T(MML_global_arena, dv_n(temp), MML_expr *);
		left->v.n = dv_n(temp);
		memcpy(
			left->v.ptr,
			_dv_ptr(temp),
			dv_n(temp) * sizeof(MML_expr *)
		);

		dv_destroy(temp);
	} else if (tok.type == MML_PIPE_TOK)
	{
		tok = peek_token(s, state);
		if (tok.type == MML_PIPE_TOK)
		{
			MML_log_err("expected expression in pipe block\n");
			return NULL;
		}

		in_pipe_block = true;

		left = parse_expr(s, PARSER_MAX_PRECED, state);
		MML_token close_pipe_tok = get_next_token(s, state);

		if (close_pipe_tok.type != MML_PIPE_TOK)
			get_next_token(s, state);

		in_pipe_block = false;
		//MML_expr *opnode = Pipe(left);

		MML_expr *opnode = arena_alloc_T(MML_global_arena, 1, MML_expr);
		opnode->type = Operation_type;
		opnode->o.op = MML_PIPE_TOK;
		opnode->o.left = left;
		opnode->o.right = NULL;

		left = opnode;
	} else if (tok.type == MML_NUMBER_TOK)
	{
		if (state->looking_for_int)
			*left = EXPR_NUM((double)strtoll(tok.buf.s, NULL, 10));
		else
			*left = EXPR_NUM(strtod(tok.buf.s, NULL)); 
		state->looking_for_int = false;
	} else {
		return NULL;
	}

	for (;;)
	{
		MML_token op_tok = peek_token(s, state);
		if (op_tok.type == MML_INVALID_TOK)
			break;

		bool do_advance = true;
		if (op_tok.type == MML_IDENT_TOK
		 || op_tok.type == MML_NUMBER_TOK
		 || op_tok.type == MML_OPEN_PAREN_TOK
		 || op_tok.type == MML_OPEN_BRACKET_TOK
		 || (op_tok.type == MML_PIPE_TOK && !in_pipe_block))
		{
			op_tok.type = MML_OP_MUL_TOK;
			do_advance = false;
		}
		if (op_tok.type > MML_NOT_OP_TOK)
			break;

		uint32_t preced = PRECEDENCE[op_tok.type];
		if (preced > max_preced)
			break;

		if (do_advance) get_next_token(s, state);

		if (op_tok.type == MML_OP_DOT_TOK)
			state->looking_for_int = true;

		MML_expr *right = parse_expr(s,
				op_is_right_associative(op_tok.type)
					? preced
					: preced-1, state);

		if (right == NULL) {
			MML_log_err("expected expression after operator %s\n",
					TOK_STRINGS[op_tok.type]);
			return NULL;
		}

		MML_expr *opnode = arena_alloc_T(MML_global_arena, 1, MML_expr);
		opnode->type = Operation_type;
		opnode->o.left = left;
		opnode->o.right = right;
		opnode->o.op = op_tok.type;

		left = opnode;
	}

	return left;
}

MML_expr *MML_parse(const char *s)
{
	struct parser_state state = {0};
	return parse_expr(&s, PARSER_MAX_PRECED, &state);
}
MML_expr_dvec MML_parse_stmts(const char *s)
{
	MML_expr_dvec temp = DVEC_INIT;
	struct parser_state state = {0};
	do
	{
		dv_push(temp, parse_expr(&s, PARSER_MAX_PRECED, &state));
	} while (get_next_token(&s, &state).type == MML_SEMICOLON_TOK);

	return temp;
}
