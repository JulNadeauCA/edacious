#include "core.h"

__BEGIN_DECLS
const char *methodNames[] = {
	N_("Backward Euler"),
	N_("Forward Euler"),
	N_("Trapezoidal"),
	N_("Gear-2"),
	NULL
};
const int isImplicit[] = {
	1,
	0,
	1,
	1
};
const int methodOrder[] = {
	1,
	1,
	2,
	2
};
const M_Real methodErrorConstant[] = {
	0.5,
	0.5,
	1.0/12.0,
	2.0/9.0
};
__END_DECLS
