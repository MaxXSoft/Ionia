# Ionia

Design and implementation of a functional scripting language.

Ionia is simple, lightweight functional programming language, with an interpreter and VM runtime implementation.

## EBNF of Ionia

```ebnf
program ::= {stat};
stat    ::= define | funcall;
define  ::= id "=" expr;
expr    ::= func | funcall | define | id | number;
func    ::= "(" [id {"," id}] ")" ":" expr;
funcall ::= (id | funcall) "(" [expr {"," expr}] ")";
id      ::= re"~([0-9]|=|\(|\)|,|:)(=|\(|\)|,|:)*";
number  ::= re"0|([1-9][0-9]*)";
```

## To-Do List

- [x] Interpreter
- [x] VM runtime
- [x] Compiler
- [x] Disassembler
- [ ] Documents
- [ ] Optimizer
- [x] REPL
- [ ] JIT
- [ ] Tutorial

## Copyright and License

Copyright (C) 2010-2019 MaxXing, MaxXSoft. License GPLv3.
