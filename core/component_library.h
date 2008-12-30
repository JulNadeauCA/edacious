/*	Public domain	*/

typedef AG_Box ES_ComponentLibraryEditor;

__BEGIN_DECLS
extern AG_Object *esComponentLibrary;
extern char **esComponentLibraryDirs;
extern int    esComponentLibraryDirCount;

void    ES_ComponentLibraryInit(void);
void    ES_ComponentLibraryDestroy(void);
void    ES_ComponentLibraryRegisterDir(const char *);
void    ES_ComponentLibraryUnregisterDir(const char *);
int     ES_ComponentLibraryLoad(void);

ES_ComponentLibraryEditor *ES_ComponentLibraryEditorNew(void *, VG_View *,
                                                        ES_Circuit *, Uint);
__END_DECLS
