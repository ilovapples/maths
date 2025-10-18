#ifndef EVAL_H
#define EVAL_H

#include "old_std_compat.h"
#include "cpp_compat.h"

#include "mml/config.h"
#include "mml/expr.h"
#include "arena/arena.h"

MML__CPP_COMPAT_BEGIN_DECLS

extern Arena *MML_global_arena;

typedef struct hashmap hashmap;

typedef struct MML_state {
	struct MML_config *config;

	hashmap *variables;

	MML_value last_val;
	bool is_init;
} MML_state;

typedef MML_value (*MML_val_func)(MML_state *restrict state, MML_expr_vec *args);

#ifndef MML_BARE_USE
MML_value MML_apply_binary_op(MML_state *restrict state,
		MML_value a, MML_value b, MML_token_type op);
#endif


/* Returns a pointer to a valid, initialized evaluator state, which should be
 * passed to any function that takes `MML_state *` as an argument. The library
 * was developed with the ability to create multiple states in mind, but it
 * has not yet been formally tested.
 * `MML_cleanup_state` must be called on this function's return value when you are
 * done with it. */
MML_state *MML_init_state(void);
/* Cleans up allocations and such stored in an evaluator state at STATE.
 * STATE must have been obtained by a call to `MML_init_state`. See `MML_init_state` for
 * more details. */
void MML_cleanup_state(MML_state *restrict state);

int32_t MML_eval_set_variable(MML_state *restrict state, strbuf name, MML_expr *expr);
MML_expr *MML_eval_get_variable(MML_state *restrict state, strbuf name);

/* evaluates EXPR using the evaluator state data in STATE */
MML_value MML_eval_expr(MML_state *restrict state, const MML_expr *expr);
MML_value MML_eval_expr_recurse(MML_state *restrict state, const MML_expr *expr);

MML_value MML_eval_parse(MML_state *state, const char *s);

MML__CPP_COMPAT_END_DECLS

#endif /* EVAL_H */
