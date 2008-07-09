/* Matrix Wrapper : wrapper for matrices in eda. Let the user decide at compile time
 * the backend he wants to use */

#if 1/*USE_M_BACKEND*/
#define MW_Matrix M_Matrix
#define MW_Vector M_Vector
#define MW_GetElement(A, k, l) &(A->v[k][l])
#define MW_GetElementVector(z, k) &(z->v[k][0])

#define MW_CopyVector(A, B, n) M_Copy(A,B)
#define MW_FreeVector M_Free
#define MW_NewVector(n) M_New(n, 1)

#elif defined(USE_SPARSE_BACKEND)
#define MW_Matrix char /* I guess type safety didn't exist back in 88 ... */
#define MW_Vector M_Real
#define MW_GetElement spGetElement
#define MW_GetElementVector(z, k) (z+k)

#define MW_CopyVector(A, B, n) memcpy(A, B, (n)*sizeof(M_Real))
#define MW_FreeVector free
#define MW_NewVector(n) (M_Real *) malloc(n*sizeof(M_Real))

#include "../SPARSE/spmatrix.h"
#endif /* backends */
