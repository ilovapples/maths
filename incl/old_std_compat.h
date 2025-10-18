#ifndef OLD_STD_COMPAT_H
#define OLD_STD_COMPAT_H

#if __STDC_VERSION__ < 202311L
#include <stdbool.h>
#define nullptr NULL
#define constexpr const
#endif

#endif /* OLD_STD_COMPAT_H */
