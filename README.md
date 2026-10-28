# stream-shell

Stream-shell (short name `stsh`), is a non-POSIX compliant shell, taking inspiration from shells that operates on structured data, such as Nushell, but stripping out unnecessary type abstractions and other stuff that just offers too many ways to do the same thing.

Everything in stream-shell are streams of values, including the environment and other variables. Streams are lazily consumed with back-pressure, supporting both finite and infinite sequences. They are transformed by commands, such as executable binaries, builtins, and closures (lambdas), and chained together using the `|`-operator. Streams makes working with large amounts of data a breeze, as they do not require all data to be loaded upfront before they can start interacting with it.

## Streams

In stream-shell, a stream is a sequence of singular values:

```
> 1 2 3
1
2
3
```

Even single value expressions make up a stream with 1 value:

```
> "I'm just a single string"
I'm just a single string
```

A stream can be made up by different value types:

```
> true 1 `foo`
true
1
foo
```

Streams can contain more complex values too, in the form of records. Records may be strongly typed using a schema, and are convertible to and from JSON.

```
> { name: "Albert" } { name: "Bernard" }
{ name: "Albert" }
{ name: "Bernard" }
```

Attempting to put a stream in another stream will result in a flattened stream. This is a useful property in order to concatenate or combine streams.

```
> 1 2..4 5
1
2
3
4
5
```

### Expressions

A stream expression is evaluated to a stream, and a value expression is evaluated to a value in a stream. Many of the value types can be used with arithmetic operations.

```
> 1 + 2 3 "foo" + "bar" 10 % 3
3
3
"foobar"
1
```

An operation has higher precedence than stream value delimiters (e.g. whitespace, newline), even when there is additional whitespace between the operands and operators.

```
> 1 -2 + 3
2
```

Parentheses can be used to isolate value expressions (can be seen as flattened sub-streams).

```
> 1 (-2 + 3)
1
1
```
```
> (1 -2)(+3)
-1
3
```

Options & flags are shorthand for records.
```
> --foo --no-bar -ABC
{ foo: true }
{ bar: false }
{ A: true, B: true, C: true }
```

Single-value records like these can also be assigned to.
```
> --foo=42 --bar="foo"
{ foo: 42 }
{ bar: "foo" }
```

Records may be strongly typed using a schema.

```
> app.User { name: "Albert" } { name: "Bernard" }
app.User { name: "Albert" }
app.User { name: "Bernard" }
```

A schema coerces subsequent operands to be instances of the defined type. Parentheses are useful to mark the start and end of records and groups of values.

```
> (app.User {} --name=Cedric (--name=David --age=35) app.Circle {} { radius: 10 }) {}
app.User { name: "" }
app.User { name: "Cedric" }
app.User { name: "Davic", age: 35 }
app.Circle { radius: 0 }
app.Circle { radius: 10 }
{}
```

## Transforming a Stream

Streams are transformed in stages chained together into a pipeline with the `|`-operator. Each stage consists of an expression that gets an input stream and provides an output stream. Normally, the input to the first stage of a pipeline is always an empty stream, and the output of the last stage is also the output of the pipeline as a whole. A stream expression simply discards the input stream and enumerates the values as the output stream.

### Command configuration (record)

Like many other shells, if the first value in an expression identifies an executable binary or builtin function, the expression is interpreted as a command. The remaining values (command arguments) are all conceptually part of the same command configuration, and will be consumed and merged into a single record value upon execution. Record values uses protobuf merging strategy. Whenever a string value is encountered, a nested record is added using the value as key into which subsequent record-values are merged. Non-string primitives (or any values after encoutering `"--"`) will be placed in a list keyed by `"@"`. This way, trivial arguments are bi-directionally convertible.

Example:
```
command --verbose -ab subcommand 42 --no-c --long=1234 -- foo
```
is parsed as
```
{
  "verbose": true,
  "a": true,
  "b": true,
  "subcommand": {
    "@": [42, "--", "foo"],
    "c": "false",
    "long": 1234,
  }
}
```

### Executable binaries

Streams can be generated or transformed by executing arbitrary binaries, or scripts, on your system. If the first value in an expression is a string that references an executable binary either via a relative path from the current working directory, an absolute path, or found in any of the locations listed in the `$PATH` environment stream, then an instance of that program will launch when values are consumed from the output stream of the command. The command configuration record is serialized into a list of string tokens and passed as arguments (mostly resembling the way it was written before merged into a record).

### Builtins

Stream-shell contains a bunch of builtin commands. The streams accepted as input by-, or generated as output from a builtin already have strong types, and the command configuration record is directly accesible by the builtin implementation.

### Closures

A closure is declared between brackets `{ [signature ->] [expression] }`, and consist of an optional signature, and an expression that shapes the output of the transformed stream. The closure is invoked for each value in the input stream, flattening the expression output stream as the output.

A signature assigns the input value to a named variable instead of using it as single-value input stream. This is useful when you need to pass the value in a command configuration.

```
> 1..2 | { i -> i i * 2 }
1
2
2
4
```

The closure input variable can be a record, in which case you may use dot-notation to access fields.

```
> { name: "Albert" } { name: "Bernard" } | { person -> person.name }
Albert
Bernard
```

List values in the output stream are treated as sub-streams, which means they're flattened.

```
> { numbers: [1, 2] } { numbers: [3, 4] } | { e -> e.numbers }
1
2
3
4
```

List access by index is made possible using dot-notation too.
NOTE: bracket notation is not supported.

```
> { numbers: [1, 2] } { numbers: [3, 4] } | { e -> e.numbers.0 }
1
3
```

You can transform list items just like any other stream.

```
> (
    { name: "Albert", friends: [{ name: "Bernard" }, { name: "Cedric" }] } 
    { name: "Derek" }
  ) | { person -> person.name (person.friends | { p -> `- ${p.name}` }) }
Albert
- Bernard
- Cedric
Derek
```

### Type conversion

The builtin command `to` coerces input values into a specific type.
```
> repeat "na" | head 3 | to json
["na", "na", "na"]
```

`to` operates on the whole input stream - to use per-value, wrap it in a closure.
```
> repeat "na" | head 3 | { to json }
"na"
"na"
"na"
```

## REPL

By default, streams are consumed using an interactive pull-mechanism on the command line, as opposed to the more traditional way of printing everything to stdout as fast as possible. Reading can be interrupted, even when iterating infinite streams. Finite streams returns after the last value has been printed.

### Value views

For more advanced ways of displaying values, hit [Tab] to cycle through different representations of the same value. 

### IO-mode

Executing an interactive command that reads from stdin to get user input and prints to stdout as feedback in Stream-shell will normally not work that well; no input is read and chunks of the output are pulled lazily. To work around this, Stream-shell ships with an IO-mode, where input and output is hooked up directly to the shell in- and output. This seamlessly works with pipelines and closures and can be activated by hitting [Tab] at the end of the prompt.

## Variables & Environment

The shell environment and variables are all contained in the same way - using stream definitions
assigned to a variable name. Just like common environment variables uses uppercase text in other shells,
they are exposed in exactly the same way in stream-shell:

```
> $HOME
/Users/marcus
```

Note that these are all streams:

```
> $PATH | head 2
/bin
/local/bin
```

You can assign to variables as well. Assignment does not mean that any consumption is done, so be wary
when using commands that you expect side-effects from. Variable name casing is not enforced, but to differentiate from environment variables, anything but upper-case is recommended.
```
> $myVar = 100..
```

Some environment variables can be assigned to:
```
> $PWD = /tmp
```
These streams are constantly being consumed by the shell itself, hence side-effects from running the stream pipelines will be triggered.

Assigning to `$PWD` is a way to change the current directory, for which there exists a builtin alias:
```
> cd /tmp
```

Environment variables can be overridden for a single command, to maintain compatibility with other POSIX shells
```
> FOO="bar" $FOO
bar
```
NOTE: local assignment to env var differs from stream assignment as only the next operand is used.

## Strings

Backticks are the preferred of writing strings, and can interpolate stream variables (takes the first value of the stream).

```
> `Home: $HOME`
Home: /Users/marcus
```

Single- and double quotation is also supported (no stream variable interpolation).

```
> "'single'" '"double"'
'single'
"double"
```

## Scripting

You can define and consume streams in a script file and run it:
```
#! /bin/stsh

$PATH = $PATH `$HOME/some_script_dir`

some_script $@
```

`@` is the variable containing the command configuration record.

### Configuration

The configuration script for interactive stream-shell (`config.st`) is loaded from `$XDG_CONFIG_HOME/stream-shell` if set, otherwise `~/.config/stream-shell/`. You can open it in your default text editor (`$EDITOR`) by running the `config` command.

## Shell Prompt

The `$PROMPT` environment variable can be set to customize the prompt of the shell. It uses the last emitted value to print the prompt.

### Oh My Posh

TODO: support is planned
