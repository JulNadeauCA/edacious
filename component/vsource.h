/*	$Csoft: vsource.h,v 1.1.1.1 2005/09/08 05:26:55 vedge Exp $	*/
/*	Public domain	*/

#ifndef _COMPONENT_VSOURCE_H_
#define _COMPONENT_VSOURCE_H_

#include <component/component.h>

#include "begin_code.h"

struct vsource {
	struct component com;

	double voltage;			/* Independent voltage */
	double resistance;		/* Internal DC resistance */
	
	TAILQ_HEAD(,cktloop) loops;	/* Forward loops */
	unsigned int        nloops;

	struct pin	**lstack;	/* Temporary loop stack */
	unsigned int	 nlstack;
};

#define VSOURCE(com) ((struct vsource *)(com))

__BEGIN_DECLS
void		 vsource_init(void *, const char *);
void		 vsource_reinit(void *);
void		 vsource_destroy(void *);
void		 vsource_find_loops(struct vsource *);
void		 vsource_free_loops(struct vsource *);
int		 vsource_load(void *, struct netbuf *);
int		 vsource_save(void *, struct netbuf *);
int		 vsource_export(void *, enum circuit_format, FILE *);
double		 vsource_emf(void *, int, int);
double		 vsource_resistance(void *, struct pin *, struct pin *);
struct window	*vsource_edit(void *);
void		 vsource_draw(void *, struct vg *);
int		 vsource_configure(void *);
void		 vsource_tick(void *);
__END_DECLS

#include "close_code.h"
#endif /* _COMPONENT_VSOURCE_H_ */
