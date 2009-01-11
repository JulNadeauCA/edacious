/*	Public domain	*/

struct es_circuit;

/* Device package description */
typedef struct es_package {
	struct es_layout _inherit;
	Uint flags;
#define ES_PACKAGE_CONNECTED	0x01		/* Connected to layout */
	TAILQ_HEAD(,vg_node) layoutEnts;	/* Entities in Layout VG */
	TAILQ_ENTRY(es_package) packages;	/* In layout */
} ES_Package;

#ifdef _ES_INTERNAL
#define PACKAGE(p) ((ES_Package *)(p))
#endif

__BEGIN_DECLS
extern AG_ObjectClass esPackageClass;
ES_Package *ES_PackageNew(struct es_circuit *);
int         ES_PackageTool_InsertDevicePkg(VG_Tool *, ES_Layout *, ES_Package *);
__END_DECLS
