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

#define Malloc AG_Malloc
#define Free AG_Free
#define Realloc AG_Realloc
#define Fatal AG_FatalError

#define _(s) (s)
#define N_(s) (s)

#include <math.h>
#include <string.h>

#include <circuit/circuit.h>
#include <component/ground.h>
#include <sources/vsource.h>

#endif /* _AGAR_EDA_H_ */
