#include "context.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void mzd_context_free(struct MzdContext *context) {
    if (context->command)
        free((void *) context->command);
    if (context->command_ptr)
        free(context->command_ptr);
}
