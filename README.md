# yunolex-2

Rewrote [this](https://github.com/ephing/yunolex) to be faster, more memory efficient, more readable, and with new features, while maintaining easy extensibility and integration.

## How to use

The basic specification of a token is as follows:

```toml
[tokenname]
regex = abc

[secondtoken]
regex = xyz
```

You can prevent certain tokens from appearing in the output token stream by using the `skip` field. Writing anything other than `true` will register as false.

```toml
[tokenname]
regex = a*
skip = true
```

You can tell the lexer to throw an exception upon reading certain tokens with the error field. Keep in mind that the message is pasted directly into the lexer template, so characters that need to be escaped in your source language must be escaped here too. Yunolex also expects the message to be surrounded by quotation marks.

```toml
[token]
regex = t+o+k+e+n+
error = "This is an error"
```

Tokens also have 3 fields related to a concept similar to scoping. The lexer keeps track of a set of 'scopes' that it is currently in. Only tokens that are also in at least one of those scopes can be lexed. You can also specify what scopes to enter and leave upon lexing certain tokens.  

The lexer starts in the global scope notated by the special character `$`, but this scope can be left and entered like any other. If you do not specify what scopes a token is in using the `in` field, yunolex will assume that it is in only the global scope.  
  
The `in`, `enter`, and `leave` fields all use space-delimited lists to specify multiple values.

```toml
[token1]
regex = 1
in = one $
enter = two
leave = one

[token2]
regex = 2
in = two
leave = two
enter = one
```

To Do list:

* ~~remake input spec to be toml-esque~~
* ~~redesign parser to treat scopes as separate sets rather than a stack~~
* test/debug regex parser and error messages
* extend to other output languages besides c++
* add helpful cmd-line arguments
* add shortcuts for specifying scopes in the spec (e.g. a wildcard)
* add error checking for values in scope lists
    - check `in` and `leave` fields for scopes that are never entered
