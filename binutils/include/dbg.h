
#include "m-serial-json.h"
#include "m-tuple.h"
#include "m-array.h"
#include "m-dict.h"
#include "m-rbtree.h"

// Serial json is not supported for standard types if not C11
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L  


TUPLE_DEF2(loc,
           (filename, string_t),
		   (row, int),
		   (pos, int) )
#define M_OPL_loc_t() TUPLE_OPLIST(loc, STRING_OPLIST, M_DEFAULT_OPLIST, M_DEFAULT_OPLIST)

ARRAY_DEF(locarr, loc_t)
#define M_OPL_locarr_t() ARRAY_OPLIST(locarr, M_OPL_loc_t() )
 
TUPLE_DEF2(global,
           (name, string_t),
		   (type, int),
		   (pos, int) )
#define M_OPL_global_t() TUPLE_OPLIST(global, STRING_OPLIST, M_DEFAULT_OPLIST, M_DEFAULT_OPLIST)

ARRAY_DEF(globarr, global_t)
#define M_OPL_globarr_t() ARRAY_OPLIST(globarr, M_OPL_global_t() )

TUPLE_DEF2(typdef,
           (name, string_t),
           (number, int),
		   (desc, string_t) )
#define M_OPL_typdef_t() TUPLE_OPLIST(typdef, STRING_OPLIST, M_DEFAULT_OPLIST, STRING_OPLIST)

ARRAY_DEF(typdefarr, typdef_t)
#define M_OPL_typdefarr_t() ARRAY_OPLIST(typdefarr, M_OPL_typdef_t() )

TUPLE_DEF2(funcvar,
           (name, string_t),
           (type, int),
		   (pos, int) )
#define M_OPL_funcvar_t() TUPLE_OPLIST(funcvar, STRING_OPLIST, M_DEFAULT_OPLIST,  M_DEFAULT_OPLIST)

ARRAY_DEF(funcvararr, funcvar_t)
#define M_OPL_funcvararr_t() ARRAY_OPLIST(funcvararr, M_OPL_funcvar_t() )

TUPLE_DEF2(func,
           (startpos, int),
		   (endpos, int),
		   (rettype, int),
		   (stacklocals, funcvararr_t),
		   (stackargs, funcvararr_t),
		   (reglocals, funcvararr_t),
		   (regargs, funcvararr_t) )
#define M_OPL_func_t() TUPLE_OPLIST(func, M_DEFAULT_OPLIST, M_DEFAULT_OPLIST, M_DEFAULT_OPLIST, M_OPL_funcvararr_t(), M_OPL_funcvararr_t(), M_OPL_funcvararr_t(), M_OPL_funcvararr_t())

//ARRAY_DEF(funcarr, func_t)
//#define M_OPL_funcarr_t() ARRAY_OPLIST(funcarr, M_OPL_func_t() )
DICT_DEF2(funcdict, string_t, func_t)
#define M_OPL_funcdict_t() DICT_OPLIST(funcdict, STRING_OPLIST, M_OPL_func_t() )

TUPLE_DEF2(root,
           (locations, locarr_t),
		   (functions, funcdict_t),
           (globals, globarr_t),
           (typdefs, typdefarr_t) )
#define M_OPL_root_t() TUPLE_OPLIST(root, M_OPL_locarr_t(), M_OPL_globarr_t(), M_OPL_typdefarr_t() )


#endif 
