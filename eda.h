/*	$Csoft$	*/
/*	Public domain	*/

#ifndef _AGAR_EDA_H_
#define _AGAR_EDA_H_

#include <config/edition.h>

#include <agar/compat/queue.h>
#include <agar/compat/strlcpy.h>
#include <agar/compat/strlcat.h>
#include <agar/compat/snprintf.h>
#include <agar/compat/vsnprintf.h>
#include <agar/compat/asprintf.h>
#include <agar/compat/vasprintf.h>
#include <agar/compat/strsep.h>
#include <agar/compat/math.h>

#define Malloc(s,c) AG_Malloc((s),(c))
#define Free(p,c) AG_Free(p,c)
#define Realloc(p,s) AG_Realloc((p),(s))
#define Fatal(msg) do { fprintf(stderr, "%s\n", msg); abort(); } while (0)

#define _(s) (s)
#define N_(s) (s)

#include <math.h>
#include <string.h>

struct eda_type {
	const char *name;
	size_t size;
	const void *ops;
};

#include <circuit/circuit.h>
#include <component/conductor.h>
#include <component/ground.h>
#include <sources/vsource.h>

#endif /* _AGAR_EDA_H_ */
