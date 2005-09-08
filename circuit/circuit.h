/*	$Csoft: circuit.h,v 1.5 2004/08/26 06:51:12 vedge Exp $	*/
/*	Public domain	*/

#ifndef _CIRCUIT_CIRCUIT_H_
#define _CIRCUIT_CIRCUIT_H_

#include <engine/vg/vg.h>
#include <component/component.h>
#include <circuit/analysis.h>
#include <mat/mat.h>

#include "begin_code.h"

#define CIRCUIT_NAME_MAX	256
#define CIRCUIT_DESCR_MAX	256
#define CIRCUIT_MAX_BRANCHES	32
#define CIRCUIT_MAX_NODES	(0xffff-1)

/* Connection to one component. */
struct cktbranch {
	struct pin *pin;
	TAILQ_ENTRY(cktbranch) branches;
};

/* Electrical common between two or more components. */
struct cktnode {
	int	name;
	int	flags;
#define CKTNODE_EXAM	0x01	/* Branches are being examined */

	TAILQ_HEAD(,cktbranch) branches;	/* Electrical connections */
	int                   nbranches;

	TAILQ_ENTRY(cktnode) nodes;
};

/* Closed loop of dipoles with respect to some origin dipole in the circuit. */
struct cktloop {
	unsigned int name;
	void *origin;			/* Origin component (ie. vsource) */
	struct dipole **dipoles;	/* Dipoles in loop */
	unsigned int   ndipoles;
	TAILQ_ENTRY(cktloop) loops;
};

struct circuit {
	struct object obj;
	char descr[CIRCUIT_DESCR_MAX];	/* Short description */
	struct vg *vg;			/* Schematics drawing */
	struct analysis *analysis;	/* Current analysis mode */

	pthread_mutex_t lock;
	int dis_nodes;			/* Display node indications */
	int dis_node_names;		/* Display node names */

	TAILQ_HEAD(,cktnode) nodes;	/* Nodes (as in KCL) */
	unsigned int	    nnodes;

	struct cktloop	**loops;	/* Closed loops (as in KVL) */
	unsigned int	 nloops;
};

__BEGIN_DECLS
void		circuit_init(void *, const char *);
void		circuit_reinit(void *);
int		circuit_load(void *, struct netbuf *);
int		circuit_save(void *, struct netbuf *);
void		circuit_destroy(void *);
void		circuit_free_nodes(struct circuit *);
void		circuit_free_components(struct circuit *);
struct window  *circuit_edit(void *);

struct cktnode	 *circuit_add_node(struct circuit *, int, int);
struct cktnode	 *circuit_get_node(struct circuit *, int);
void		  circuit_del_node(struct circuit *, struct cktnode *);

struct cktbranch *circuit_add_branch(struct circuit *, struct cktnode *,
			             struct pin *);
void		  circuit_del_branch(struct circuit *, struct cktnode *,
		                     struct cktbranch *);
__inline__ struct cktbranch *circuit_get_branch(struct circuit *,
		                                struct cktnode *,
						struct pin *);

struct analysis	*circuit_set_analysis(struct circuit *,
		                      const struct analysis_ops *);
void		 circuit_unset_analysis(struct circuit *);
int		 circuit_start_analysis(struct circuit *);
int		 circuit_stop_analysis(struct circuit *);

void		 circuit_modified(struct circuit *);
void		 circuit_compose_Rmat(struct circuit *);
int		 circuit_solve_currents(struct circuit *);
__END_DECLS

#include "close_code.h"
#endif	/* _CIRCUIT_CIRCUIT_H_ */
