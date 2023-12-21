# yunolex-2

Rewrote [this](https://github.com/ephing/yunolex) to be faster, more memory efficient, more readable, and with new features, while maintaining easy extensibility and integration.

## Motivation

Yunolex is a lexer builder much like flex or antlr4. A lexer is a program that takes in some stream of characters (e.g. `10+2`) and converts it into a stream of tokens (e.g. `[(NUMBER:10),(PLUS:+),(NUMBER,2)]`), such that you can write some parser, interpreter or other algorithm in the realm of semantic meaning rather than a character-by-character basis. A lexer *builder*, then, is a program that takes the specification for what the tokens are (the valid values for a token are defined by a regular expression) and outputs a lexer that will lex an input stream with that specification.

This is a long solved problem, with many existing maintained alternatives. However, some, like flex, do not offer great language support and have remarkably unintuitive or complex input formats. Others, like antlr4, are actually predominantly parser builders. Yunolex exists solely so that I could trivially attribute a degree of semantic meaning to any stream of characters in exactly the format *I* wanted.

## How to use

### Creating a lexer

The basic specification of a token is as follows:

```
[tokenname]
regex = abc

[secondtoken]
regex = xyz
```

You can prevent certain tokens from appearing in the output token stream by using the `skip` field. Writing anything other than `true` will register as false.

```
[tokenname]
regex = a*
skip = true
```

You can tell the lexer to throw an exception upon reading certain tokens with the error field. Keep in mind that the message is pasted directly into the lexer template, so characters that need to be escaped in your source language must be escaped here too. Yunolex also expects the message to be surrounded by quotation marks.

```
[token]
regex = t+o+k+e+n+
error = "This is an error"
```

Tokens also have 3 fields related to a concept similar to scoping. The lexer keeps track of a set of 'scopes' that it is currently in. Only tokens that are also in at least one of those scopes can be lexed. You can also specify what scopes to enter and leave upon lexing certain tokens.  

The lexer starts in the global scope notated by the special character `$`, but this scope can be left and entered like any other. If you do not specify what scopes a token is in using the `in` field, yunolex will assume that it is in only the global scope.  
  
The `in`, `enter`, and `leave` fields all use space-delimited lists to specify multiple values.

```
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

After creating the input specification, running the thing should be as simple as passing the input as a command line argument (e.g. `./yunolex input.yuno`)

### Integrating with other projects

So you have a lexer generated by Yunolex. In order to use it, you simply need import the file, create a Lexer object, and call its `lex` function.

<table>
<tr><td><b>Language</b></td><td><b>Trivial Example</b></td><td><b>Output</b></td></tr>
<tr>
  <td>C++</td>
  <td>

```
#include "lexer.h"
std::istream input; // presume you are reading from a file or something
auto lexer = Lexer::Lexer();
auto tokenstream = lexer.lex(input);
```
  </td>
  <td>
  std::vector of Tokens (an object containing the name of the token, the stream of characters that was matched, and the position in the input where they were). 
  </td>
</tr>
</table>
This table should be extended whenever a new language is supported.

## To Do list:

* ~~remake input spec to be toml-esque~~
* ~~redesign parser to treat scopes as separate sets rather than a stack~~
* test/debug regex parser and error messages
* extend to other output languages besides c++
* add helpful cmd-line arguments
* add shortcuts for specifying scopes in the spec (e.g. a wildcard)
* add error checking for values in scope lists
  * check `in` and `leave` fields for scopes that are never entered
