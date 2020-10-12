#include <string.h>
#include <stdlib.h>
#include "c.h"
#include "stab.h"

#include "dwarf.h"

#define emit(...) print( __VA_ARGS__)

static char rcsid[] = "$Id$";

static char *currentfile;       /* current file name */
static int ntypes;
static int nsyms;
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




static int emit_uint8(uint8_t value)
{
    emit("\t.byte %u\n", value);
    return 1;
}
 
static unsigned get_uleb128_size(unsigned value)
{
	unsigned size = 0;
	do {
		value >>= 7;
		size += 1;
	} while (value != 0);
	return size;
} 

static int emit_uleb128(unsigned value)
{
    emit("\t.uleb128 0x%x\n", value);
    return get_uleb128_size(value);
}

static unsigned get_sleb128_size(long value)
{
	unsigned size = 0;
	do {
		value >>= 7;
		size += 1;
	} while (value != 0 && value != -1);
	return size;
}
 

static int emit_sleb128(long value)
{
    emit("\t.sleb128 %d\n", value);
    return get_sleb128_size(value);
}

static int emit_string(const char *str)
{   
    emit("\t.asciz \"%s\"\n",str);
    return(strlen(str)+1);
}

static void emit_basetype_abrev(){
    emit_uleb128(DW_TAG_base_type);
    emit_uleb128(DW_TAG_base_type);
    emit_uint8(0);
    emit_uleb128(DW_AT_name);
    emit_uleb128(DW_FORM_string);
    emit_uleb128(DW_AT_encoding);
    emit_uleb128(DW_FORM_data1);
    emit_uleb128(DW_AT_byte_size);
    emit_uleb128(DW_FORM_data1);
    emit_uleb128(0);
    emit_uleb128(0);
}
 
static void emit_ptrtype_abrev(){
    emit_uleb128(DW_TAG_pointer_type);
    emit_uleb128(DW_TAG_pointer_type);
    emit_uint8(0);
    emit_uleb128(DW_AT_type);
    emit_uleb128(DW_FORM_ref_addr);
    emit_uleb128(0);
    emit_uleb128(0);
}

static void emit_enumtype_abrev(){
    emit_uleb128(DW_TAG_enumeration_type);
    emit_uleb128(DW_TAG_enumeration_type);
    emit_uint8(1);
    emit_uleb128(0);
    emit_uleb128(0);
    emit_uleb128(DW_TAG_enumerator);
    emit_uleb128(DW_TAG_enumerator);
    emit_uint8(0);
    emit_uleb128(DW_AT_name);
    emit_uleb128(DW_FORM_string);
    emit_uleb128(DW_AT_const_value);
    emit_uleb128(DW_FORM_data1);
    emit_uleb128(0);
    emit_uleb128(0);
}

static void emit_arraytype_abrev(){
    emit_uleb128(DW_TAG_array_type);
    emit_uleb128(DW_TAG_array_type);
    emit_uint8(1);
    emit_uleb128(DW_AT_type);
    emit_uleb128(DW_FORM_ref_addr);
    emit_uleb128(0);
    emit_uleb128(0);
    
    emit_uleb128(DW_TAG_subrange_type);
    emit_uleb128(DW_TAG_subrange_type);
    emit_uint8(0);
    emit_uleb128(DW_AT_upper_bound);
    emit_uleb128(DW_FORM_udata);
    emit_uleb128(0);
    emit_uleb128(0);
}


static void emit_subroutine_type_abrev(){
    emit_uleb128(DW_TAG_subroutine_type);
    emit_uleb128(DW_TAG_subroutine_type);
    emit_uint8(0);
    emit_uleb128(DW_AT_type);
    emit_uleb128(DW_FORM_ref_addr);
    emit_uleb128(0);
    emit_uleb128(0);
}

static void emit_subprogram_abrev(){
    emit_uleb128(DW_TAG_subprogram);
    emit_uleb128(DW_TAG_subprogram);
    emit_uint8(1);
    emit_uleb128(DW_AT_name);
    emit_uleb128(DW_FORM_string);
    emit_uleb128(DW_AT_type);
    emit_uleb128(DW_FORM_ref_addr);
    emit_uleb128(0);
    emit_uleb128(0);
    
    emit_uleb128(DW_TAG_formal_parameter);
    emit_uleb128(DW_TAG_formal_parameter);
    emit_uint8(0);
    emit_uleb128(DW_AT_name);
    emit_uleb128(DW_FORM_string);
    emit_uleb128(DW_AT_type);
    emit_uleb128(DW_FORM_ref_addr);
    emit_uleb128(DW_AT_location);
    emit_uleb128(DW_FORM_block1);
    emit_uleb128(0);
    emit_uleb128(0);
}

static void emit_compound_abrev(){
    emit_uleb128(DW_TAG_structure_type);
    emit_uleb128(DW_TAG_structure_type);
    emit_uint8(1);
    emit_uleb128(DW_AT_name);
    emit_uleb128(DW_FORM_string);
    emit_uleb128(DW_AT_byte_size);
    emit_uleb128(DW_FORM_udata);
    emit_uleb128(0);
    emit_uleb128(0);
    
    emit_uleb128(DW_TAG_union_type);
    emit_uleb128(DW_TAG_union_type);
    emit_uint8(1);
    emit_uleb128(DW_AT_name);
    emit_uleb128(DW_FORM_string);
    emit_uleb128(DW_AT_byte_size);
    emit_uleb128(DW_FORM_udata);
    emit_uleb128(0);
    emit_uleb128(0);
    
    emit_uleb128(DW_TAG_member);
    emit_uleb128(DW_TAG_member);
    emit_uint8(0);
    emit_uleb128(DW_AT_name);
    emit_uleb128(DW_FORM_string);
    emit_uleb128(DW_AT_type);
    emit_uleb128(DW_FORM_ref_addr);
    emit_uleb128(DW_AT_data_member_location);
    emit_uleb128(DW_FORM_block1);
    emit_uleb128(0);
    emit_uleb128(0);
}

static void emit_compile_abrev(){
    emit_uleb128(DW_TAG_compile_unit);
    emit_uleb128(DW_TAG_compile_unit);
    emit_uint8(1);
    emit_uleb128(DW_AT_name);
    emit_uleb128(DW_FORM_string);
    emit_uleb128(0);
    emit_uleb128(0);
}

static int emit_basetype(int typeno, char* name, uint8_t encoding, uint8_t size) {
    int csize = 0;
    emit("T%d:\n", typeno);
    csize += emit_uleb128(DW_TAG_base_type);
    csize += emit_string(name);
    csize += emit_uint8(encoding);
    csize += emit_uint8(size);
    return(csize);
}

static int emit_ptrtype(int typeno, int ptrtypeno){
    int csize = 0;
    emit("T%d:\n", typeno);
    csize += emit_uleb128(DW_TAG_pointer_type);
    emit("\t.long T%d\n", ptrtypeno);
    
}

static int emit_arraytype(int type, int arraytype, unsigned int range){
    int csize = 0;
    emit("T%d:\n", type);
    csize += emit_uleb128(DW_TAG_array_type);
    emit("\t.long T%d\n", arraytype);
    csize += emit_uleb128(DW_TAG_subrange_type);
    emit_uleb128(range);
}

static int emit_subroutine_type(int type, int rettype){
    int csize = 0;
    emit("T%d:\n", type);
    csize += emit_uleb128(DW_TAG_subroutine_type);
    emit("\t.long T%d\n", rettype);
    
}


static void emit_compile(char* name){
    emit_uleb128(DW_TAG_compile_unit);
    emit_string(name);
}

static int emit_member(char* name, int type, int ofs)
{  int csize = 0;
   emit_uleb128(DW_TAG_member);
   csize += emit_string(name);
   emit("\t.long T%d\n", type);
   csize = 1 + get_uleb128_size(ofs);
   emit_uint8(csize);
   emit_uint8(DW_OP_plus_uconst);
   emit_uleb128(ofs);
   return(csize + 1);
}


static int emit_structtype(int typeno, char* name, int size)
{  int csize = 0;
   emit("T%d:\n", typeno);
   csize += emit_uleb128(DW_TAG_structure_type);
   csize += emit_string(name);
   csize += emit_uint8(size);
}

static int emit_uniontype(int typeno, char* name, int size)
{  int csize = 0;
   emit("T%d:\n", typeno);
   csize += emit_uleb128(DW_TAG_union_type);
   csize += emit_string(name);
   csize += emit_uint8(size);
}

static int emit_enum()
{  int csize = 0;
   csize += emit_uleb128(DW_TAG_enumeration_type);
}

static int emit_enumval(char* name, int num)
{  int csize = 0;
   csize += emit_uleb128(DW_TAG_enumeration_type);
}

static int emit_subprogram(char* name, int rettype){
    int csize = 0;
    int i;
    csize += emit_uleb128(DW_TAG_subprogram);
    csize += emit_string(name);
    emit("\t.long T%d\n", rettype);
}


static int emit_varlocreg(int regno)
{  int csize = 0;
   emit_uint8(1);
   emit_uint8(DW_OP_reg0+regno);
   return(2);
}

static int emit_varlocofs(int ofs)
{  int csize = 0;
   
   csize = 1 + get_sleb128_size(ofs);
   emit_uint8(csize);
   emit_uint8(DW_OP_breg8);
   emit_sleb128(ofs);
   return(csize + 1);
}


static int emit_formal(char* name, int type, int regno, int fpofs){
    int csize = 0;
    int i;
    csize += emit_uleb128(DW_TAG_formal_parameter);
    csize += emit_string(name);
    emit("\t.long T%d\n", type);
    if(regno) emit_varlocreg(regno);
        else emit_varlocofs(fpofs);
}

static int emit_local(char* name, int type, int regno, int fpofs){
    int csize = 0;
    int i;
    csize += emit_uleb128(DW_TAG_variable);
    csize += emit_string(name);
    emit("\t.long T%d\n", type);
    emit_uint8(0);
    if(regno) emit_varlocreg(regno);
        else emit_varlocofs(fpofs);
}

static void emit_var_abrev(){
    emit_uleb128(DW_TAG_variable);
    emit_uleb128(DW_TAG_variable);
    emit_uint8(0);
    emit_uleb128(DW_AT_name);
    emit_uleb128(DW_FORM_string);
    emit_uleb128(DW_AT_type);
    emit_uleb128(DW_FORM_ref_addr);
    emit_uleb128(DW_AT_external);
    emit_uleb128(DW_FORM_flag);
    emit_uleb128(DW_AT_location);
    emit_uleb128(DW_FORM_block1);
    emit_uleb128(0);
    emit_uleb128(0);
}

static int emit_globalvar(char* name, int type){
    int csize = 0;
    int i;
    csize += emit_uleb128(DW_TAG_variable);
    csize += emit_string(name);
    emit("\t.long T%d\n", type);
    emit_uint8(1);
    emit_uint8(5);
    emit_uint8(DW_OP_addr);
    emit("\t.long %s\n", name);
}

static int emit_typedef_abbrev(){
    emit_uleb128(DW_TAG_typedef);
    emit_uleb128(DW_TAG_typedef);
    emit_uint8(0);
    emit_uleb128(DW_AT_name);
    emit_uleb128(DW_FORM_string);
    emit_uleb128(DW_AT_type);
    emit_uleb128(DW_FORM_ref_addr);
    emit_uleb128(0);
    emit_uleb128(0);
}

static int emit_typedef(char* name, int type){
    int csize = 0;
    int i;
    csize += emit_uleb128(DW_TAG_typedef);
    csize += emit_string(name);
    emit("\t.long T%d\n", type);
   
}

static void emit_abrevtable(){
    emit("\t.section\t.debug_abbrev\n");
    emit("abbrev_start:\n");
    emit_compile_abrev();
    emit_basetype_abrev();
    emit_ptrtype_abrev();
    emit_arraytype_abrev();
    emit_subprogram_abrev();
    emit_compound_abrev();
    emit_enumtype_abrev();
    emit_var_abrev();
    emit_typedef_abbrev();
    emit_uleb128(0);
    emit_uleb128(0);
}

static void emit_infoheader() {
     (*IR->segment)(DBGBEG);
    emit("\t.long info_end-info_start\n");
    emit("info_start:\n");
    emit("\t.short 2\n");
    emit("\t.long abbrev_start\n");
    emit_uint8(4);
    (*IR->segment)(DBGEND);
}

/* dbxout - output .stabs entry for type ty */
static void dbxout(Type ty) {
	ty = unqual(ty);
	if (!ty->x.printed) {
        emittype(ty);
	}
}

/* dbxtype - emit a stabs entry for type ty, return type code */
static int dbxtype(Type ty) {	
	dbxout(ty);
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
	//print("%d", tc);
	if (ty->x.printed)
		return;
   
	ty->x.printed = 1;
	switch (ty->op) {
	case VOID:	/* void is defined as itself */
         (*IR->segment)(DBGBEG);
        emit_basetype(tc, ty->u.sym->name, DW_ATE_void , 4);
		break;
	case INT:		
		 (*IR->segment)(DBGBEG);
        emit_basetype(tc, ty->u.sym->name, DW_ATE_signed, ty->size);
        break;
	case UNSIGNED:		
         (*IR->segment)(DBGBEG);
        emit_basetype(tc, ty->u.sym->name, DW_ATE_unsigned, ty->size);
		break;
	case FLOAT:	/* float, double, long double get sizes, not ranges */
         (*IR->segment)(DBGBEG);
        emit_basetype(tc, ty->u.sym->name, DW_ATE_float, ty->size);
		break;
	case POINTER:
		emittype(ty->type);
         (*IR->segment)(DBGBEG);
        emit_ptrtype(tc, ty->type->x.typeno);
		break;
	case FUNCTION:
		emittype(ty->type);
         (*IR->segment)(DBGBEG);
        emit_subroutine_type(tc, ty->type->x.typeno);
		break;
	case ARRAY:	/* array includes subscript as an int range */
        emittype(ty->type);
         (*IR->segment)(DBGBEG);        
		emit_arraytype(tc, ty->type->x.typeno, ty->size/ty->type->size);
        break;
	case STRUCT: case UNION: {        
        Field p;        
        for (p = fieldlist(ty); p; p = p->link) emittype(p->type);
         (*IR->segment)(DBGBEG);
        if(ty->op == STRUCT) emit_structtype(tc, ty->u.sym->name, ty->size);
        else emit_uniontype(tc, ty->u.sym->name, ty->size);
        for (p = fieldlist(ty); p; p = p->link) {
		emit_member(p->name, p->type->x.typeno, 8*p->offset);
        }
        break;
       }
	case ENUM: {
		Symbol *p;		
		for (p = ty->u.sym->u.idlist; *p; p++) {
            emit_enumval((*p)->name, (*p)->u.value);
		}
		break;
		}
	}
    (*IR->segment)(DBGEND);
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
		print("%s%d:\n", stabprefix, lab);
	}
}

/* stabinit - initialize stab output */
void stabinit(char *file, int argc, char *argv[]) {
	typedef void (*Closure)(Symbol, void *);
	extern char *getcwd(char *, size_t);
	
	if (0 && file && *file) {
		print("\t.stabs \"lcc4_compiled.\",0x%x,0,0,0\n", N_OPT);
		char buf[1024], *cwd = getcwd(buf, sizeof buf);
		if (cwd)
			print("\t.stabs \"%s/\",0x%x,0,3,%stext0\n", cwd, N_SO, stabprefix);
		print("\t.stabs \"%s\",0x%x,0,3,%stext0\n", file, N_SO, stabprefix);
		(*IR->segment)(CODE);
		print("%stext0:\n", stabprefix, N_SO);
		currentfile = file;
	}
    nsyms = 0;
    emit_abrevtable();
    emit_infoheader();
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
	
 
    if (file) emit_compile(file);  
    (*IR->segment)(CODE);
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

static void emit_symlabel() {
    emit("E%d:\n", nsyms++);
}


/* stabsym - output a stab entry for symbol p */
void stabsym(Symbol p) {
	int code, tc, sz = p->type->size;   
	if (p->generated || p->computed)
		return;
    
	if (isfunc(p->type) && p->sclass != STATIC) {
        tc = dbxtype(freturn(p->type));
        (*IR->segment)(DBGBEG);
        emit_symlabel();
        emit_subprogram(p->name, tc);
		(*IR->segment)(DBGEND);
        return;
	}
	if (!IR->wants_argb && p->scope == PARAM && p->structarg) {
		assert(isptr(p->type) && isstruct(p->type->type));
		tc = dbxtype(p->type->type);
		sz = p->type->type->size;
	} else
		tc = dbxtype(p->type);
    (*IR->segment)(DBGBEG);
	if (p->sclass == AUTO && p->scope == GLOBAL || p->sclass == EXTERN) {
		emit_symlabel();
        emit_globalvar(p->name, tc);
	} else if (p->sclass == STATIC) {
		print("\t.stabs \"%s:%c%d\",%d,0,0,%s\n", p->name, p->scope == GLOBAL ? 'S' : 'V',
			tc, p->u.seg == BSS ? N_LCSYM : N_STSYM, p->x.name);
		(*IR->segment)(DBGEND);
        return;
	} else if (p->sclass == REGISTER) {
		if (p->x.regnode) {
			int r = p->x.regnode->number;
			if (p->x.regnode->set == FREG)
				r += 32;	/* floating point */
            if(p->scope == PARAM) {
                emit_symlabel();
                emit_formal(p->name, tc, r, 0);
            } else {
                emit_symlabel();
                emit_local(p->name, tc, r, 0);
            }
		}
        (*IR->segment)(DBGEND);
		return;
	} else if (p->scope == PARAM) {
        emit_symlabel();
        emit_formal(p->name, tc, 0, framesize + p->x.offset);
	} 
  emit_symlabel();
  emit_local(p->name, tc, 0, framesize + p->x.offset);
  (*IR->segment)(DBGEND);
}

/* stabtype - output a stab entry for type *p */
void stabtype(Symbol p) {
    int tc;
    if (p->type) {
		if (p->sclass == 0) {
			dbxtype(p->type);
        } else if (p->sclass == TYPEDEF) {
	    tc = dbxtype(p->type);
       (*IR->segment)(DBGBEG);
        emit_typedef(p->name, tc);
        (*IR->segment)(DBGEND);
    }
    }
}

/* stabend - finalize a function */
void stabfend(Symbol p, int lineno) {
    if (isfunc(p->type) && p->sclass != STATIC) {
        print(".globl %s_end\n",p->x.name);
		print("%s_end:\n",p->x.name);
        return;
    } 
}

/* stabend - finalize stab output */
void stabend(Coordinate *cp, Symbol p, Coordinate **cpp, Symbol *sp, Symbol *stab) {
	(*IR->segment)(DBGBEG);
    emit("info_end:\n");
    (*IR->segment)(DBGEND);
}
