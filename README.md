# Some Note

Some note for creating Ion Lang.

## EBNF of Ion Lang

```ebnf
program ::= {stat ";"};
stat	::=	define | funcall;
define  ::= id "=" expr;
expr    ::= func | funcall | id | number;
func    ::= "(" [id {"," id}] ")" ":" expr;
funcall ::= id "(" [expr {"," expr}] ")";
id      ::= re"~([0-9]|=|\(|\)|,|:)(=|\(|\)|,|:)*";
number  ::= re"0|([1-9][0-9]*)";
```
