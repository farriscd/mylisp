#include <stdio.h>
#include <stdlib.h>
#include "mpc.h"

#ifdef _WIN32
#include <string.h>

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

long eval_op(long x, char* op, long y) {
  if (strcmp(op, "+") == 0) { return x + y; }
  if (strcmp(op, "-") == 0) { return x - y; }
  if (strcmp(op, "*") == 0) { return x * y; }
  if (strcmp(op, "/") == 0) { return x / y; }
  if (strcmp(op, "%") == 0) { return x % y; }
  if (strcmp(op, "^") == 0) { return pow(x, y); }
  if (strcmp(op, "min") == 0) { return x > y ? y : x; }
  if (strcmp(op, "max") == 0) { return x > y ? x : y; }
  return 0;
}

long eval(mpc_ast_t* t) {
  if (strstr(t->tag, "number")) {
    return atoi(t->contents);
  }

  //returns just the number if that's all there is
  char* op = t->children[1]->contents;
  if (strstr(t->children[1]->tag, "number")) {
    return atoi(t->children[1]->contents);
  }

  long x = eval(t->children[2]);

  int i = 3;
  while (strstr(t->children[i]->tag, "expr")) {
    x = eval_op(x, op, eval(t->children[i]));
    i++;
  }

  // if it didn't entire the while loop, assumes there isn't another
  // argument and checks for the minus operator to make it a negation
  if ((strcmp(op, "-") == 0) && i == 3) {
    return -x;
  }

  return x;
}

int main(int argc, char** argv) {

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
      long result = eval(r.output);
      printf("%li\n", result);
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
