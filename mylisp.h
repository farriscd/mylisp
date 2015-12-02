#include "mpc.h"

struct lval;
struct lenv;
typedef struct lval lval;
typedef struct lenv lenv;
typedef lval* (*lbuiltin) (lenv*, lval*);

/* LISP Value ENUM Types */
enum { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_FUN, LVAL_SEXPR, LVAL_QEXPR };

/* ENUM Error Types */
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

/* LISP Value Type */

struct lval {
  int type;
  long num;
  char* err;
  char* sym;
  lbuiltin fun;
  int count;
  struct lval** cell;
};

/* LISP Value Functions */

lval* lval_num(long n);
lval* lval_err(char* e);
lval* lval_sym(char* s);
lval* lval_fun(lbuiltin f);
lval* lval_sexpr(void);
lval* lval_qexpr(void);

lval* lval_copy(lval* v);
void lval_del(lval* v);

lval* lval_add(lval* v, lval* k);
lval* lval_pop(lval* v, int i);
lval* lval_take(lval* v, int i);
lval* lval_join(lval* v, lval* k);

/* LISP Environment Type */

struct lenv {
  int count;
  char** syms;
  lval** vals;
};

/* LISP Envinronment Functions */

lenv* lenv_new(void);
lval* lenv_get(lenv* e, lval* v);
void lenv_put(lenv* e, lval* k, lval* v);
void lenv_del(lenv* e);

void lenv_add_builtin(lenv* e, char* name, lbuiltin func);
void lenv_add_builtins(lenv* e);

/* Read & Eval */
lval* lval_read(mpc_ast_t* t);
lval* lval_read_num(mpc_ast_t* t);

lval* lval_eval(lenv* e, lval* v);
lval* lval_eval_sexpr(lenv* e, lval* v);

/* Builtin Operations */
lval* builtin(lenv* e, lval* v, char* func);

lval* builtin_op(lenv* e, lval* a, char* op);
lval* builtin_add(lenv* e, lval* a);
lval* builtin_sub(lenv* e, lval* a);
lval* builtin_mul(lenv* e, lval* v);
lval* builtin_div(lenv* e, lval* v);
lval* builtin_min(lenv* e, lval* v);
lval* builtin_max(lenv* e, lval* v);
lval* builtin_mod(lenv* e, lval* v);
lval* builtin_exp(lenv* e, lval* v);

lval* builtin_len(lenv* e, lval* v);
lval* builtin_head(lenv* e, lval* v);
lval* builtin_init(lenv* e, lval* v);
lval* builtin_tail(lenv* e, lval* v);
lval* builtin_list(lenv* e, lval* v);
lval* builtin_eval(lenv* e, lval* v);
lval* builtin_join(lenv* e, lval* v);

/* Print functions */
void lval_print(lval* v);
void lval_expr_print(lval* v, char open, char close);
void lval_println(lval* v);
