#ifndef PROMPT_H
#define PROMPT_H

#include "eval.h"
#include "cpp_compat.h"

MML__CPP_COMPAT_BEGIN_DECLS

void MML_run_prompt(MML_state *state);

void term_restore(void);

MML__CPP_COMPAT_END_DECLS

#endif /* PROMPT_H */
