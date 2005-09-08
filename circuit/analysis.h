/*	$Csoft: analysis.h,v 1.2 2004/09/29 03:44:53 vedge Exp $	*/
/*	Public domain	*/

#ifndef _ANALYSIS_ANALYSIS_H_
#define _ANALYSIS_ANALYSIS_H_
#include "begin_code.h"

struct analysis {
	struct circuit *ckt;
	const struct analysis_ops *ops;
	int running;
};

struct analysis_ops {
	char *name;
	size_t size;
	void (*init)(void *);
	void (*destroy)(void *);
	void (*start)(void *);
	void (*stop)(void *);
	void (*cktmod)(void *);
	struct window *(*settings)(void *);
};

#define ANALYSIS(p) ((struct analysis *)p)

__BEGIN_DECLS
void	analysis_init(void *, const struct analysis_ops *);
void	analysis_destroy(void *);
void	analysis_configure(int, union evarg *);
__END_DECLS

#include "close_code.h"
#endif	/* _ANALYSIS_ANALYSIS_H_ */
