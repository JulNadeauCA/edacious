/*	Public domain	*/

/* Available integration methods */
enum es_integration_method {
	BE,                     /* Backwards Euler */
	FE,                     /* Forward Euler */
	TR,                     /* Trapezoidal rule */
	G2                      /* Second order gear */
};

typedef struct {
	enum es_integration_method method;
	const char *name;			/* Method name */
	const char *desc;			/* Long name */
	int isImplicit;				/* Implicit method? */
	int order;				/* Order */
	M_Real errConst;			/* Error constant */
} ES_IntegrationMethod;

#define ES_IMPLICIT_METHOD(n)	(esIntegrationMethods[n].isImplicit)
#define ES_METHOD_ORDER(n)	(esIntegrationMethods[n].order)
#define ES_METHOD_ERRCONST(n)	(esIntegrationMethods[n].errConst)

__BEGIN_DECLS
extern const ES_IntegrationMethod esIntegrationMethods[];
extern const int esIntegrationMethodCount;
__END_DECLS
