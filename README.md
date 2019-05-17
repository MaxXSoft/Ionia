# Ionia

Design and implementation of a functional scripting language.

Ionia is simple, lightweight functional programming language, with implementation of interpreter and VM runtime.

## EBNF of Ion Lang

```ebnf
program ::= {stat};
stat    ::= define | funcall;
define  ::= id "=" expr;
expr    ::= func | funcall | define | id | number;
func    ::= "(" [id {"," id}] ")" ":" expr;
funcall ::= id "(" [expr {"," expr}] ")";
id      ::= re"~([0-9]|=|\(|\)|,|:)(=|\(|\)|,|:)*";
number  ::= re"0|([1-9][0-9]*)";
```

## To-Do List

- [x] Interpreter
- [x] VM runtime
- [x] Compiler
- [ ] Documents
- [ ] Disassembler
- [ ] REPL
- [ ] JIT

## Copyright and License

Copyright (C) 2010-2019 MaxXing, MaxXSoft. License GPLv3.
