/*	Public domain	*/

typedef AG_Box ES_SchemLibraryEditor;

__BEGIN_DECLS
extern AG_Object *esSchemLibrary;
extern char **esSchemLibraryDirs;
extern int    esSchemLibraryDirCount;

void    ES_SchemLibraryInit(void);
void    ES_SchemLibraryDestroy(void);
void    ES_SchemLibraryRegisterDir(const char *);
void    ES_SchemLibraryUnregisterDir(const char *);
int     ES_SchemLibraryLoad(void);

ES_SchemLibraryEditor *ES_SchemLibraryEditorNew(void *, VG_View *, Uint);
__END_DECLS
