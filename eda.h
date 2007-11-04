/*	Public domain	*/

#ifndef _AGAR_EDA_H_
#define _AGAR_EDA_H_

#include <config/edition.h>

#include <agar/core/queue.h>
#include <agar/core/strlcpy.h>
#include <agar/core/strlcat.h>
#include <agar/core/snprintf.h>
#include <agar/core/vsnprintf.h>
#include <agar/core/asprintf.h>
#include <agar/core/vasprintf.h>
#include <agar/core/strsep.h>
#include <agar/core/math.h>

#define Malloc(s,c) AG_Malloc((s),(c))
#define Free(p,c) AG_Free(p,c)
#define Realloc(p,s) AG_Realloc((p),(s))
#define Fatal AG_FatalError

#define _(s) (s)
#define N_(s) (s)

#include <math.h>
#include <string.h>

#include <circuit/circuit.h>
#include <component/ground.h>
#include <sources/vsource.h>

#endif /* _AGAR_EDA_H_ */
