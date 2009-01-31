/*	Public domain	*/

typedef struct es_vnoise {
	struct es_vsource _inherit;
	M_Real vPrev;			/* Voltage from previous step */
	M_Real vMin, vMax;		/* Voltage range */
	M_Real deltaMax;		/* Smoothness factor */
	char srcPath[256];		/* Random device path */
	FILE *srcFd;			/* Random device handle */
	Uint32 *buf;			/* Random data buffer */
	Uint bufSize;			/* Size of buffer (bytes) */
	Uint bufPos;			/* Position in buffer */
} ES_VNoise;

#define ES_VNOISE(com) ((struct es_vnoise *)(com))

__BEGIN_DECLS
extern ES_ComponentClass esVNoiseClass;
__END_DECLS
