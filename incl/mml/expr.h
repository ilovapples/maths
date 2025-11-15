#ifndef EXPR_H
#define EXPR_H

#include <complex.h>
#include <stdint.h>

#include "old_std_compat.h"
#include "cpp_compat.h"

#include "mml/token.h"
#include "dvec/dvec.h"

MML__CPP_COMPAT_BEGIN_DECLS

typedef struct MML_expr MML_expr;

typedef struct MML_Operation {
	MML_expr *left;
	MML_expr *right;
	MML_token_type op;
} MML_Operation;

typedef enum MML_ExprType {
	OutputCode_type,
	Invalid_type,
	Nothing_type,
	Operation_type,
	Integer_type,
	RealNumber_type,
	ComplexNumber_type,
	Boolean_type,
	Identifier_type,
	Vector_type,
	FuncObject_type,
} MML_expr_type;

typedef struct {
	MML_expr **ptr;
	size_t n;
} MML_expr_vec;

typedef struct {
	strbuf *ptr;
	size_t len;
} strslice;

typedef struct {
	strslice params;
	MML_expr *body;
} MML_func_object;

typedef dvec_t(MML_expr *) MML_expr_dvec;

#define VALTYPE_IS_ORDERED(v) \
	((v).type != ComplexNumber_type && \
	 (v).type != Vector_type && \
	 (v).type != Invalid_type)

struct value_union_size {
	uint64_t b[3];
};

typedef struct MML_value {
	MML_expr_type type;
	union {
		int64_t i;
		double n;
		_Complex double cn;
		bool b;
		strbuf s;
		MML_expr_vec v;
		MML_func_object fo;
		struct value_union_size w;
	};
} MML_value;

typedef struct MML_expr {
	MML_expr_type type;
	union {
		MML_Operation o;

		int64_t i;
		double n;
		_Complex double cn;
		bool b;
		strbuf s;
		MML_expr_vec v;
		MML_func_object fo;
		struct value_union_size w; // used for copying the union between MML_expr's
	};
} MML_expr;

#define EXPR_NUM(num) ((MML_expr) { RealNumber_type, .n = (num) })
#define VAL_NUM(num) ((MML_value) { RealNumber_type, .n = (num) })
#define VAL_CNUM(num) ((MML_value) { ComplexNumber_type, .cn = (num) })
#define VAL_BOOL(bl) ((MML_value) { Boolean_type, .b = (bl) })

enum CODE_TYPES {
	MML_ERROR_INVAL,
	MML_QUIT_INVAL,
	MML_CLEAR_INVAL,
};

constexpr MML_value VAL_INVAL = { Invalid_type, .w={} };
constexpr MML_value NOTHING_VAL = { Nothing_type, .w={} };
constexpr MML_expr NOTHING_EXPR = { Nothing_type, .w={} };

#define VAL_IS_NUM(v) (\
    (v).type == RealNumber_type \
 || (v).type == ComplexNumber_type \
 || (v).type == Boolean_type)

typedef struct MML_state MML_state;
void MML_print_indent(uint32_t indent);
MML_value MML_print_typedval(MML_state *restrict state, const MML_value *val);
MML_value MML_println_typedval(MML_state *restrict state, const MML_value *val);
MML_value MML_print_typedval_multiargs(MML_state *restrict state, MML_expr_vec *args);
MML_value MML_println_typedval_multiargs(MML_state *restrict state, MML_expr_vec *args);
void MML_print_exprh(const MML_expr *expr);
MML_value MML_print_exprh_tv_func(MML_state *restrict , MML_expr_vec *args);

void MML_free_pp(void *p);


double MML_get_number(const MML_value *v);
_Complex double MML_get_complex(const MML_value *v);

MML__CPP_COMPAT_END_DECLS

#endif /* EXPR_H */
