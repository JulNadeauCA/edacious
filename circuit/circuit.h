/*	$Csoft: circuit.h,v 1.5 2005/09/15 02:04:48 vedge Exp $	*/
/*	Public domain	*/

#ifndef _CIRCUIT_CIRCUIT_H_
#define _CIRCUIT_CIRCUIT_H_

#include <agar/core.h>
#include <agar/vg.h>
#include <agar/sc.h>

#include <component/component.h>
#include <circuit/sim.h>

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

/* Connection between two or more components. */
struct cktnode {
	u_int flags;
#define CKTNODE_EXAM		0x01	/* Branches are being examined
					   (used to avoid redundancies) */
#define CKTNODE_REFERENCE	0x02	/* Reference node (eg. ground) */

	TAILQ_HEAD(,cktbranch) branches;
	u_int		      nbranches;
};

/* Closed loop of dipoles with respect to some component in the circuit. */
struct cktloop {
	u_int name;
	void *origin;			/* Origin component (ie. vsource) */
	struct dipole **dipoles;	/* Dipoles in loop */
	u_int	       ndipoles;
	TAILQ_ENTRY(cktloop) loops;
};

struct vsource;

struct circuit {
	AG_Object obj;
	char descr[CIRCUIT_DESCR_MAX];	/* Short description */
	VG *vg;				/* Schematics drawing */
	struct sim *sim;		/* Current simulation mode */

	pthread_mutex_t lock;
	int dis_nodes;			/* Display node indications */
	int dis_node_names;		/* Display node names */

	struct cktnode **nodes;		/* Nodes (element 0 is ground) */
	struct cktloop **loops;		/* Closed loops */
	struct vsource **vsrcs;		/* Independent vsources */

	u_int l;			/* Number of loops */
	u_int m;			/* Number of independent vsources */
	u_int n;			/* Number of nodes (except ground) */
};

__BEGIN_DECLS
void		circuit_init(void *, const char *);
void		circuit_reinit(void *);
int		circuit_load(void *, AG_Netbuf *);
int		circuit_save(void *, AG_Netbuf *);
void		circuit_destroy(void *);
void		circuit_free_components(struct circuit *);
void	       *circuit_edit(void *);

int	circuit_add_node(struct circuit *, u_int);
void	circuit_del_node(struct circuit *, u_int);
void	circuit_copy_node(struct circuit *, struct cktnode *, struct cktnode *);
void	circuit_destroy_node(struct cktnode *);

struct cktbranch *circuit_add_branch(struct circuit *, u_int, struct pin *);
void		  circuit_del_branch(struct circuit *, u_int,
		                     struct cktbranch *);

__inline__ struct cktbranch *circuit_get_branch(struct circuit *, u_int,
						struct pin *);

__inline__ int	 cktnode_grounded(struct circuit *, u_int);
__inline__ int	 cktnode_vsource(struct circuit *, u_int, u_int, int *);

struct sim	*circuit_set_sim(struct circuit *, const struct sim_ops *);
void		 circuit_modified(struct circuit *);
__END_DECLS

#include "close_code.h"
#endif	/* _CIRCUIT_CIRCUIT_H_ */
