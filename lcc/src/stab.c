#include <string.h>
#include <stdlib.h>
#include "c.h"
#include "stab.h"

static char rcsid[] = "$Id$";

static char *currentfile;       /* current file name */
static int ntypes;
static Type missing[16];
static int indmissing;

extern Interface sparcIR;

char *stabprefix = "L";

extern char *stabprefix;
extern void stabblock(int, int, Symbol*);
extern void stabend(Coordinate *, Symbol, Coordinate **, Symbol *, Symbol *);
extern void stabfend(Symbol, int);
extern void stabinit(char *, int, char *[]);
extern void stabline(Coordinate *);
extern void stabsym(Symbol);
extern void stabtype(Symbol);

static void dbxout(Type);
static int dbxtype(Type);
static void emittype(Type);


/* dbxout - output .stabs entry for type ty */
static void dbxout(Type ty) {
	ty = unqual(ty);
	if (!ty->x.printed) {		
		print("\t.stabs \"");
		if (ty->u.sym && !(isfunc(ty) || isarray(ty) || isptr(ty)))
			print("%s", ty->u.sym->name);
		print(":%c", isstruct(ty) || isenum(ty) ? 'T' : 't');
		emittype(ty);
		print("\",%d,0,0,0\n", N_LSYM);
	}
}

/* dbxtype - emit a stabs entry for type ty, return type code */
static int dbxtype(Type ty) {	
    indmissing = 0;
	dbxout(ty);
    while(indmissing) dbxout(missing[indmissing--]);
	return ty->x.typeno;
}

/*
 * emittype - emit ty's type number, emitting its definition if necessary.
 * Returns the output column number after emission; col is the approximate
 * output column before emission and is used to emit continuation lines for long
 * struct, union, and enum types. Continuations are not emitted for other types,
 * even if the definition is long. lev is the depth of calls to emittype.
 */
static void emittype(Type ty) {
	int tc = ty->x.typeno;
    unsigned tmp;         
	if (isconst(ty) || isvolatile(ty)) {
		emittype(ty->type);
		ty->x.typeno = ty->type->x.typeno;
		ty->x.printed = 1;
		return;
	}
	if (tc == 0) {
		ty->x.typeno = tc = ++ntypes;
	}
	print("%d", tc);
	if (ty->x.printed)
		return;
	ty->x.printed = 1;
	switch (ty->op) {
	case VOID:	/* void is defined as itself */
		print("=r1");
		break;
	case INT:		
		print("=r1;%D;%D;", ty->u.sym->u.limits.min.i, ty->u.sym->u.limits.max.i);		
		break;
	case UNSIGNED:		
		print("=r1;0;%U;", ty->u.sym->u.limits.max.i);		
		break;
	case FLOAT:	/* float, double, long double get sizes, not ranges */
		print("=r1;%d;0;", ty->size);
		break;
	case POINTER:
        print("=*");
        tmp = ty->type->x.printed;
        if(!tmp) {                    
            ty->type->x.printed = 1;
            missing[++indmissing] = ty->type;
        }		
		emittype(ty->type);
        ty->type->x.printed = tmp;
		break;
	case FUNCTION:
		print("=f");
		emittype(ty->type);
		break;
	case ARRAY:	/* array includes subscript as an int range */
		print("=a");
        tmp = ty->type->x.printed;
        if(!tmp) {                    
            ty->type->x.printed = 1;
            missing[++indmissing] = ty->type;
        }				
        emittype(ty->type);
        ty->type->x.printed = tmp;
		print(";0;%d;", ty->size/ty->type->size - 1);        
		break;
	case STRUCT: case UNION: {        
        Field p;        
		print("=%c%d", ty->op == STRUCT ? 's' : 'u', ty->size);
        print(";");
        for (p = fieldlist(ty); p; p = p->link) {
			if (p->name)
				print("%s,", p->name);
			else
				print(",");
            tmp = p->type->x.printed;
            if(!tmp) {                    
                p->type->x.printed = 1;
                missing[++indmissing] = p->type;
            }
            emittype(p->type);
            p->type->x.printed = tmp;                
            if (p->lsb)
				print(",%d;", 8*p->offset +
				(IR->little_endian ? fieldright(p) : fieldleft(p)));
			else
				print(",%d;", 8*p->offset);				
		}                
        break;		
        }
	case ENUM: {
		Symbol *p;		
		print("=e;");
		for (p = ty->u.sym->u.idlist; *p; p++) {
			print("%s:%d,", (*p)->name, (*p)->u.value);			
		}
		print(";");
		break;
		}
	//default:
		//assert(0);
	}
	return;
}

/* stabblock - output a stab entry for '{' or '}' at level lev */
void stabblock(int brace, int lev, Symbol *p) {
	if (brace == '{')
		while (*p)
			stabsym(*p++);
	if (IR == &sparcIR)
		print(".stabd 0x%x,0,%d\n", brace == '{' ? N_LBRAC : N_RBRAC, lev);
	else {
		int lab = genlabel(1);
		print("\t.stabn 0x%x,0,%d,%s%d_%s\n", brace == '{' ? N_LBRAC : N_RBRAC, lev,
			stabprefix, lab, cfunc->x.name);
		print("%s%d:\n", stabprefix, lab);
	}
}

/* stabinit - initialize stab output */
void stabinit(char *file, int argc, char *argv[]) {
	typedef void (*Closure)(Symbol, void *);
	extern char *getcwd(char *, size_t);
	
	if (file && *file) {
		print("\t.stabs \"lcc4_compiled.\",0x%x,0,0,0\n", N_OPT);
		char buf[1024], *cwd = getcwd(buf, sizeof buf);
		if (cwd)
			print("\t.stabs \"%s/\",0x%x,0,3,%stext0\n", cwd, N_SO, stabprefix);
		print("\t.stabs \"%s\",0x%x,0,3,%stext0\n", file, N_SO, stabprefix);
		(*IR->segment)(CODE);
		print("%stext0:\n", stabprefix, N_SO);
		currentfile = file;
	}
	dbxtype(inttype);
	dbxtype(chartype);
	dbxtype(doubletype);
	dbxtype(floattype);
	dbxtype(longdouble);
	dbxtype(longtype);
	dbxtype(longlong);
	dbxtype(shorttype);
	dbxtype(signedchar);
	dbxtype(unsignedchar);
	dbxtype(unsignedlong);
	dbxtype(unsignedlonglong);
	dbxtype(unsignedshort);
	dbxtype(unsignedtype);
	dbxtype(voidtype); 
	
}

/* stabline - emit stab entry for source coordinate *cp */
void stabline(Coordinate *cp) {
	if (cp->file && cp->file != currentfile) {
		int lab = genlabel(1);
		print("\t.stabs \"%s\",0x%x,0,0,%s%d\n", cp->file, N_SOL, stabprefix, lab);
		print("%s%d:\n", stabprefix, lab);
		currentfile = cp->file;
	}
	if (IR == &sparcIR)
		print("\t.stabd 0x%x,0,%d\n", N_SLINE, cp->y);
	else {
		int lab = genlabel(1);
		print("\t.stabn 0x%x,0,%d,%s%d-%s\n", N_SLINE, cp->y,
			stabprefix, lab, cfunc->x.name);
		print("%s%d:\n", stabprefix, lab);
	}
}

/* stabsym - output a stab entry for symbol p */
void stabsym(Symbol p) {
	int code, tc, sz = p->type->size;

	if (p->generated || p->computed)
		return;
	if (isfunc(p->type)) {
		print("\t.stabs \"%s:%c%d\",%d,0,0,%s\n", p->name,
			p->sclass == STATIC ? 'f' : 'F', dbxtype(freturn(p->type)),
			N_FUN, p->x.name);
		return;
	}
	if (!IR->wants_argb && p->scope == PARAM && p->structarg) {
		assert(isptr(p->type) && isstruct(p->type->type));
		tc = dbxtype(p->type->type);
		sz = p->type->type->size;
	} else
		tc = dbxtype(p->type);
	if (p->sclass == AUTO && p->scope == GLOBAL || p->sclass == EXTERN) {
		print("\t.stabs \"%s:G", p->name);
		code = N_GSYM;
	} else if (p->sclass == STATIC) {
		print("\t.stabs \"%s:%c%d\",%d,0,0,%s\n", p->name, p->scope == GLOBAL ? 'S' : 'V',
			tc, p->u.seg == BSS ? N_LCSYM : N_STSYM, p->x.name);
		return;
	} else if (p->sclass == REGISTER) {
		if (p->x.regnode) {
			int r = p->x.regnode->number;
			if (p->x.regnode->set == FREG)
				r += 32;	/* floating point */
				print("\t.stabs \"%s:%c%d\",%d,0,", p->name,
					p->scope == PARAM ? 'P' : 'r', tc, N_RSYM);
			print("%d,%d\n", sz, r);
		}
		return;
	} else if (p->scope == PARAM) {
		print("\t.stabs \"%s:p", p->name);
		code = N_PSYM;
	} else if (p->scope >= LOCAL) {
		print("\t.stabs \"%s:", p->name);
		code = N_LSYM;
	} else
		assert(0);
	print("%d\",%d,0,0,%s\n", tc, code,
		p->scope >= PARAM && p->sclass != EXTERN ? p->x.name : "0");
}

/* stabtype - output a stab entry for type *p */
void stabtype(Symbol p) {	
    if (p->type) {
		if (p->sclass == 0)
			dbxtype(p->type);
		else if (p->sclass == TYPEDEF)
			print("\t.stabs \"%s:=d%d\",%d,0,0,0\n", p->name, dbxtype(p->type), N_LSYM);            
	}    
}

/* stabend - finalize a function */
void stabfend(Symbol p, int lineno) {}

/* stabend - finalize stab output */
void stabend(Coordinate *cp, Symbol p, Coordinate **cpp, Symbol *sp, Symbol *stab) {
	(*IR->segment)(CODE);
	print("\t.stabs \"\", %d, 0, 0,%setext\n", N_SO, stabprefix);
	print("%setext:\n", stabprefix);
}
