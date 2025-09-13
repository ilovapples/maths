#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>

#include "parser.h"
#include "eval.h"
#include "token.h"

static Token current_tok = nToken(INVALID_TOK, NULL, 0);

// Gets the next token and advances the string pointer.
Token get_next_token(const char **s, struct parser_state *state)
{
	Token ret = nToken(INVALID_TOK, NULL, 0);

	if (state->has_peeked) {
		state->has_peeked = false;
		*s = state->saved_s;
		return state->peeked_tok;
	}

	while (isspace(**s)) ++*s;

	if (**s == '\0') return nToken(EOF_TOK, NULL, 0);

	TokenType type = TOK_BY_CHAR[**s - 0x21];
	switch (type) {
	case OP_DOT_TOK:
	case OP_AT_TOK:
	case OP_POW_TOK:
	case OP_MUL_TOK:
	case OP_DIV_TOK:
	case OP_MOD_TOK:
	case OP_ADD_TOK:
	case OP_SUB_TOK:
	case OPEN_PAREN_TOK:
	case CLOSE_PAREN_TOK:
	case OPEN_BRAC_TOK:
	case CLOSE_BRAC_TOK:
	case OPEN_BRACKET_TOK:
	case CLOSE_BRACKET_TOK:
	case COMMA_TOK:
	case PIPE_TOK:
	case SEMICOLON_TOK:
	case TILDE_TOK:
		ret = nToken(type, *s, 1);
		++*s;
		break;
	case OP_LESS_TOK:
		if ((*s)[1] == '=')
		{
			ret = nToken(OP_LESSEQ_TOK, *s, 2);
			*s += 2;
		}
		ret = nToken(OP_LESS_TOK, *s, 1);
		++*s;
		break;
	case OP_GREATER_TOK:
		if ((*s)[1] == '=')
		{
			ret = nToken(OP_GREATEREQ_TOK, *s, 2);
			*s += 2;
		}
		ret = nToken(OP_GREATER_TOK, *s, 1);
		++*s;
		break;
	case OP_EQ_TOK:
		if ((*s)[1] != '=')
		{
			ret = nToken(OP_ASSERT_EQUAL, *s, 1);
			++*s;
			break;
		}
		ret = nToken(OP_EQ_TOK, *s, 2);
		*s += 2;
		break;
	case OP_NOT_TOK:
		if ((*s)[1] == '=')
		{
			ret = nToken(OP_NOTEQ_TOK, *s, 2);
			*s += 2;
			break;
		}
		ret = nToken(OP_NOT_TOK, *s, 1);
		++*s;
		break;
	case DIGIT_TOK:
		ret.buf.s = (char *)*s;
		char *end;
		if (state->looking_for_int)
			strtoll(ret.buf.s, &end, 10);
		else
			strtod(ret.buf.s, &end);
		ret.buf.len = end - ret.buf.s;
		*s = end;
		ret.type = NUMBER_TOK;
		break;
	case DOLLAR_TOK:
		++*s;
		[[fallthrough]];
	case LETTER_TOK:
	case UNDERSCORE_TOK:
		ret.buf.s = (char *)*s;
		do {
			++*s;
		} while (isalnum(**s) || **s == '_');
		ret.buf.len = *s - ret.buf.s;
		ret.type = (type == DOLLAR_TOK) ? INSERTED_IDENT_TOK : IDENT_TOK;
		ret.buf.allocd = false;
		break;
	default:
		fprintf(stderr, "invalid token starts at '%.5s'\n", *s);
		break;
	}


	current_tok = ret;
	return ret;
}

// Peeks at the next token without advancing the string pointer.
Token peek_token(const char **s, struct parser_state *state)
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

bool op_is_unary(TokenType op)
{
	return (op == TILDE_TOK || (op >= OP_NOT_TOK && op <= OP_UNARY_NOTHING));
}

bool op_is_right_associative(TokenType op)
{
	return op == OP_POW_TOK || op_is_unary(op);
}

Expr *parse_expr(const char **s, uint32_t max_preced, struct parser_state *state)
{
	Token tok = get_next_token(s, state);

	Expr *left = calloc(1, sizeof(Expr));
	left->num_refs = 1;

	if (tok.type == OP_SUB_TOK || tok.type == OP_ADD_TOK
			|| op_is_unary(tok.type))
	{
		TokenType new_token_type = tok.type;
		if (new_token_type == OP_ADD_TOK)
			new_token_type = OP_UNARY_NOTHING;
		else if (new_token_type == OP_SUB_TOK)
			new_token_type = OP_NEGATE;
		
		Expr *operand = parse_expr(s, PRECEDENCE[new_token_type], state);

		left->type = Operation_type;
		left->u.o.left = operand;
		left->u.o.right = NULL;
		left->u.o.op = new_token_type;
	} else if (tok.type == IDENT_TOK || tok.type == INSERTED_IDENT_TOK)
	{
		Token ident = tok;
		Token next_tok = peek_token(s, state);

		if (tok.type == IDENT_TOK && next_tok.type == OPEN_BRAC_TOK)
		{
			Expr *name = calloc(1, sizeof(Expr));
			name->type = Identifier_type;
			name->num_refs = 1;
			name->u.v.s = ident.buf;

			left->type = Operation_type;
			left->u.o.left = name;
			left->u.o.op = OP_FUNC_CALL_TOK;

			get_next_token(s, state);

			left->u.o.right = calloc(1, sizeof(Expr));
			left->u.o.right->type = Vector_type;
			left->u.o.right->num_refs = 1;
			left->u.o.right->should_free_vec_block = false;
			left->u.o.right->u.v.v = new_vec(1);
			do
			{
				if (peek_token(s, state).type == CLOSE_BRAC_TOK)
					break;
				Expr *next_expr = parse_expr(s, PARSER_MAX_PRECED, state);
				push_to_vec(&left->u.o.right->u.v.v,
						next_expr);
				--next_expr->num_refs;
			} while (get_next_token(s, state).type == COMMA_TOK);

			if (current_tok.type != CLOSE_BRAC_TOK)
			{
				fprintf(stderr, "expected closing brace for function call, got %s\n",
						TOK_STRINGS[next_tok.type]);
				free_expr(&left);
				return nullptr;
			}
		} else
		{
			left->type = (tok.type == IDENT_TOK)
				? Identifier_type
				: InsertedIdentifier_type;
			left->u.v.s = ident.buf;
		}
	} else if (tok.type == OPEN_PAREN_TOK)
	{
		free(left); // have to free the memory block held in `left` before overwriting the pointer
		left = parse_expr(s, PARSER_MAX_PRECED, state);
		Token close_paren_tok = get_next_token(s, state);
		if (close_paren_tok.type != CLOSE_PAREN_TOK)
		{
			fprintf(stderr, "expected close paren for parenthesis block, got %s\n", TOK_STRINGS[close_paren_tok.type]);
			free_expr(&left);
			return nullptr;
		}
	} else if (tok.type == OPEN_BRACKET_TOK)
	{
		VecN vec = new_vec(1);
		while (tok.type != CLOSE_BRACKET_TOK)
		{
			tok = peek_token(s, state);
			if (tok.type == CLOSE_BRACKET_TOK)
				break;

			Expr *e = parse_expr(s, PARSER_MAX_PRECED, state);
			push_to_vec(&vec, e);
			--e->num_refs;

			tok = get_next_token(s, state);
			if (tok.type != CLOSE_BRACKET_TOK
			 && tok.type != COMMA_TOK)
			{
				fprintf(stderr, "unexpected token %s found after element"
						" in vector literal (expected CLOSE_BRACKET_TOK or COMMA_TOK)\n",
					 TOK_STRINGS[tok.type]);
				free_expr(&left);
				free_vec(&vec);
				return nullptr;
			}
		}
		*left = (Expr) { Vector_type, 1, .u.v.v = vec };
	} else if (tok.type == PIPE_TOK)
	{
		free(left);
		left = nullptr;
		tok = peek_token(s, state);
		if (tok.type == PIPE_TOK)
		{
			fprintf(stderr, "expected expression in pipe block\n");
			return nullptr;
		}
		left = parse_expr(s, PARSER_MAX_PRECED, state);
		Token close_pipe_tok = get_next_token(s, state);
		if (close_pipe_tok.type != PIPE_TOK)
		{
			fprintf(stderr, "expected closing pipe for pipe block, got %s\n",
					TOK_STRINGS[close_pipe_tok.type]);
			free_expr(&left);
			return nullptr;
		}
		Expr *opnode = calloc(1, sizeof(Expr));
		opnode->type = Operation_type;
		opnode->num_refs = 1;
		opnode->u.o.left = left;
		opnode->u.o.right = nullptr;
		opnode->u.o.op = PIPE_TOK;

		left = opnode;
	} else if (tok.type == NUMBER_TOK)
	{
		if (state->looking_for_int)
			*left = EXPR_NUM((double)strtoll(tok.buf.s, NULL, 10));
		else
			*left = EXPR_NUM(strtod(tok.buf.s, NULL)); 
		left->num_refs = 1;
		state->looking_for_int = false;
	} else
	{
		//fprintf(stderr, "found no valid operations/literals, returning (null)\n");
		free_expr(&left);
		return nullptr;
	}

	for (;;)
	{
		Token op_tok = peek_token(s, state);
		if (op_tok.type == INVALID_TOK)
		{
			fprintf(stderr, "invalid token found\n");
			free_expr(&left);
			return nullptr;
		}
		bool do_advance = true;
		if (op_tok.type == INSERTED_IDENT_TOK
		 || op_tok.type == IDENT_TOK || op_tok.type == NUMBER_TOK)
		{
			op_tok.type = OP_MUL_TOK;
			do_advance = false;
		}
		if (op_tok.type > NOT_OP_TOK)
			break;

		uint32_t preced = PRECEDENCE[op_tok.type];
		if (preced > max_preced)
			break;

		if (do_advance) get_next_token(s, state);

		if (op_tok.type == OP_DOT_TOK)
			state->looking_for_int = true;

		Expr *right = parse_expr(s,
				op_is_right_associative(op_tok.type)
					? preced
					: preced-1, state);

		if (right == nullptr)
		{
			fprintf(stderr, "expected expression after operator %s\n",
					TOK_STRINGS[op_tok.type]);
			free_expr(&left);
			return nullptr;
		}

		Expr *opnode = calloc(1, sizeof(Expr));
		opnode->type = Operation_type;
		opnode->num_refs = 1;
		opnode->u.o.left = left;
		opnode->u.o.right = right;
		opnode->u.o.op = op_tok.type;

		left = opnode;
	}

	return left;
}

inline Expr *parse(const char *s)
{
	struct parser_state state = {0};
	return parse_expr(&s, PARSER_MAX_PRECED, &state);
}
VecN parse_stmts(const char *s)
{
	VecN ret = new_vec(1);
	struct parser_state state = {0};
	do
	{
		push_to_vec(&ret, parse_expr(&s, PARSER_MAX_PRECED, &state));
	} while (get_next_token(&s, &state).type == SEMICOLON_TOK);

	return ret;
}

void parse_stmts_to_evaluator(const char *s, struct evaluator_state *eval_state)
{
	struct parser_state state = {0};
	do
	{
		Expr *cur_expr = parse_expr(&s, PARSER_MAX_PRECED, &state);
		if (cur_expr != nullptr)
			eval_push_expr(eval_state, cur_expr);
	} while (get_next_token(&s, &state).type == SEMICOLON_TOK);
}
