# mylisp
fun little project creating a lisp-like language
based off buildyourownlisp.com

dependencies: https://github.com/orangeduck/mpc
	      libedit-dev
	      
cc -std=c99 -Wall repl.c mpc.c -ledit -lm -o repl
