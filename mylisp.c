#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include "mpc.h"
#include "mylisp.h"

#define LASSERT(args, cond, err) \
  if (!(cond)) { lval_del(args); return lval_err(err); }

#ifdef _WIN32
#include <string.h>

/* Optional case of no editline package */
static char buffer[2048];

char* readline(char* prompt) {
  fputs(prompt, stdout);
  fgets(buffer, 2048, stdin);
  char* cpy = malloc(strlen(buffer)+1);
  strpcy(cpy,buffer);
  cpy[strlen(cpy)-1] = '\0';
  return cpy;
}

void add_history(char* unused) {}

#else
#include <editline/readline.h>
#include <editline/history.h>
#endif

/**/
/* LISP Value constructors and functions */
/**/

/* Constructors */
lval* lval_num(long n) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_NUM;
  v->num = n;
  return v;
}

lval* lval_err(char* e) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_ERR;
  v->err = malloc(strlen(e) + 1);
  strcpy(v->err, e);
  return v;
}

lval* lval_sym(char* s) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_SYM;
  v->sym = malloc(strlen(s) + 1);
  strcpy(v->sym, s);
  return v;
}

lval* lval_fun(lbuiltin f) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_FUN;
  v->fun = f;
  return v;
}

lval* lval_sexpr(void) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_SEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}

lval* lval_qexpr(void) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_QEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}

/* Functions */
lval* lval_copy(lval* v) {
  lval* x = malloc(sizeof(lval));
  x->type = v->type;

  switch (v->type) {
    case LVAL_FUN: x->fun = v->fun; break;
    case LVAL_NUM: x->num = v->num; break;

    case LVAL_ERR:
      x->err = malloc(strlen(v->err) + 1);
      strcpy(x->err, v->err); break;

    case LVAL_SYM:
      x->sym = malloc(strlen(v->sym) + 1);
      strcpy(x->sym, v->sym); break;

    case LVAL_SEXPR:
    case LVAL_QEXPR:
      x->count = v->count;
      x->cell = malloc(sizeof(lval*) * x->count);
      for (int i = 0; i < x->count; i++) {
        x->cell[i] = lval_copy(v->cell[i]);
      }
    break;
  }
  return x;
}

void lval_del(lval* v) {
  switch (v->type) {
    case LVAL_FUN: break;
    case LVAL_NUM: break;

    case LVAL_ERR: free(v->err); break;
    case LVAL_SYM: free(v->sym); break;

    case LVAL_QEXPR:
    case LVAL_SEXPR:
      for (int i = 0; i < v->count; i++) {
        lval_del(v->cell[i]);
      }
      free(v->cell);
      break;
  }
  free(v);
}

/* Stack manipulation functions */
lval* lval_add(lval* v, lval* k) {
  v->count++;
  v->cell = realloc(v->cell, sizeof(lval*) * v->count);
  v->cell[v->count-1] = k;
  return v;
}

lval* lval_pop(lval* v, int i) {
  lval* x = v->cell[i];
  memmove(&v->cell[i], &v->cell[i+1], sizeof(lval*) * (v->count-i-1));
  v->count--;
  v->cell = realloc(v->cell, sizeof(lval*) * v->count);
  return x;
}

lval* lval_take(lval* v, int i) {
  lval* x = lval_pop(v, i);
  lval_del(v);
  return x;
}

lval* lval_join(lval* v, lval* k) {
  while (k->count) {
    v = lval_add(v, lval_pop(k, 0));
  }
  lval_del(k);
  return v;
}

/**/
/* LISP Environment Constructors & Functions */
/**/

lenv* lenv_new(void) {
  lenv* e = malloc(sizeof(lenv));
  e->count = 0;
  e->syms = NULL;
  e->vals = NULL;
  return e;
}

lval* lenv_get(lenv* e, lval* k) {
  for (int i = 0; i < e->count; i++) {
    if (strcmp(e->syms[i], k->sym) == 0) {
      return lval_copy(e->vals[i]);
    }
  }
  return lval_err("Unbound symbol");
}

void lenv_put(lenv* e, lval* k, lval* v) {
  for (int i = 0; i < e->count; i++) {
    if (strcmp(e->syms[i], k->sym) == 0) {
      lval_del(e->vals[i]);
      e->vals[i] = lval_copy(v);
      return;
    }
  }
  e->count++;
  e->vals = realloc(e->vals, sizeof(lval*) * e->count);
  e->syms = realloc(e->syms, sizeof(char*) * e->count);

  e->vals[e->count-1] = lval_copy(v);
  e->syms[e->count-1] = malloc(strlen(k->sym)+1);
  strcpy(e->syms[e->count-1], k->sym);
}

void lenv_del(lenv* e) {
  for (int i = 0; i < e->count; i++) {
    free(e->syms[i]);
    lval_del(e->vals[i]);
  }
  free(e->syms);
  free(e->vals);
  free(e);
}

void lenv_add_builtin(lenv* e, char* name, lbuiltin func) {
  lval* k = lval_sym(name);
  lval* v = lval_fun(func);
  lenv_put(e, k, v);
  lval_del(k);
  lval_del(v);
}

/* Default built-in environment functions */
void lenv_add_builtins(lenv* e) {
  lenv_add_builtin(e, "def", builtin_def);

  lenv_add_builtin(e, "list", builtin_list);
  lenv_add_builtin(e, "len", builtin_len);
  lenv_add_builtin(e, "head", builtin_head);
  lenv_add_builtin(e, "init", builtin_init);
  lenv_add_builtin(e, "tail", builtin_tail);
  lenv_add_builtin(e, "eval", builtin_eval);
  lenv_add_builtin(e, "join", builtin_join);

  lenv_add_builtin(e, "+", builtin_add);
  lenv_add_builtin(e, "-", builtin_sub);
  lenv_add_builtin(e, "*", builtin_mul);
  lenv_add_builtin(e, "/", builtin_div);
  lenv_add_builtin(e, "min", builtin_min);
  lenv_add_builtin(e, "max", builtin_max);
  lenv_add_builtin(e, "%", builtin_mod);
  lenv_add_builtin(e, "^", builtin_exp);
}


/**/
/* LISP Read & Evaluation Functions */
/**/

/* Read functions */
lval* lval_read(mpc_ast_t* t) {
  if (strstr(t->tag, "number")) { return lval_read_num(t); }
  if (strstr(t->tag, "symbol")) { return lval_sym(t->contents); }

  lval* x = NULL;
  if (strcmp(t->tag, ">") == 0) { x = lval_sexpr(); }
  if (strstr(t->tag, "sexpr")) { x = lval_sexpr(); }
  if (strstr(t->tag, "qexpr")) { x = lval_qexpr(); }

  for (int i = 0; i < t->children_num; i++) {
    if (strcmp(t->children[i]->contents, "(") == 0) { continue; }
    if (strcmp(t->children[i]->contents, ")") == 0) { continue; }
    if (strcmp(t->children[i]->contents, "}") == 0) { continue; }
    if (strcmp(t->children[i]->contents, "{") == 0) { continue; }
    if (strcmp(t->children[i]->tag, "regex") == 0) { continue; }
    x = lval_add(x, lval_read(t->children[i]));
  }
  return x;
}

lval* lval_read_num(mpc_ast_t* t) {
  errno = 0;
  int x = strtol(t->contents, NULL, 10);
  return errno != ERANGE ?
    lval_num(x) : lval_err("Invalied Number");
}

/* Evaluation function */
lval* lval_eval(lenv* e, lval* v) {
  if (v->type == LVAL_SYM) {
    lval* x = lenv_get(e, v);
    lval_del(v);
    return x;
  }
  if (v->type == LVAL_SEXPR) { return lval_eval_sexpr(e, v); }
  return v;
}

/* S-Expression evaluation function */
lval* lval_eval_sexpr(lenv* e, lval* v) {
  for (int i = 0; i < v->count; i++) {
    v->cell[i] = lval_eval(e, v->cell[i]);
  }

  for (int i = 0; i < v->count; i++) {
    if (v->cell[i]->type == LVAL_ERR) { return lval_take(v, i); }
  }

  if (v->count == 0) { return v; }
  if (v->count == 1) { return lval_take(v, 0); }

  lval* f = lval_pop(v, 0);
  if (f->type != LVAL_FUN) {
    lval_del(f);
    lval_del(v);
    return lval_err("Invalid Symbol");
  }

  lval* result = f->fun(e, v);
  lval_del(f);

  return result;
}

/**/
/* Built-in Functions */
/**/

/* Built-in function lookup */
lval* builtin(lenv* e, lval* v, char* func) {
  if (strcmp("list", func) == 0) { return builtin_list(e, v); }
  if (strcmp("head", func) == 0) { return builtin_head(e, v); }
  if (strcmp("tail", func) == 0) { return builtin_tail(e, v); }
  if (strcmp("join", func) == 0) { return builtin_join(e, v); }
  if (strcmp("eval", func) == 0) { return builtin_eval(e, v); }
  if (strcmp("init", func) == 0) { return builtin_init(e, v); }
  if (strcmp("len", func) == 0) { return builtin_len(e, v); }
  if (strstr("+-/*%^minmax", func)) { return builtin_op(e, v, func); }

  lval_del(v);
  return lval_err("Unknown Function");
}

/* Built-in define function */
lval* builtin_def(lenv* e, lval* v) {
  LASSERT(v, v->cell[0]->type == LVAL_QEXPR,
    "Function 'def' passed invalid type");

  lval* syms = v->cell[0];

  for (int i = 0; i < syms->count; i++) {
    LASSERT(v, syms->cell[i]->type == LVAL_SYM,
      "Function 'def' cannot define non-symbol");
  }
  LASSERT(v, syms->count == v->count-1,
    "Function 'def' passed invalid number of arguments");

  for (int i = 0; i < syms->count; i++) {
    lenv_put(e, syms->cell[i], v->cell[i+1]);
  }

  lval_del(v);
  return lval_sexpr();
}

/* Built-in operations */
lval* builtin_op(lenv* e, lval* v, char* op) {
  for (int i = 0; i < v->count; i++) {
    LASSERT(v, v->cell[i]->type == LVAL_NUM,
      "Invalid Number");
  }
  lval* x = lval_pop(v, 0);
  if ((strcmp(op, "-") == 0) && v->count == 0) {
    x->num = -x->num;
  }
  while (v->count > 0) {
    lval* y = lval_pop(v, 0);
    if (strcmp(op, "+") == 0) { x->num += y->num; }
    if (strcmp(op, "-") == 0) { x->num -= y->num; }
    if (strcmp(op, "*") == 0) { x->num *= y->num; }
    if (strcmp(op, "/") == 0) {
      if (y->num == 0) {
        lval_del(x);
        lval_del(y);
        x = lval_err("Division by zero");
        break;
      } else {
          x-> num /= y->num;
        }
      }
    if (strcmp(op, "%") == 0) { x->num %= y->num; }
    if (strcmp(op, "^") == 0) { x->num = pow(x->num, y->num); }
    if (strcmp(op, "min") == 0) { x->num = x->num > y->num ? y->num : x->num; }
    if (strcmp(op, "max") == 0) { x->num = x->num > y->num ? x->num : y->num; }

    lval_del(y);
  }
  lval_del(v);
  return x;
}

/* Arithmetic Operations */
lval* builtin_add(lenv* e, lval* v) {
  return builtin_op(e, v, "+");
}

lval* builtin_sub(lenv* e, lval* v) {
  return builtin_op(e, v, "-");
}

lval* builtin_mul(lenv* e, lval* v) {
  return builtin_op(e, v, "*");
}

lval* builtin_div(lenv* e, lval* v) {
  return builtin_op(e, v, "/");
}

lval* builtin_min(lenv* e, lval* v) {
  return builtin_op(e, v, "min");
}

lval* builtin_max(lenv* e, lval* v) {
  return builtin_op(e, v, "max");
}

lval* builtin_mod(lenv* e, lval* v) {
  return builtin_op(e, v, "%");
}

lval* builtin_exp(lenv* e, lval* v) {
  return builtin_op(e, v, "^");
}

/* List Operations */
lval* builtin_list(lenv* e, lval* v) {
  v->type = LVAL_QEXPR;
  return v;
}

lval* builtin_len(lenv* e, lval* v) {
  LASSERT(v, v->count == 1,
    "Function 'len' passed too many arguments");
  LASSERT(v, v->cell[0]->type == LVAL_QEXPR,
    "Function 'len' passed invalid type");
  LASSERT(v, v->cell[0]->count != 0,
    "Invalid syntax");

  lval* x = lval_take(v, 0);
  return lval_num(x->count);
}

lval* builtin_head(lenv* e, lval* v) {
  LASSERT(v, v->count == 1,
    "Function 'head' passed too many arguments");
  LASSERT(v, v->cell[0]->type == LVAL_QEXPR,
    "Function 'head' passed invalid type");
  LASSERT(v, v->cell[0]->count != 0,
    "Invalid syntax");

  lval* x = lval_take(v, 0);
  while (x->count > 1) { lval_del(lval_pop(x, 1)); }
  return x;
}

lval* builtin_init(lenv* e, lval* v) {
  LASSERT(v, v->count == 1,
    "Function 'init' passed too many arguments");
  LASSERT(v, v->cell[0]->type == LVAL_QEXPR,
    "Function 'init' passed invalid type");
  LASSERT(v, v->cell[0]->count != 0,
    "Invalid syntax");

  lval* x = lval_take(v, 0);
  lval_del(lval_pop(x, x->count-1));
  return x;
}

lval* builtin_tail(lenv* e, lval* v) {
  LASSERT(v, v->count == 1,
    "Function 'tail' passed too many arguments");
  LASSERT(v, v->cell[0]->type == LVAL_QEXPR,
    "Function 'tail' passed invalid type");
  LASSERT(v, v->cell[0]->count != 0,
    "Invalid syntax");

  lval* x = lval_take(v, 0);
  lval_del(lval_pop(x, 0));
  return x;
}

lval* builtin_eval(lenv* e, lval* v) {
  LASSERT(v, v->count == 1,
    "Function 'eval' passed too many arguments");
  LASSERT(v, v->cell[0]->type == LVAL_QEXPR,
    "Function 'eval' passed invalid type");

  lval* x = lval_take(v, 0);
  x->type = LVAL_SEXPR;
  return lval_eval(e, x);
}

lval* builtin_join(lenv* e, lval* v) {
  for (int i = 0; i < v->count; i++) {
    LASSERT(v, v->cell[i]->type == LVAL_QEXPR,
      "Function 'join' passed invalid type");
  }

  lval* x = lval_pop(v, 0);
  while (v->count) { x = lval_join(x, lval_pop(v, 0)); }

  lval_del(v);
  return x;
}

/**/
/* Printing Functions */
/**/

void lval_print(lval* v) {
  switch (v->type) {
    case LVAL_NUM: printf("%li", v->num); break;
    case LVAL_ERR: printf("Error: %s", v->err); break;
    case LVAL_SYM: printf("%s", v->sym); break;
    case LVAL_SEXPR: lval_expr_print(v, '(', ')'); break;
    case LVAL_QEXPR: lval_expr_print(v, '{', '}'); break;
    case LVAL_FUN: printf("<function>"); break;
  }
}

void lval_expr_print(lval* v, char open, char close) {
  putchar(open);
  for (int i = 0; i < v->count; i++) {
    lval_print(v->cell[i]);
    if (i != (v->count-1)) {
      putchar(' ');
    }
  }
  putchar(close);
}

void lval_println(lval* v) {
  lval_print(v);
  putchar('\n');
}

/**/
/* Main */
/**/

int main(int argc, char** argv) {

  /* Parser and grammar definitions */
  mpc_parser_t* Number = mpc_new("number");
  mpc_parser_t* Symbol = mpc_new("symbol");
  mpc_parser_t* Sexpr = mpc_new("sexpr");
  mpc_parser_t* Qexpr = mpc_new("qexpr");
  mpc_parser_t* Expr = mpc_new("expr");
  mpc_parser_t* MyLisp =  mpc_new("mylisp");

  mpca_lang(MPCA_LANG_DEFAULT,
    "                                                   \
      number : /-?[0-9]+/ ;                             \
      symbol : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!%^&]+/ ;     \
      sexpr  : '(' <expr>* ')' ;                        \
      qexpr  : '{' <expr>* '}' ;                        \
      expr   : <number> | <symbol> | <sexpr> | <qexpr> ;\
      mylisp : /^/ <expr>* /$/ | ;                      \
    ",
    Number, Symbol, Sexpr, Qexpr, Expr, MyLisp);

  puts("MyLisp Version 0.0.5");
  puts("Press Ctrl+c to Exit\n");

  lenv* e = lenv_new();
  lenv_add_builtins(e);

  while (1) {
    char* input = readline("MyLisp> ");
    add_history(input);

    mpc_result_t r;
    if (mpc_parse("<stdin>", input, MyLisp, &r)) {
      lval* x = lval_eval(e, lval_read(r.output));
      // mpc_ast_print(r.output);
      lval_println(x);
      lval_del(x);
      mpc_ast_delete(r.output);
    } else {
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }

    free(input);
  }
  lenv_del(e);
  mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, MyLisp);

  return 0;
}
