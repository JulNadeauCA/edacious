/*	$Csoft: tool.h,v 1.3 2004/08/24 01:32:12 vedge Exp $	*/
/*	Public domain	*/

#include <engine/widget/fspinbutton.h>

#ifndef _MGATE_TOOL_H_
#define _MGATE_TOOL_H_
#include "begin_code.h"

struct eda_tool {
	char	 *name;
	int	  icon;
	void	(*init)(void);
	void	(*destroy)(void);
	struct window *win;
};

struct eda_type {
	const char *name;
	size_t size;
	const void *ops;
};

__BEGIN_DECLS
struct fspinbutton *bind_double(struct window *, const char *,
                                   double *,
				   void (*)(int, union evarg *), const char *);
struct fspinbutton *bind_double_ro(struct window *, const char *,
                                   double *, const char *);
__END_DECLS

#include "close_code.h"
#endif /* _MGATE_TOOL_H_ */
