/*	Public domain	*/

/* Available integration methods */
enum es_integration_method {
	BE,                     /* Backwards Euler */
	FE,                     /* Forward Euler */
	TR,                     /* Trapezoidal rule */
	G2                      /* Second order gear */
};
__BEGIN_DECLS
extern const char *methodNames[];
extern const int isImplicit[];
extern const int methodOrder[];
extern const M_Real methodErrorConstant[];
__END_DECLS
