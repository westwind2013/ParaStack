#ifndef H_UNWIND_H

#define H_UNWIND_H

#include "h_common.h"

#include <errno.h>

#include "libunwind-x86_64.h"
#include "libunwind-ptrace.h"


// 0. function:
// description:	initialize
// input: 		target
// output:
int h_initialize(H_TARGET target);

// 0. function:
// description:	finish
// input:
// output:
int h_finish();

// 0. function:
// description:	backtrace
// input: 		target
// output:		tag
int h_backtrace(H_TARGET target, short *tag);

#endif /* H_UNWIND_H */
