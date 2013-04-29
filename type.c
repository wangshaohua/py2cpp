#include "symtab.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

char* newTemp() {
    static int i = 0;
    char * tmp = (char* ) malloc( sizeof(char) * 10);
    sprintf(tmp, "_t%d", i++); return tmp; }
char* newIterator() {
    static int i = 0;
    char* tmp = (char*) malloc( sizeof(char) * 4);
    sprintf(tmp, "i%d", i ++ );
    return tmp;
}

int level = 1;
void indent_output() {
    int i ;
    for(i= 0 ; i < level; i ++ ) {
        printf("\t");
    }
}

struct type t_unknown  = {UNKNOWN_KIND, 0};
struct type t_char = {CHAR_KIND, 1, "char"};
struct type t_boolean = {BOOLEAN_KIND, 1, "bool"};
struct type t_integer = {INTEGER_KIND, 4, "int"};
struct type t_float = {FLOAT_KIND, 4, "float"};
struct type t_string = {STRING_KIND, 0,  "string"};

static void assign_type_to_stmt(stmt_ty s);
static void assign_type_to_expr(expr_ty e);
static void assign_type_to_comprehension(comprehension_ty com);
static void push_type_to_expr(expr_ty e);
static int type_compare(type_ty t1, type_ty t2);
static type_ty max_type(type_ty t1, type_ty t2);
static void stmt_for_expr(expr_ty e);
static void eliminate_python_unique_for_expr(expr_ty e);
static void eliminate_python_unique_for_stmt(stmt_ty s);

static type_ty
create_list_type(type_ty t) {
    type_ty tp = (type_ty) malloc ( sizeof(struct type) );
    sprintf(tp->name, "vector< %s >", t->name);
    tp->kind = LIST_KIND;
    tp->base = t;
    return tp;
}


static int
type_compare(type_ty t1, type_ty t2) {
}

static type_ty
max_type(type_ty t1, type_ty t2) {
    if(t1 != &t_integer && t1 != &t_float) return t1;
    if(t2 != &t_integer && t2 != &t_float) return t2;
    if(t1 == t2) return t1;
    if(t1 == &t_float || t2 == &t_float) return &t_float;
}

static void
assign_type_to_stmt(stmt_ty s) {
    int i;
    switch(s->kind) {
        case FuncDef_kind:
            break;
        case ClassDef_kind:
            break;
        case Return_kind:
            break;
        case Delete_kind:
            break;
        case Assign_kind:
            {
                int n_target = s->assign.n_target;
                expr_ty * targets = s->assign.targets;
                expr_ty value = s->assign.value;
                assign_type_to_expr(value);
                strcpy(value->addr, targets[0]->name.id);
                insert_to_current_table(targets[0]->name.id,
                        value->e_type, SE_VARIABLE_KIND);

                eliminate_python_unique_for_expr(value);
                if(value->isplain)
                    stmt_for_expr(value);

                int i ;
                for(i= 1; i < n_target; i ++ ){
                    assign_type_to_expr(targets[i]);
                    insert_to_current_table(targets[i]->name.id,
                            value->e_type, SE_VARIABLE_KIND);

                    indent_output();
                    switch(value->e_type->kind) {
                        case INTEGER_KIND:
                        case FLOAT_KIND:
                        case STRING_KIND:
                        case BOOLEAN_KIND:
                            printf("%s %s = %s;\n", value->e_type->name, targets[i]->addr, value->addr);
                            break;
                        case LIST_KIND:
                            printf("%s & %s = %s;\n", value->e_type->name,
                                    targets[i]->addr, value->addr);
                            break;
                    }
                }
            }
            break;
        case AugAssign_kind:
            break;
        case Print_kind:
            {
                int i ;
                for(i = 0; i < s->print.n_value; i ++ ) {
                    assign_type_to_expr(s->print.values[i]);
                }
                char* dest = "cout";
                if(s->print.dest!= NULL) {
                    eliminate_python_unique_for_expr(s->print.dest);
                    dest = s->print.dest->addr;
                }

                for(i = 0; i < s->print.n_value; i ++ ) {
                    if(s->print.values[i]->isplain)
                        stmt_for_expr(s->print.values[i]);
                    else
                        eliminate_python_unique_for_expr(s->print.values[i]);
                }

                int fcout = 0;
                int end = 0;
                for(i = 0; i < s->print.n_value; i ++ ) {
                    if(s->print.values[i]->e_type->kind == LIST_KIND) {
                        if(end == 0)  printf(";\n");
                        indent_output();
                        printf("%s << \"[\";\n", dest);
                        indent_output();
                        printf("for_each(%s.begin(), %s.end() - 1, output<%s, %s>);\n",
                                s->print.values[i]->addr, s->print.values[i]->addr,
                                s->print.values[i]->e_type->base->name, dest);
                        indent_output();
                        printf("%s << *(%s.end() -1);\n", dest, s->print.values[i]->addr);
                        indent_output();
                        printf("%s << \"] \";\n", dest);
                        fcout = 0;
                        end = 1;
                    }
                    else {
                        if(fcout == 0 ) {
                            indent_output();
                            printf("%s << %s << \" \"", dest, s->print.values[i]->addr);
                            fcout = 1;
                        }else {
                            printf(" << %s << \" \"", s->print.values[i]->addr);
                        }
                    }
                }
                if(fcout == 0) {
                    indent_output();
                    printf("cout");
                }
                if(s->print.newline_mark == 1) {
                    printf(" << endl;\n");
                }else {
                    printf(";\n");
                }

            }
            break;
        case For_kind:
            break;
        case While_kind:
            break;
        case If_kind:
            break;
        case With_kind:
            break;
        case Raise_kind:
            break;
        case Try_kind:
            break;
        case Assert_kind:
            break;
        case Global_kind:
            break;
        case Expr_kind:
            assign_type_to_expr(s->exprstmt.value);
            if(s->exprstmt.value->isplain) {
                stmt_for_expr(s->exprstmt.value);
            }
            else {
                eliminate_python_unique_for_expr(s->exprstmt.value);
            }
            break;
    }
}


static void
assign_type_to_expr(expr_ty e) {
    int i;
    switch(e->kind) {
        case BoolOp_kind:
            break;
        case BinOp_kind:
            assign_type_to_expr(e->binop.left);
            assign_type_to_expr(e->binop.right);
            e->isplain = e->binop.left->isplain & e->binop.right->isplain;

            if(e->binop.op == Mult && (e->binop.left->e_type->kind == STRING_KIND || e->binop.right->e_type->kind == STRING_KIND)) {
                e->isplain = 0;
            }
            if(e->binop.op == Pow) {
                e->isplain = 0;
            }
            if(e->binop.left->e_type->kind == LIST_KIND) {
                e->e_type = e->binop.left->e_type;
            }
            else {
                e->e_type = max_type(e->binop.left->e_type, e->binop.right->e_type);
            }
            break;
        case UnaryOp_kind:
            break;
        case Lambda_kind:
            break;
        case IfExp_kind:
            break;
        case Dict_kind:
            break;
        case Set_kind:
            break;
        case ListComp_kind:
            e->isplain = 0;
            for(i = 0; i < e->listcomp.n_com; i ++ ) {
                assign_type_to_comprehension(e->listcomp.generators[i]);
            }
            assign_type_to_expr(e->listcomp.elt);
            e->e_type = create_list_type(e->listcomp.elt->e_type);
            break;
        case SetComp_kind:
            break;
        case DictComp_kind:
            break;
        case GeneratorExp_kind:
            break;
        case Yield_kind:
            break;
        case Compare_kind:
            assign_type_to_expr(e->compare.left);
            for(i = 0; i< e->compare.n_comparator; i ++ ) {
                assign_type_to_expr(e->compare.comparators[i]);
                switch(e->compare.ops[i]) {
                    case Is:
                    case IsNot:
                    case In:
                    case NotIn:
                        e->isplain = 0;break;
                    default:
                        e->isplain = 1;break;
                }
            }
            e->e_type = &t_boolean;
            if(e->compare.n_comparator != 1) {
                e->isplain = 0;
            }
            break;
        case Call_kind:
            break;
        case Repr_kind:
            break;
        case Num_kind:
            switch(e->num.kind) {
                case INTEGER:
                    e->e_type = &t_integer;
                    break;
                case DECIMAL:
                    e->e_type = &t_float;
                    break;
            }
            e->isplain = 1;
            break;
        case Str_kind:
            e->e_type = &t_string;
            e->isplain = 1;
            break;
        case Attribute_kind:
            break;
        case Subscript_kind:
            break;
        case Name_kind:
            {
                type_ty tp = search_type_for_name(e->name.id);
                if(tp == NULL) {
                    e->e_type = &t_unknown;
                }else {
                    e->e_type = tp;
                }
                strcpy(e->addr, e->name.id);
                e->isplain = 1;
                /*
                if(e->e_type->kind == LIST_KIND ) {
                    e->isplain = 0;
                }else {
                    e->isplain = 1;
                }*/
            }
            break;
        case List_kind:
            assign_type_to_expr(e->list.elts[0]);
            e->e_type = create_list_type(e->list.elts[0]->e_type);
            e->isplain = 0;
            for(i = 0; i < e->list.n_elt; i ++ ) {
                assign_type_to_expr(e->list.elts[i]);
            }
            break;
        case Tuple_kind:
            break;
    }
}

static void
assign_type_to_comprehension(comprehension_ty com) {
    expr_ty iter = com->iter;
    expr_ty target = com->target;

    int i;
    for(i = 0; i < com->n_test; i ++ ) {
        assign_type_to_expr(com->tests[i]);
    }
    assign_type_to_expr(iter);

    if(iter->e_type->kind = LIST_KIND) {
        target->e_type = iter->e_type->base;
    }else if(iter->e_type->kind ==DICT_KIND) {
        target->e_type = iter->e_type->kbase;
    }

    push_type_to_expr(target);
}


static void
push_type_to_expr(expr_ty e) {
    if(e->kind == Tuple_kind) {
        if(e->e_type->kind == TUPLE_KIND) {

        }
    }
    else if(e->kind == Name_kind) {
        insert_to_current_table(e->name.id, e->e_type, SE_TEMP);
        strcpy(e->addr, e->name.id);
    }
}



void
assign_type_to_ast(stmt_seq* ss) {
    int i = 0;
    for(; i < ss->size; i ++ ) {
        assign_type_to_stmt(ss->seqs[i]);
    }
}


char* get_op_literal(operator_ty op) {
    switch(op) {
        case Add: return "+";
        case Sub: return "-";
        case Mult: return "*";
        case Div:  return "/";
        case Mod:  return "\%";
        case LShift: return "<<";
        case RShift: return ">>";
        case BitOr:  return "|";
        case BitXor:return "^";
        case BitAnd:  return "&";
    }
}


char* get_cmp_literal(compop_ty op) {
    switch(op) {
        case Eq: return "==";
        case NotEq: return "!=";
        case Lt: return "<";
        case LtE: return "<=";
        case Gt: return ">";
        case GtE: return ">=";
    }
}

static void
stmt_for_expr(expr_ty e) {
    if(e->isplain == 0) return ;
    switch(e->kind) {
        case BinOp_kind:
        {
            expr_ty l = e->binop.left;
            expr_ty r = e->binop.right;
            char* oper = get_op_literal(e->binop.op);
            stmt_for_expr(e->binop.left);
            stmt_for_expr(e->binop.right);
            if(e->addr[0] != 0) {
                indent_output();
                printf("%s %s = %s %s %s;\n", e->e_type->name, e->addr,
                        l->addr, oper, r->addr);
            }else {
                sprintf(e->addr, "(%s %s %s)", l->addr, oper, r->addr);
            }
        }
            break;
        case Num_kind:
            if(e->addr[0] != 0) {
                indent_output();
                printf("%s %s = ", e->e_type->name, e->addr);
                if(e->num.kind == INTEGER)  {
                    printf("%d;\n", e->num.ivalue);
                }
                else {
                    printf("%f;\n", e->num.fvalue);
                }
            }
            else {
                if(e->num.kind == INTEGER) {
                    sprintf(e->addr, "%d", e->num.ivalue);
                }else {
                    sprintf(e->addr, "%f", e->num.fvalue);
                }
            }

            break;
        case Str_kind:
            if(e->addr[0] != 0) {
                indent_output();
                printf("%s %s = \"%s\";\n", e->e_type->name, e->addr,  e->str.s);
            }else {
                sprintf(e->addr, "\"%s\"", e->str.s);
            }
            break;
        case Compare_kind:
            stmt_for_expr(e->compare.left);
            stmt_for_expr(e->compare.comparators[0]);
            if(e->addr[0] != 0 ) {
                indent_output();
                printf("%s %s = %s %s %s;\n", e->e_type->name, e->addr,
                        e->compare.left->addr,get_cmp_literal(e->compare.ops[0]), e->compare.comparators[0]->addr);
            }else {
                sprintf(e->addr, "%s %s %s",
                        e->compare.left->addr,get_cmp_literal(e->compare.ops[0]), e->compare.comparators[0]->addr);
            }
            break;
    }
}


static void
eliminate_python_unique_for_expr(expr_ty e) {
    if(e->isplain) return ;
    int i;
    switch(e->kind) {
        case BinOp_kind:
        {
            if(e->addr[0] == 0)
                strcpy(e->addr, newTemp());
            expr_ty l = e->binop.left;
            expr_ty r = e->binop.right;
            if(e->binop.left->e_type->kind == LIST_KIND) {
                if(e->binop.op == Add) {
                    indent_output();
                    printf("%s %s;\n", e->e_type->name, e->addr);
                    eliminate_python_unique_for_expr(e->binop.left);

                    char* iter = newIterator();
                    indent_output();
                    printf("for( int %s = 0; %s < %s.size(); %s ++ )\n", iter, iter, l->addr, iter);
                    level ++;
                    indent_output();
                    printf("%s.push_back(%s[%s]);\n\n", e->addr, l->addr, iter);
                    level --;
                    eliminate_python_unique_for_expr(e->binop.right);

                    indent_output();
                    printf("for( int %s = 0; %s < %s.size(); %s ++ )\n", iter, iter, r->addr, iter);
                    level ++;
                    indent_output();
                    printf("%s.push_back(%s[%s]);\n", e->addr, r->addr, iter);
                    level --;
                    free(iter);
                }
                else if(e->binop.op == Mult) {
                    if(r->e_type->kind == LIST_KIND) {
                        expr_ty t = r; r = l; l = t;
                    }
                    if(r->isplain)
                        stmt_for_expr(r);
                    else
                        eliminate_python_unique_for_expr(r);
                    if(l->isplain)
                        stmt_for_expr(l);
                    else
                        eliminate_python_unique_for_expr(l);
                    indent_output();
                    printf("%s %s;\n", e->e_type->name, e->addr);
                    char* iter = newIterator();
                    char* iter1 = newIterator();
                    indent_output();
                    printf("for( int %s = 0; %s < %s; %s ++ )\n", iter, iter, r->addr, iter);
                    level ++;
                    indent_output();
                    printf("for( int %s = 0; %s < %s.size(); %s ++ )\n", iter1, iter1, l->addr, iter1);
                    level ++;
                    indent_output();
                    printf("%s.push_back(%s[%s]);\n\n", e->addr, l->addr, iter1);
                    level -= 2;
                    free(iter);
                    free(iter1);
                }else {
                    fprintf(stderr, "List Error\n");
                    exit(-1);
                }
            }
            else if(e->binop.op == Pow) {
                if(l->isplain) {
                    stmt_for_expr(l);
                }
                else {
                    eliminate_python_unique_for_expr(l);
                }
                if(r->isplain) {
                    stmt_for_expr(r);
                }
                else {
                    eliminate_python_unique_for_expr(r);
                }
                indent_output();
                printf("%s %s = pow(%s, %s);\n", e->e_type->name, e->addr,
                        l->addr, r->addr);
                //e->isplain = 1;
            }else if(e->binop.op == Mult) {
                if(l->e_type->kind == STRING_KIND || r->e_type->kind == STRING_KIND) {
                    if(r->kind == Str_kind) {
                        expr_ty t = l;
                        l = r;
                        r = t;
                    }
                    indent_output();
                    printf("%s %s;\n", "string", e->addr);
                    char * iter = newIterator();
                    if(r->isplain) {
                        stmt_for_expr(r);
                    }
                    else {
                        eliminate_python_unique_for_expr(r);
                    }
                    indent_output();
                    printf("for(int %s = 0; %s < %s; %s ++ ) {\n", iter, iter, r->addr, iter);
                    level ++;
                    indent_output();
                    printf("%s += %s;\n", e->addr, l->addr);
                    level --;
                    free(iter);
                }
            }else {
                eliminate_python_unique_for_expr(l);
                eliminate_python_unique_for_expr(r);
                e->isplain = 1;
            }
        }
            break;
        case List_kind:
            if(e->addr[0] == 0)
                strcpy(e->addr, newTemp());
            indent_output();
            printf("%s %s;\n", e->e_type->name, e->addr);

            int plain = e->list.elts[0]->isplain;
            if(plain)
                stmt_for_expr(e->list.elts[0]);
            else
                eliminate_python_unique_for_expr(e->list.elts[0]);
            indent_output();
            printf("%s.push_back(%s);\n", e->addr, e->list.elts[0]->addr);
            for(i = 1; i < e->list.n_elt;  i ++ ) {
                plain = e->list.elts[i]->isplain;
                if(plain)
                    stmt_for_expr(e->list.elts[i]);
                else
                    eliminate_python_unique_for_expr(e->list.elts[i]);
                indent_output();
                printf("%s.push_back(%s);\n", e->addr, e->list.elts[i]->addr);
            }
            printf("\n");
            break;
        case ListComp_kind:
            {
                indent_output();
                printf("%s %s;\n", e->e_type->name, e->addr);
                for(i = 0; i < e->listcomp.n_com; i ++ )  {
                    eliminate_python_unique_for_expr(e->listcomp.generators[i]->iter);
                    int j;
                    for(j = 0; j < e->listcomp.generators[i]->n_test; j ++ ) {
                        expr_ty t = e->listcomp.generators[i]->tests[j];
                        if(t->isplain)
                            stmt_for_expr(t);
                        else
                            eliminate_python_unique_for_expr(t);
                    }
                }
                int oldlevel = level;
                for(i = 0; i < e->listcomp.n_com; i ++ ) {
                    indent_output();
                    printf("for(%s %s: %s)\n", e->listcomp.elt->e_type->name, e->listcomp.generators[i]->target->addr,
                            e->listcomp.generators[i]->iter->addr);
                    level ++;
                    if(e->listcomp.generators[i]->n_test) {
                        indent_output();
                        printf("if( ");
                        int j;
                        for(j = 0; j < e->listcomp.generators[i]->n_test; j ++ ) {
                            printf("%s", e->listcomp.generators[i]->tests[j]->addr);
                            if(j != e->listcomp.generators[i]->n_test - 1)
                                printf(" && ");
                        }
                        printf(" )\n");
                        level ++;
                    }
                }
                indent_output();
                printf("%s.push_back(%s);\n", e->addr, e->listcomp.elt->addr);
                level = oldlevel;

            }
            break;
        case Compare_kind:
            if(e->compare.left->isplain)
                stmt_for_expr(e->compare.left);
            else
                eliminate_python_unique_for_expr(e->compare.left);
            if(e->addr[0] != 0) {
                for(i = 0; i < e->compare.n_comparator; i ++ ) {
                    if(e->compare.comparators[i]->isplain)
                        stmt_for_expr(e->compare.comparators[i]);
                    else
                        eliminate_python_unique_for_expr(e->compare.comparators[i]);
                }
                indent_output();
                printf("%s %s = ", e->e_type->name, e->addr);
                for(i = 0; i< e->compare.n_comparator; i ++ ) {
                    char* prev = NULL;
                    char* p = NULL;
                    if(i == 0) {
                        prev = e->compare.left->addr;
                    }else {
                        prev = e->compare.comparators[i-1]->addr;
                    }
                    p = e->compare.comparators[i]->addr;
                    switch(e->compare.ops[i]) {
                        case NotIn:
                            printf("find(%s.begin(), %s.end(), %s) == %s.end()", p, p, prev, p);
                            break;
                        case In:
                            printf("find(%s.begin(), %s.end(), %s) != %s.end()", p, p, prev, p);
                            break;
                        default:
                            printf("%s", prev);
                            printf(" %s %s", get_cmp_literal(e->compare.ops[i]), p);
                    }
                    if(i != e->compare.n_comparator -1 ) {
                        printf(" && ");
                    }
                }
                printf(";\n");
            }
            else {
                for(i = 0; i< e->compare.n_comparator; i ++ ) {
                    if(e->compare.comparators[i]->isplain)
                        stmt_for_expr(e->compare.comparators[i]);
                    else
                        eliminate_python_unique_for_expr(e->compare.comparators[i]);
                    if(i == 0) {
                            strcat(e->addr, e->compare.left->addr);
                    }else {
                            strcat(e->addr, e->compare.comparators[i-1]->addr);
                    }
                    sprintf(e->addr + strlen(e->addr), " %s %s", get_cmp_literal(e->compare.ops[i]),
                            e->compare.comparators[i]->addr);
                    if(i != e->compare.n_comparator -1 ) {
                        strcat(e->addr, " && ");
                    }
                }
            }
            break;
    }
}


static void
eliminate_python_unique_for_stmt(stmt_ty s) {

}
