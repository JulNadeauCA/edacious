/*	$Csoft: component.h,v 1.1.1.1 2005/09/08 05:26:55 vedge Exp $	*/
/*	Public domain	*/

#ifndef _COMPONENT_COMPONENT_H_
#define _COMPONENT_COMPONENT_H_

#include <circuit/circuit.h>
#ifdef EDITION
#include <engine/widget/menu.h>
#endif

#include "begin_code.h"

#define COMPONENT_MAX_PINS	128
#define COMPONENT_PIN_NAME_MAX	16

/* Exportable circuit model formats. */
enum circuit_format {
	CIRCUIT_SPICE3		/* Spice3 deck (tested with Spice-3f5) */
};

/* Primary interface element between the model and the circuit. */
struct pin {
	int	n;				/* Pin number */
	char	name[COMPONENT_PIN_NAME_MAX];	/* Pin name */
	double	x, y;				/* Position in drawing */

	struct component *com;		/* Back pointer to component */
	int		  node;		/* Node connection (or -1) */
	struct cktbranch *branch;	/* Branch into node */

	int	selected;		/* Pin selected for edition */
	double	u;			/* Potential at this pin (fictitious) */
	double	du;			/* Change in potential as t->0 */
};

/* Ordered pair of pins belonging to the same component. */
struct dipole {
	struct component *com;		/* Back pointer to component */
	struct pin *p1;			/* Positive pin (current enters) */
	struct pin *p2;			/* Negative pin (current exits) */

	struct cktloop **loops;		/* Loops this dipole is part of */
	int	        *lpols;		/* Polarities with respect to loops */
	unsigned int    nloops;
};

/* Generic component description and operation vector. */
struct component_ops {
	struct object_ops ops;			/* Generic object ops */
	const char *name;			/* Name (eg. "Resistor") */
	const char *pfx;			/* Prefix (eg. "R") */

	void		 (*draw)(void *, struct vg *);
	struct window	*(*edit)(void *);
	int		 (*configure)(void *);
	int		 (*export_model)(void *, enum circuit_format, FILE *);
	void		 (*tick)(void *);
	double		 (*resistance)(void *, struct pin *, struct pin *);
	double		 (*capacitance)(void *, struct pin *, struct pin *);
	double		 (*inductance)(void *, struct pin *, struct pin *);
	double		 (*isource)(void *, struct pin *, struct pin *);
};

struct component {
	struct object obj;
	const struct component_ops *ops;
	struct circuit *ckt;			/* Back pointer to circuit */
	struct vg_block *block;			/* Schematic block */

	u_int flags;
#define COMPONENT_FLOATING	0x01		/* Not yet connected */

	int selected;				/* Selected for edition? */
	int highlighted;			/* Selected for selection? */

	pthread_mutex_t lock;
	struct timeout redraw_to;		/* Timer for vg updates */
	struct pin pin[COMPONENT_MAX_PINS];	/* Connection points */
	int npins;
	struct dipole *dipoles;			/* Ordered pin pairs */
	int ndipoles;

	float Tnom;		/* Model reference temperature (k) */
	float Tspec;		/* Component instance temperature (k) */

	TAILQ_ENTRY(component) components;
};

#define COM(p) ((struct component *)(p))
#define PIN(p,n) (&COM(p)->pin[n])
#define PNODE(p,n) (COM(p)->pin[n].node)
#define U(p,n) (PIN((p),(n))->u)

struct mapview;

__BEGIN_DECLS
void		component_init(void *, const char *, const char *, const void *,
			       const struct pin *);
void		component_destroy(void *);
int		component_load(void *, struct netbuf *);
int		component_save(void *, struct netbuf *);
struct window  *component_edit(void *);
void		component_insert(int, union evarg *);

struct pin     *component_pin_overlap(struct circuit *, struct component *,
		                   double, double);
void		component_connect(struct circuit *, struct component *,
		                  struct vg_vertex *);
int		component_highlight_pins(struct circuit *, struct component *);
void		component_highlight_pin(struct circuit *, struct pin *, int);

double		dipole_resistance(struct dipole *);
double		dipole_capacitance(struct dipole *);
double		dipole_inductance(struct dipole *);
__inline__ int	dipole_in_loop(struct dipole *, struct cktloop *,
		               int *);

#ifdef EDITION
void		component_reg_menu(struct AGMenu *, struct AGMenuItem *,
		                   struct circuit *, struct mapview *);
#endif
__END_DECLS

#include "close_code.h"
#endif /* _COMPONENT_COMPONENT_H_ */
