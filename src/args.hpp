#ifndef MZD_ARGS_HPP
#define MZD_ARGS_HPP

#ifdef __cplusplus
extern "C" {
#endif

#include "context.h"

void mzd_args_populate_context(struct MzdContext *context, const int argc, const char **argv);

#ifdef __cplusplus
}
#endif

#endif
