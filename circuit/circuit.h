/*	$Csoft: circuit.h,v 1.2 2005/09/08 09:46:01 vedge Exp $	*/
/*	Public domain	*/

#ifndef _CIRCUIT_CIRCUIT_H_
#define _CIRCUIT_CIRCUIT_H_

#include <engine/vg/vg.h>
#include <component/component.h>
#include <circuit/sim.h>
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

/* Connection between two or more components. */
struct cktnode {
	int	flags;
#define CKTNODE_EXAM	0x01		/* Branches are being examined */

	TAILQ_HEAD(,cktbranch) branches;
	u_int		      nbranches;
};

/* Closed loop of dipoles with respect to some component in the circuit. */
struct cktloop {
	unsigned int name;
	void *origin;			/* Origin component (ie. vsource) */
	struct dipole **dipoles;	/* Dipoles in loop */
	unsigned int   ndipoles;
	TAILQ_ENTRY(cktloop) loops;
};

struct vsource;

struct circuit {
	struct object obj;
	char descr[CIRCUIT_DESCR_MAX];	/* Short description */
	struct vg *vg;			/* Schematics drawing */
	struct sim *sim;		/* Current simulation mode */

	pthread_mutex_t lock;
	int dis_nodes;			/* Display node indications */
	int dis_node_names;		/* Display node names */

	struct cktnode **nodes;		/* Nodes */
	struct cktloop **loops;		/* Closed loops */
	struct vsource **vsrcs;		/* Independent vsources */

	u_int l;			/* Number of loops */
	u_int m;			/* Number of independent vsources */
	u_int n;			/* Number of nodes */
};

__BEGIN_DECLS
void		circuit_init(void *, const char *);
void		circuit_reinit(void *);
int		circuit_load(void *, struct netbuf *);
int		circuit_save(void *, struct netbuf *);
void		circuit_destroy(void *);
void		circuit_free_components(struct circuit *);
struct window  *circuit_edit(void *);

int	circuit_add_node(struct circuit *, u_int);
void	circuit_del_node(struct circuit *, int);
void	circuit_copy_node(struct circuit *, struct cktnode *, struct cktnode *);
void	circuit_destroy_node(struct cktnode *);

struct cktbranch *circuit_add_branch(struct circuit *, int, struct pin *);
void		  circuit_del_branch(struct circuit *, int, struct cktbranch *);

__inline__ struct cktbranch *circuit_get_branch(struct circuit *, int,
						struct pin *);

__inline__ int circuit_gnd_node(struct circuit *, int);

struct sim	*circuit_set_sim(struct circuit *, const struct sim_ops *);
void		 circuit_modified(struct circuit *);
__END_DECLS

#include "close_code.h"
#endif	/* _CIRCUIT_CIRCUIT_H_ */
