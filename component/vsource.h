/*	$Csoft: vsource.h,v 1.3 2005/09/15 02:04:49 vedge Exp $	*/
/*	Public domain	*/

#ifndef _COMPONENT_VSOURCE_H_
#define _COMPONENT_VSOURCE_H_

#include <component/component.h>

#include "begin_code.h"

struct vsource {
	struct component com;
	double voltage;
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
int		 vsource_load(void *, AG_Netbuf *);
int		 vsource_save(void *, AG_Netbuf *);
int		 vsource_export(void *, enum circuit_format, FILE *);
double		 vsource_emf(void *, int, int);
AG_Window	*vsource_edit(void *);
void		 vsource_draw(void *, VG *);
void		 vsource_tick(void *);
__END_DECLS

#include "close_code.h"
#endif /* _COMPONENT_VSOURCE_H_ */
