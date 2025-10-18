#ifndef MML__CPP_COMPAT_H
#define MML__CPP_COMPAT_H

#ifdef __cplusplus
#define MML__CPP_COMPAT_BEGIN_DECLS extern "C" {
#define MML__CPP_COMPAT_END_DECLS }

#define restrict
#else
#define MML__CPP_COMPAT_BEGIN_DECLS
#define MML__CPP_COMPAT_END_DECLS

#define restrict restrict
#endif

#endif /* MML__CPP_COMPAT_H */
