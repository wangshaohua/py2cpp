#ifndef _SYMTAB_H_
#define _SYMTAB_H_

#include "mod.h"

enum symtab_entry_kind { SE_UNKNOWN_KIND, SE_VARIABLE_KIND, SE_CONSTANT_KIND , SE_GLOBAL_KIND, SE_DEFAULT_KIND};
enum symtab_kind {SK_FILE_KIND, SK_FUNCTION_KIND };

enum type_kind { CHAR_KIND, SHORT_KIND, INTEGER_KIND, ARRAY_KIND, STRUCT_KIND, POINTER_KIND, FLOAT_KIND,
    DOUBLE_KIND, FUNCTION_KIND };


typedef struct symtab_entry * symtab_entry_ty;
typedef struct symtab * symtab_ty;


struct symtab_entry {
    int se_offset;
    char se_name[128];
    enum symtab_entry_kind se_kind;    
    type_ty se_type;
};



struct symtab {
    enum symtab_kind st_kind;
    int st_size;   /* the size of table entry */
    int st_capability;  /* the whole slot in array */
    symtab_entry_ty * st_symbols; 
   
    /* pointer to out scope*/
    symtab_ty st_parent;
    
    int n_child;
    int child_capability;
    symtab_ty * st_children;
};




struct  type{
    enum type_kind kind;
    int length;
    
    /*This is for arrary*/
    type_ty base;  /* <- also for pointer */
    int size;

    /*This is for struct */
    symtab_ty fields;

    /*This is for function */
    int n_params;
    int n_default;
    type_ty* params;
    type_ty* defaults;
};


void assign_type_to_ast(stmt_seq* ss);


//symtab_ty create_symtab(enum symtab_kind kind);


//int insert_to_current_table(char *name, type_ty t, enum symtab_entry_kind kind);


#endif