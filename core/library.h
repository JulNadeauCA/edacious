/*	Public domain	*/

typedef AG_Box ES_LibraryEditor;

__BEGIN_DECLS
extern AG_Object *esModelLibrary;

void    ES_LibraryInit(void);
void    ES_LibraryDestroy(void);
void    ES_RegisterModelDirectory(const char *);
void    ES_UnregisterModelDirectory(const char *);
int     ES_LibraryLoad(void);

ES_LibraryEditor *ES_LibraryEditorNew(void *, VG_View *, ES_Circuit *, Uint);
__END_DECLS
