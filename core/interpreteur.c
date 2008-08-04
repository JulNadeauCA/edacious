/*	Public domain	*/

/*
 * Arbitrary mathematical expression parser.
 * Public domain, found here :
 * http://www.developpez.net/forums/showthread.php?t=456224
 * Modifications made :
 * - reorganized file structure
 * - ported code to use M_Real instead of double
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "interpreteur.h"

/* Supprimez la ligne suivante pour entrer en mode debogage */
#define EVALUER_NO_DEBUG

#ifdef  EVALUER_NO_DEBUG
#	define EvaluerDebug Ignorer
#else
#	include <stdio.h>
#	define EvaluerDebug printf
#endif


typedef int    BOOL     ;
#define        FALSE    0
#define        TRUE     1

typedef enum OPERATIONS {
	/* Fonction invalide */
	INVALIDF = -1,

	/* 1. Delimiteurs de bloc*/
	P_O, P_F, END,

	/* 2. Operateurs arithmetiques */
	PLUS, MOINS, FOIS, DIV, POW,

	/* 3. Fonctions */
	OPP, SIN, COS, TAN, ABS, RACINE, LOG, LOG10, EXP, USTEP
}OPERATION;

#define NB_OPERATIONS 18

typedef enum PRIORITY_LEVELS {
	PRIOR_P_F,
	PRIOR_P_O,
	PRIOR_END,
	PRIOR_PLUS_MOINS,
	PRIOR_FOIS_DIV,
	PRIOR_FONCTION,
	PRIOR_POW
}PRIORITY_LEVEL;

typedef enum TYPES_ASSOCIATIVITE {
	GAUCHE, DROITE
}TYPE_ASSOCIATIVITE;

typedef M_Real (*F1VAR)(M_Real);
typedef M_Real (*F2VAR)(M_Real, M_Real);

typedef struct tagOPERATIONINFO {
	/********************************************\
	* Une operation est definie par sa priorite, *
	* son associativite,                         *
	* le nombres d'arguments qu'elle requiert    *
	* l'adresse de la fonction en question       *
	\********************************************/
	PRIORITY_LEVEL        priorite;
	TYPE_ASSOCIATIVITE    associativite;
	size_t                nbArgs;
	void *                p_fonction;
}OPERATIONINFO;

OPERATIONINFO TabOperations[NB_OPERATIONS];

int     EnleverEspaces(char * lpszString);
BOOL    IsSeparator(int c);
int     EmpilerMot(char * lpMot, size_t cchMot, LPPARAM lpParams, size_t nbParams);
int     EmpilerSep(int c);
int     EmpilerOperande(M_Real x);
int     EmpilerChaine(char * lpszMot, LPPARAM lpParams, size_t nbParams);
int     EmpilerFonction(int f);
int     EmpilerParam(char * lpszParam, LPPARAM lpParams, size_t nbParams);
BOOL    Prioritaire(int operation, int second_operation);
int     Evaluer(void);

M_Real  my_Somme(M_Real a, M_Real b);
M_Real  my_Diff(M_Real a, M_Real b);
M_Real  my_Prod(M_Real a, M_Real b);
M_Real  my_Div(M_Real a, M_Real b);
M_Real  my_Pow(M_Real a, M_Real b);
M_Real  my_Opp(M_Real x);
M_Real  my_Sin(M_Real x);
M_Real  my_Cos(M_Real x);
M_Real  my_Tan(M_Real x);
M_Real  my_Abs(M_Real x);
M_Real  my_Racine(M_Real x);
M_Real  my_Log(M_Real x);
M_Real  my_Log10(M_Real x);
M_Real  my_Exp(M_Real x);
M_Real  my_UnitStep(M_Real x);

int     Ignorer(char const * s, ...);

#define MAX_OPERATIONS    20
#define MAX_OPERANDES     20

struct {
	size_t    NbElements;
	int       t[MAX_OPERATIONS];
}Operations;

struct {
	size_t    NbElements;
	M_Real    t[MAX_OPERANDES];
}Operandes;













void InterpreteurInit()
{
	TabOperations[P_O].priorite          = PRIOR_P_O;
	TabOperations[P_O].associativite     = GAUCHE;
	TabOperations[P_O].nbArgs            = 0;
	TabOperations[P_O].p_fonction        = NULL;

	TabOperations[P_F].priorite          = PRIOR_P_F;
	TabOperations[P_F].associativite     = GAUCHE;
	TabOperations[P_F].nbArgs            = 0;
	TabOperations[P_F].p_fonction        = NULL;

	TabOperations[END].priorite          = PRIOR_END;
	TabOperations[END].associativite     = GAUCHE;
	TabOperations[END].nbArgs            = 0;
	TabOperations[END].p_fonction        = NULL;

	TabOperations[PLUS].priorite         = PRIOR_PLUS_MOINS;
	TabOperations[PLUS].associativite    = GAUCHE;
	TabOperations[PLUS].nbArgs           = 2;
	TabOperations[PLUS].p_fonction       = my_Somme;

	TabOperations[MOINS].priorite        = PRIOR_PLUS_MOINS;
	TabOperations[MOINS].associativite   = GAUCHE;
	TabOperations[MOINS].nbArgs          = 2;
	TabOperations[MOINS].p_fonction      = my_Diff;

	TabOperations[FOIS].priorite         = PRIOR_FOIS_DIV;
	TabOperations[FOIS].associativite    = GAUCHE;
	TabOperations[FOIS].nbArgs           = 2;
	TabOperations[FOIS].p_fonction       = my_Prod;

	TabOperations[DIV].priorite          = PRIOR_FOIS_DIV;
	TabOperations[DIV].associativite     = GAUCHE;
	TabOperations[DIV].nbArgs            = 2;
	TabOperations[DIV].p_fonction        = my_Div;

	TabOperations[POW].priorite          = PRIOR_POW;
	TabOperations[POW].associativite     = GAUCHE;
	TabOperations[POW].nbArgs            = 2;
	TabOperations[POW].p_fonction        = my_Pow;

	TabOperations[OPP].priorite          = PRIOR_FONCTION;
	TabOperations[OPP].associativite     = DROITE;
	TabOperations[OPP].nbArgs            = 1;
	TabOperations[OPP].p_fonction        = my_Opp;

	TabOperations[SIN].priorite          = PRIOR_FONCTION;
	TabOperations[SIN].associativite     = DROITE;
	TabOperations[SIN].nbArgs            = 1;
	TabOperations[SIN].p_fonction        = my_Sin;

	TabOperations[COS].priorite          = PRIOR_FONCTION;
	TabOperations[COS].associativite     = DROITE;
	TabOperations[COS].nbArgs            = 1;
	TabOperations[COS].p_fonction        = my_Cos;

	TabOperations[TAN].priorite          = PRIOR_FONCTION;
	TabOperations[TAN].associativite     = DROITE;
	TabOperations[TAN].nbArgs            = 1;
	TabOperations[TAN].p_fonction        = my_Tan;

	TabOperations[ABS].priorite          = PRIOR_FONCTION;
	TabOperations[ABS].associativite     = DROITE;
	TabOperations[ABS].nbArgs            = 1;
	TabOperations[ABS].p_fonction        = my_Abs;

	TabOperations[RACINE].priorite       = PRIOR_FONCTION;
	TabOperations[RACINE].associativite  = DROITE;
	TabOperations[RACINE].nbArgs         = 1;
	TabOperations[RACINE].p_fonction     = my_Racine;

	TabOperations[LOG].priorite          = PRIOR_FONCTION;
	TabOperations[LOG].associativite     = DROITE;
	TabOperations[LOG].nbArgs            = 1;
	TabOperations[LOG].p_fonction        = my_Log;

	TabOperations[LOG10].priorite        = PRIOR_FONCTION;
	TabOperations[LOG10].associativite   = DROITE;
	TabOperations[LOG10].nbArgs          = 1;
	TabOperations[LOG10].p_fonction      = my_Log10;

	TabOperations[EXP].priorite          = PRIOR_FONCTION;
	TabOperations[EXP].associativite     = DROITE;
	TabOperations[EXP].nbArgs            = 1;
	TabOperations[EXP].p_fonction        = my_Exp;

	TabOperations[USTEP].priorite          = PRIOR_FONCTION;
	TabOperations[USTEP].associativite     = DROITE;
	TabOperations[USTEP].nbArgs            = 1;
	TabOperations[USTEP].p_fonction        = my_UnitStep;

}

void InterpreteurReset()
{
	Operations.NbElements = 0;
	Operations.t[0] = INVALIDF;
	Operandes.NbElements = 0;
}

int Calculer(char * lpszString, LPPARAM lpParams, size_t nbParams, M_Real * lpResult)
{
	int ret = EVALUER_SUCCESS;

	if ((lpszString != NULL) && (lpResult != NULL))
	{
		char * lpMot = NULL;
		int c, sep;
		/* c   : caractere courant */
		/* sep : caractere precedent */
		size_t i = 0;

		ret = EnleverEspaces(lpszString);

		lpMot = lpszString;
		sep = c = lpMot[i];

		while (ret == EVALUER_SUCCESS)
		{
			if (IsSeparator(c))
			{
			    /*
			    1. On empile l'eventuel mot
			    2. On empile le separateur
			    3. On deplace le pointeur pMot vers le mot suivant
			    4. On remet i a zero
			    */

			    /* Si c est l'operateur -, trouver sa signification */
				if (c == '-' && i == 0 && sep != ')')
					c = 'o';

				if (i > 0)
					ret = EmpilerMot(lpMot, i, lpParams, nbParams);

				if (ret == EVALUER_SUCCESS)
				{
					ret = EmpilerSep(c);

					if (c == '\0')
						break;
					else
						if (ret == EVALUER_SUCCESS)
						{
							lpMot += (i + 1);
							i = 0;
						}
				}
			}
			else
				i++;

            sep = c;
			c = lpMot[i];
		}

		if (ret == EVALUER_SUCCESS)
		{
			if (Operandes.NbElements == 1)
                if (Operations.NbElements == 0)
                    *lpResult = Operandes.t[0];
                else
					if (Operations.t[Operations.NbElements - 1] == P_O)
						ret = EVALUER_P_F_MANQUANTE;
					else
						ret = EVALUER_FEW_ARGS;
			else
				ret = EVALUER_TOO_MUCH_ARGS;
		}
	}
	else
		ret = EVALUER_NULL_ARG;

	return ret;
}

int EnleverEspaces(char * lpszString)
{
	int ret = EVALUER_SUCCESS;
	char * lpBuffer;

	lpBuffer = malloc(strlen(lpszString) + 1);
	if (lpBuffer != NULL)
	{
		int i = 0, j = 0;
		char c;

		for(i = 0; (c = lpszString[i]) != '\0'; i++)
			if (!isspace(c))
				lpBuffer[j++] = c;
		lpBuffer[j] = '\0';

		strcpy(lpszString, lpBuffer);
		free(lpBuffer);
	}
	else
		ret = EVALUER_MEMOIRE_INSUFFISANTE;

	return ret;
}

BOOL IsSeparator(int c)
{
	BOOL ret;

	switch(c)
	{
	case '+': case '-': case '*': case '/': case '^': case '(': case ')': case '\0':
		ret = TRUE;
		break;

	default:
		ret = FALSE;
	}

	return ret;
}

int EmpilerMot(char * lpMot, size_t cchMot, LPPARAM lpParams, size_t nbParams)
{
	int ret = EVALUER_SUCCESS;
	char * lpszMot = NULL;

	lpszMot = malloc(cchMot + 1);
	if (lpszMot != NULL)
	{
		M_Real x;
		char * p_stop = NULL;

		strncpy(lpszMot, lpMot, cchMot);
		lpszMot[cchMot] = '\0';

		x = strtod(lpszMot, &p_stop);

		/* Si la conversion s'est bien deroulee */
		if (p_stop - lpszMot == (ptrdiff_t)cchMot)
		{
		    /* c'est un nombre (une operande) */
			EvaluerDebug("EmpilerOperande %g\n", x);
			ret = EmpilerOperande(x);
		}
		else
		{
		    /* c'est peut etre le nom d'une fonction ou un parametre ... */
			ret = EmpilerChaine(lpszMot, lpParams, nbParams);
		}

		free(lpszMot);
	}
	else
		ret = EVALUER_MEMOIRE_INSUFFISANTE;

	return ret;
}

int EmpilerOperande(M_Real x)
{
	int ret = EVALUER_SUCCESS;

	if (Operandes.NbElements < MAX_OPERANDES)
	{
		Operandes.t[Operandes.NbElements] = x;
		Operandes.NbElements++;
	}
	else
		ret = EVALUER_PILE_PLEINE;

	return ret;
}

int EmpilerChaine(char * lpszMot, LPPARAM lpParams, size_t nbParams)
{
	int fonction = INVALIDF, ret = EVALUER_SUCCESS;

	if (!strcmp(lpszMot, "sin"))
		fonction = SIN;

	if (!strcmp(lpszMot, "cos"))
		fonction = COS;

	if (!strcmp(lpszMot, "tan"))
		fonction = TAN;

	if (!strcmp(lpszMot, "abs"))
		fonction = ABS;

	if (!strcmp(lpszMot, "sqrt"))
		fonction = RACINE;

	if (!strcmp(lpszMot, "ln"))
		fonction = LOG;

	if (!strcmp(lpszMot, "log"))
		fonction = LOG10;

	if (!strcmp(lpszMot, "exp"))
		fonction = EXP;
	if (!strcmp(lpszMot, "u"))
		fonction = USTEP;

	if (fonction != INVALIDF)
	{
		EvaluerDebug("EmpilerFonction %s\n", lpszMot);
		ret = EmpilerFonction(fonction);
	}
	else
	{
		EvaluerDebug("EmpilerParam %s = ", lpszMot);
		ret = EmpilerParam(lpszMot, lpParams, nbParams);
	}

	return ret;
}

int EmpilerSep(int c)
{
	int ret = EVALUER_SUCCESS;

	if (Operations.NbElements < MAX_OPERATIONS)
	{
		int operation;

		switch(c)
		{
		case '+':
			operation = PLUS;
			break;

		case '-':
			operation = MOINS;
			break;

		case '*':
			operation = FOIS;
			break;

		case '/':
			operation = DIV;
			break;

		case '^':
			operation = POW;
			break;

		case '(':
			operation = P_O;
			break;

		case ')':
			operation = P_F;
			break;

		case '\0':
			operation = END;
			break;

		case 'o':
			operation = OPP;
			break;

		default:
			ret = EVALUER_OPERATION_INCONNUE;
		}

		if (ret == EVALUER_SUCCESS)
		{
			EvaluerDebug("EmpilerFonction %c\n", c == 'o' ? '-' : (c == '\0' ? '$' : c));
			ret = EmpilerFonction(operation);
		}
	}
	else
		ret = EVALUER_PILE_PLEINE;

	return ret;
}

int EmpilerFonction(int f)
{
	int p_o = 0, ret = EVALUER_SUCCESS;

	if (f != P_O)
	{
		int operation_courante;

		operation_courante = Operations.t[Operations.NbElements - 1];

		while ((Operations.NbElements > 0) && Prioritaire(operation_courante, f))
		{
			EvaluerDebug("IRQ!\n");
			p_o = (operation_courante == P_O);
			ret = Evaluer();

			if ((ret != EVALUER_SUCCESS) || ((f == P_F) && (p_o)))
				break;

			if (Operations.NbElements > 0)
				operation_courante = Operations.t[Operations.NbElements - 1];
		}
	}

	if (ret == EVALUER_SUCCESS)
	{
		if ((f == P_F) || (f == END))
		{
			if ((f == P_F) && (!p_o))
				ret = EVALUER_P_O_MANQUANTE;
		}
		else
		{
			if (Operations.NbElements < MAX_OPERATIONS)
			{
				Operations.t[Operations.NbElements] = f;
				Operations.NbElements++;
			}
			else
				ret = EVALUER_PILE_PLEINE;
		}
	}

	return ret;
}

BOOL Prioritaire(int operation, int second_operation)
{
	int prior_operation, prior_second, associativite;

	prior_operation = TabOperations[operation].priorite;
	prior_second = TabOperations[second_operation].priorite;
	associativite = TabOperations[operation].associativite;

	return ((prior_operation > prior_second) || ((prior_operation == prior_second) && (associativite == GAUCHE)));
}

int EmpilerParam(char * lpszParam, LPPARAM lpParams, size_t nbParams)
{
	int ret = EVALUER_SUCCESS;

	if (lpParams != NULL)
	{
		size_t i;
		BOOL found = FALSE;
		M_Real x = 0.0;			/* make compiler happy */

        /* On recherche lpszParam dans lpParams */
		for(i = 0; i < nbParams; i++)
			if (!strcmp(lpParams[i].lpszName, lpszParam))
			{
				found = TRUE;
				x = lpParams[i].Value;
				break;
			}

		if (found)
		{
		    /* on empile la valeur du parametre */
			EvaluerDebug("EmpilerOperande %g\n", x);
			ret = EmpilerOperande(x);
		}
		else
		{
			EvaluerDebug("Parametre Indefini !\n");
			ret = EVALUER_PARAM_INDEFINI;
		}
	}
	else
		ret = EVALUER_PARAM_INDEFINI;

	return ret;
}

int Evaluer()
{
	int ret = EVALUER_SUCCESS;

	if (Operations.NbElements > 0)
	{
		int operation;
		size_t nbArgs, nbOperandes;
		void * p_fonction;

		operation = Operations.t[Operations.NbElements - 1];
		nbArgs = TabOperations[operation].nbArgs;
		p_fonction = TabOperations[operation].p_fonction;

		nbOperandes = Operandes.NbElements;

		if (nbOperandes >= nbArgs)
		{
			if (p_fonction != NULL)
			{
				switch(nbArgs)
				{
				case 1:
					Operandes.t[nbOperandes - 1] = ((F1VAR)p_fonction)(Operandes.t[nbOperandes - 1]);
					break;

				case 2:
					Operandes.t[nbOperandes - 2] = ((F2VAR)p_fonction)(Operandes.t[nbOperandes - 2], Operandes.t[nbOperandes - 1]);
					Operandes.NbElements--;
					break;

				default:
					ret = EVALUER_UNSUPPORTED_OPERATION;
				}
			}

			Operations.NbElements--;
		}
		else
			ret = EVALUER_FEW_ARGS;
	}
	else
		ret = EVALUER_OPERATION_MANQUANTE;

	return ret;
}

M_Real my_Somme(M_Real a, M_Real b)
{
	M_Real y;
	y = a + b;
	EvaluerDebug("%g + %g = %g\n", a, b, y);
	return y;
}

M_Real my_Diff(M_Real a, M_Real b)
{
	M_Real y;
	y = a - b;
	EvaluerDebug("%g - %g = %g\n", a, b, y);
	return y;
}

M_Real my_Prod(M_Real a, M_Real b)
{
	M_Real y;
	y = a * b;
	EvaluerDebug("%g * %g = %g\n", a, b, y);
	return y;
}

M_Real my_Div(M_Real a, M_Real b)
{
	M_Real y;
	y = a / b;
	EvaluerDebug("%g / %g = %g\n", a, b, y);
	return y;
}

M_Real my_Pow(M_Real a, M_Real b)
{
	M_Real y;
	y = pow(a, b);
	EvaluerDebug("%g ^ %g = %g\n", a, b, y);
	return y;
}

M_Real my_Opp(M_Real x)
{
	M_Real y;
	y = -x;
	EvaluerDebug("-%g = %g\n", x, y);
	return y;
}

M_Real my_Sin(M_Real x)
{
	M_Real y;
	y = Sin(x);
	EvaluerDebug("sin(%g) = %g\n", x, y);
	return y;
}

M_Real my_Cos(M_Real x)
{
	M_Real y;
	y = Cos(x);
	EvaluerDebug("cos(%g) = %g\n", x, y);
	return y;
}

M_Real my_Tan(M_Real x)
{
	M_Real y;
	y = Tan(x);
	EvaluerDebug("tan(%g) = %g\n", x, y);
	return y;
}

M_Real my_Abs(M_Real x)
{
	M_Real y;
	y = Fabs(x);
	EvaluerDebug("abs(%g) = %g\n", x, y);
	return y;
}

M_Real my_Racine(M_Real x)
{
	M_Real y;
	y = Sqrt(x);
	EvaluerDebug("sqrt(%g) = %g\n", x, y);
	return y;
}

M_Real my_Log(M_Real x)
{
	M_Real y;
	y = Log(x);
	EvaluerDebug("ln(%g) = %g\n", x, y);
	return y;
}

M_Real my_Log10(M_Real x)
{
	M_Real y;
	y = Log(x) / Log(10);
	EvaluerDebug("log(%g) = %g\n", x, y);
	return y;
}

M_Real my_Exp(M_Real x)
{
	M_Real y;
	y = Exp(x);
	EvaluerDebug("exp(%g) = %g\n", x, y);
	return y;
}

M_Real my_UnitStep(M_Real x)
{
	M_Real y;
	y = x > 0 ? 1 : 0;
	EvaluerDebug("u(%g) = %g\n", x, y);
	return y;
}

int Ignorer(char const * s, ...)
{
    return (int)strlen(s);
}
