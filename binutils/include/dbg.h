
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
static inline void update_typdefarrvalue (typdefarr_t *p, const typdefarr_t val) { typdefarr_set(*p, val); } 
#define M_OPL_typdefarr_t() M_OPEXTEND(ARRAY_OPLIST(typdefarr, M_OPL_typdef_t() ), UPDATE(update_typdefarrvalue M_IPTR) )

DICT_DEF2(typdefdict, string_t, typdefarr_t)
#define M_OPL_typdefdict_t() DICT_OPLIST(typdefdict, STRING_OPLIST, M_OPL_typdefarr_t() )

TUPLE_DEF2(funcvar,
           (name, string_t),
           (type, int),
		   (pos, int) )
#define M_OPL_funcvar_t() TUPLE_OPLIST(funcvar, STRING_OPLIST, M_DEFAULT_OPLIST,  M_DEFAULT_OPLIST)

ARRAY_DEF(funcvararr, funcvar_t)
#define M_OPL_funcvararr_t() ARRAY_OPLIST(funcvararr, M_OPL_funcvar_t() )


TUPLE_DEF2(func,
           (filename, string_t),
           (startpos, int),
		   (endpos, int),
		   (rettype, int),
		   (stacklocals, funcvararr_t),
		   (stackargs, funcvararr_t),
		   (reglocals, funcvararr_t),
		   (regargs, funcvararr_t) )
static inline void update_funcvalue (func_t *p, const func_t val) { func_set(*p, val); } 
#define M_OPL_func_t()  M_OPEXTEND(TUPLE_OPLIST(func, STRING_OPLIST, M_DEFAULT_OPLIST, M_DEFAULT_OPLIST, M_DEFAULT_OPLIST, M_OPL_funcvararr_t(), M_OPL_funcvararr_t(), M_OPL_funcvararr_t(), M_OPL_funcvararr_t()) , UPDATE(update_funcvalue M_IPTR) )
 


//ARRAY_DEF(funcarr, func_t)
//#define M_OPL_funcarr_t() ARRAY_OPLIST(funcarr, M_OPL_func_t() )
DICT_DEF2(funcdict, string_t, func_t)
#define M_OPL_funcdict_t() DICT_OPLIST(funcdict, STRING_OPLIST, M_OPL_func_t() )

TUPLE_DEF2(root,
           (locations, locarr_t),
		   (functions, funcdict_t),
           (globals, globarr_t),
           (typdefs, typdefdict_t) )
#define M_OPL_root_t() TUPLE_OPLIST(root, M_OPL_locarr_t(), M_OPL_funcdict_t(), M_OPL_globarr_t(), M_OPL_typdefdict_t() )


#endif 
