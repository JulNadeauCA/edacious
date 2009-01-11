/*	Public domain	*/

typedef AG_Box ES_PackageLibraryEditor;

__BEGIN_DECLS
extern AG_Object *esPackageLibrary;
extern char **esPackageLibraryDirs;
extern int    esPackageLibraryDirCount;

void    ES_PackageLibraryInit(void);
void    ES_PackageLibraryDestroy(void);
void    ES_PackageLibraryRegisterDir(const char *);
void    ES_PackageLibraryUnregisterDir(const char *);
int     ES_PackageLibraryLoad(void);

ES_PackageLibraryEditor *ES_PackageLibraryEditorNew(void *, VG_View *, ES_Layout *, Uint, AG_EventFn, const char *, ...);
__END_DECLS
