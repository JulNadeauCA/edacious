/*	Public domain	*/

#include "core.h"

__BEGIN_DECLS
const ES_IntegrationMethod esIntegrationMethods[] = {
	/* N	Name	Descr			Impl	Order	ErrConst */
	{ BE,	"BE",	N_("Backward Euler"),	1,	1,	0.5 },
	{ FE,	"FE",	N_("Forward Euler"),	0,	1,	0.5 },
	{ TR,	"TR",	N_("Trapezoidal"),	1,	2,	1.0/12.0 },
	{ G2,	"G2",	N_("Gear-2"),		1,	2,	2.0/9.0 }
};
const int esIntegrationMethodCount = sizeof(esIntegrationMethods) /
                                     sizeof(esIntegrationMethods[0]);
__END_DECLS
