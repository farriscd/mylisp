#include <stdio.h>
#include <stdlib.h>
#include "mpc.h"

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

/* LISP Value type declaration */
typedef struct {
  int type;
  long num;
  int err;  
} lval;

/* lval types */
enum { LVAL_NUM, LVAL_ERR };
/* lval error types */
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

/* lval number type constructor */
lval lval_num(long x) {
  lval v;
  v.type = LVAL_NUM;
  v.num = x;
  return v;
}

/* lval error type constructor */
lval lval_err(int x) {
  lval v;
  v.type = LVAL_ERR;
  v.err = x;
  return v;
}

/* Functions to print lval */
void lval_print(lval v) {
  switch (v.type) {
    case LVAL_NUM: printf("%li", v.num); break;
    case LVAL_ERR:
      if (v.err == LERR_DIV_ZERO) {
	printf("Error: Division by Zero");
      }
      if (v.err == LERR_BAD_OP) {
	printf("Error: Invalid Operator");
      }
      if (v.err == LERR_BAD_NUM) {
	printf("Error: Invalid Number");
      }
      break;
  }
}
void lval_println(lval v) {
  lval_print(v);
  putchar('\n');
}

/* Operator evaluation function */
lval eval_op(lval x, char* op, lval y) {
  /* Error checking */
  if (x.type == LVAL_ERR) { return x; }
  if (y.type == LVAL_ERR) { return y; }  
  
  /* Math operations */
  if (strcmp(op, "+") == 0) { return lval_num(x.num + y.num); }
  if (strcmp(op, "-") == 0) { return lval_num(x.num - y.num); }
  if (strcmp(op, "*") == 0) { return lval_num(x.num * y.num); }
  if (strcmp(op, "/") == 0) { return y.num == 0 ? lval_err(LERR_DIV_ZERO) : lval_num(x.num / y.num); }
  if (strcmp(op, "%") == 0) { return lval_num(x.num % y.num); }
  if (strcmp(op, "^") == 0) { return lval_num(pow(x.num, y.num)); }
  if (strcmp(op, "min") == 0) { return x.num > y.num ? lval_num(y.num) : lval_num(x.num); }
  if (strcmp(op, "max") == 0) { return x.num > y.num ? lval_num(x.num) : lval_num(y.num); }
  
  /* Return error if unlisted operator */
  return lval_err(LERR_BAD_OP);
}

/* Evaluates parsed input */
lval eval(mpc_ast_t* t) {
  if (strstr(t->tag, "number")) {
    /* Converts to long type and checks for conversaion errors */
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
  }

  /* Checks for and returns a lone number */
  if (strstr(t->children[1]->tag, "number")) {
    errno = 0;
    long x = strtol(t->children[1]->contents, NULL, 10);
    return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
  }
  
  /* Reads in operator symbol */
  char* op = t->children[1]->contents;
  
  /* Read operand contents into lval */
  lval x = eval(t->children[2]);

  /* Recursive evaluation loop */
  int i = 3;
  while (strstr(t->children[i]->tag, "expr")) {
    x = eval_op(x, op, eval(t->children[i]));
    i++;
  }

  /* Check for negation operator based on the the assumption the loop
   * was not entered */
  if ((strcmp(op, "-") == 0) && i == 3) {
    return lval_num(-x.num);
  }

  return x;
}

int main(int argc, char** argv) {

  /* Parser and grammar definitions */
  mpc_parser_t* Number = mpc_new("number");
  mpc_parser_t* Operator = mpc_new("operator");
  mpc_parser_t* Expr = mpc_new("expr");
  mpc_parser_t* MyLisp =  mpc_new("mylisp");

  mpca_lang(MPCA_LANG_DEFAULT,
    "                                                     \
      number   : /-?[0-9]+/ ;                             \
      operator : '+' | '-' | '*' | '/' | '%' | '^' |      \
                 /min/ | /max/ ;                          \
      expr     : <number> | '(' <operator> <expr>+ ')' ;  \
      mylisp    : /^/ <operator> <expr>+ /$/ |            \
                  /^/ <number> /$/ ;                      \
    ",
    Number, Operator, Expr, MyLisp);

  puts("MyLisp Version 0.1a");
  puts("Press Ctrl+c to Exit\n");

  while (1) {
    char* input = readline("MyLisp> ");
    add_history(input);

    mpc_result_t r;
    if (mpc_parse("<stdin>", input, MyLisp, &r)) {
      mpc_ast_print(r.output);
      lval result = eval(r.output);
      lval_println(result);
      mpc_ast_delete(r.output);
    } else {
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }

    free(input);
  }
  mpc_cleanup(4, Number, Operator, Expr, MyLisp);

  return 0;
}
