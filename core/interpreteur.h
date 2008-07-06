/* Arbitrary mathematical expression parser.
 * Public domain, found here :
 * http://www.developpez.net/forums/showthread.php?t=456224
 * Modifications made :
 * - reorganized file structure
 * - ported code to use M_Real instead of double
 * */


#ifndef INTERPRETEUR_H

#define INTERPRETEUR_H

/*
Fichier : interpreteur.h
------------------------
    Fichier d'en-tete pour l'utilisation de l'interpreteur
*/

#include <stddef.h>
#include <eda.h>

typedef enum EVALUER_ERRORS {
	EVALUER_SUCCESS,
	EVALUER_NULL_ARG,
	EVALUER_FEW_ARGS,
	EVALUER_TOO_MUCH_ARGS,
	EVALUER_P_O_MANQUANTE,
	EVALUER_P_F_MANQUANTE,
	EVALUER_PILE_PLEINE,
	EVALUER_MEMOIRE_INSUFFISANTE,
	EVALUER_PARAM_INDEFINI,
	EVALUER_OPERATION_INCONNUE,
	EVALUER_OPERATION_MANQUANTE,
	EVALUER_UNSUPPORTED_OPERATION
}EVALUER_ERROR;

typedef struct tagPARAM {
	char * lpszName;
	M_Real Value;
}PARAM, * LPPARAM;

void  InterpreteurInit(void);
void  InterpreteurReset(void);
int   Calculer(char * lpszString, LPPARAM lpParams, size_t nbParams, M_Real * lpResult);

#endif

