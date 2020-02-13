/*	Public domain	*/

struct es_circuit;

/* Device package description */
typedef struct es_package {
	struct es_layout _inherit;		/* ES_Layout -> ES_Package */
	Uint flags;
#define ES_PACKAGE_CONNECTED	0x01		/* Connected to layout */
	TAILQ_HEAD(,vg_node) layoutEnts;	/* Entities in Layout VG */
	TAILQ_ENTRY(es_package) packages;	/* In layout */
} ES_Package;

#define ESPACKAGE(p)             ((ES_Package *)(p))
#define ESCPACKAGE(obj)          ((const ES_Package *)(obj))
#define ES_PACKAGE_SELF()          ESPACKAGE( AG_OBJECT(0,"ES_Layout:ES_Package:*") )
#define ES_PACKAGE_PTR(n)          ESPACKAGE( AG_OBJECT((n),"ES_Layout:ES_Package:*") )
#define ES_PACKAGE_NAMED(n)        ESPACKAGE( AG_OBJECT_NAMED((n),"ES_Layout:ES_Package:*") )
#define ES_CONST_PACKAGE_SELF()   ESCPACKAGE( AG_CONST_OBJECT(0,"ES_Layout:ES_Package:*") )
#define ES_CONST_PACKAGE_PTR(n)   ESCPACKAGE( AG_CONST_OBJECT((n),"ES_Layout:ES_Package:*") )
#define ES_CONST_PACKAGE_NAMED(n) ESCPACKAGE( AG_CONST_OBJECT_NAMED((n),"ES_Layout:ES_Package:*") )

__BEGIN_DECLS
extern AG_ObjectClass esPackageClass;
ES_Package *ES_PackageNew(struct es_circuit *);
int         ES_PackageTool_InsertDevicePkg(VG_Tool *, ES_Layout *, ES_Package *);
__END_DECLS
