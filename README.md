# stream-shell

Stream-shell (short name `stsh`), is a non-POSIX compliant shell, taking inspiration from shells that operates on structured data, such as Nushell, but stripping out unnecessary type abstractions and other stuff that just offers too many ways to do the same thing.

Everything in stream-shell are streams, including the environment and other variables. Streams are lazily consumed with back-pressure, supporting both finite and infinite sequences. They are transformed by commands, such as executable binaries, builtins, and closures (lambdas), and chained together using the `|`-operator. Streams makes working with large amounts of data a breeze, as they do not require all data to be loaded upfront before it can start interacting with it.

Streams are consumed by for example printing the values to stdout, or dumping them to a file, similar to more traditional shells. They can also be interpolated into other stream definitions. To re-use more complex stream definitions, you can assign the stream with its transforms to a variable, either on the command line or in a script - note that the upstream(s) and commands involved in the stream are not executed until the variable is part of another stream that is being consumed.

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

Other expressions are more obvious to represent a stream, such as a numeric range:

```
> 1..3
1
2
3
```

Streams can contain more complex values too, in the form of records. Records can be strongly typed by using a protobuf schema provided by the shell itself, plugins, or the user. Regardless if a schema exists or not, records are always convertible to JSON or proto3 binary format.

```
> user.proto.Person { name: "Albert" } { name: "Bernard" }
user.proto.Person { name: "Albert" }
{ name: "Bernard" }
```

Fun fact: the values used in the above streams may look like primitives, but are in reality represented by proto3 value wrappers. This is opaque to both transformation and consumption of the values, but it forces a uniform way to model types of streams.

```
> 0.. | describe
google.protobuf.Int32Value
> 0xFF | describe
google.protobuf.BytesValue
```

### Expressions

A stream expression is evaluated to a stream, and a value expression is evaluated to a value in a in a stream. Many of the value types can be used with arithmetic operations. An operation has higher precedence than stream value delimiters (e.g. whitespace, newline), even when there is additional whitespace between the expressions and operators.

```
> 1 + 2 3 "foo" + "bar" 10 % 3
3
3
"foobar"
1
```

Parentheses can be used to isolate value expressions, and force anonymous records.
```
> 1 -2 + 3
2
```
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
```
> 1 | ({ name: "Bernard" })
{ name: "Bernard" }
```

### Sub-streams

A stream value cannot contain other streams. Attempting to put a stream in another stream will result in a flattened stream. This is a useful property in order to concatenate or combine streams.

```
> 1 2..4 5
1
2
3
4
5
```

There is another use-case for sub-streams - they can be provided as command parameters

```
// todo: figure out a better example
> git add (ls src)
```

Streams themselves cannot be used with any arithmetic operator, but can be transformed.

## Transforming a Stream

Streams are transformed in pipelines chained together with the `|`-operator. Each transformation gets an input stream and provides an output stream. The input to a stream pipeline is always an empty stream, and the output of the last transformation is also the output of the stream pipeline itself. A semi-colon can be used to mark the end of a stream pipeline, allowing multiple stream pipelines on a single line, just like other shells.

### Executable binaries

Streams can be generated or transformed by executing arbitrary binaries on your system. Even when running a command by itself, the input will implicitly be an empty stream. The output stream type can be inferred by the shell, plugins, or user configuration in order to automatically take care of parsing. Executable binaries can be used anywhere in a streaming pipeline, including in the expression-part of a closure.

### Builtins

Stream-shell contains a few builtin commands that can be used in streaming pipelines or in closures. The streams accepted as input by-, or generated as output from a builtin already have strong types, and parsing/serialization is not enforced.

### Closures

A closure is declared between brackets `{ [signature ->] [expression(s)] }`, and consist of an optional signature, and expression(s) that shapes the output of the transformed stream. The closure is invoked for each value in the input stream.

```
> 1..2 | { add 1 }
2
3
```

A signature assigns the input value to named variable instead of using it as input stream. This is useful when you need to pass the value as command argument.

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

Lists are treated as streams, which means they're flattened into the output stream.

```
> { numbers: [1, 2] } { numbers: [3, 4] } | { e -> e.numbers }
1
2
3
4
```

You can transform list items just like any other stream.

```
> (
    { name: "Albert", friends: [{ name: "Bernard" }, { name: "Cedric" }] } 
    { name: "Derek" }
  ) | { person -> person.name (person.friends | { get name } | prepend "- ") }
Albert
- Bernard
- Cedric
Derek
```

### Formatting

The builtin command `to` transforms an input stream to a specific format.
```
> repeat "na" | tail 3 | to json
["na", "na", "na"]
```

`to` operates on the whole input stream - to use per-value, wrap it in a closure.
```
> repeat "na" | tail 3 | { to json }
"na"
"na"
"na"
```


### Writing your own commands

Most shell scripting langauges offer ways to define native functions or custom commands. Stream-shell is not adding yet another way to do something that can be easily done by writing a script or program in your favorite language, whether that's Python, Rust, TypeScript etc. See [Configuration](#Configuration) for ways to ensure you can parse the input stream correctly.

## Consuming a Stream

As you have seen in the previous examples, all streams have been printed out to stdout. This is just one of many ways to consume a stream.

### Interactive

By default, streams are consumed using an interactive pull-mechanism on the command line, as opposed to the more traditional way of printing everything to stdout as fast as possible. The following example shows an infinite stream of numbers, being pulled on the REPL. Reading can be aborted using Ctrl+C. Finite streams returns after the last value has been printed.

```
> 1..
1
[Enter]
2
[Enter]
3
[Ctrl+C]
>
```

### Automatic

For finite streams, the traditional behavior of printing to stdout line-by-line can be useful and may be enabled using a trailing `:`.

```
> "foo" "bar" | { s -> s == "bar" } :
false
true
```

#### Slicing printout

You can specify a window size in order to have succeeding values replace already printed values in-place in the terminal. This is especially useful when the upstream is emitting values with a delay. The following example shows a single line with the current time updated in realtime.

```
> now : 1
2025-03-13T22:01:59+0000
```

### Write to file

Just like some traditional shells, you can direct the output to a file instead of stdout, keeping the same syntax as bash.

```
> "hello" > lines.log
> "append hello" >> lines.log
```

Note that the POSIX syntax for redirecting stdin from a file is not available in stream-shell. Instead you generate a stream from a file
using the `open` command.

```
> open lines.log :
hello
```

### Background

Whether a stream is printed to stdout or written to a file, it's all done synchronously returning
only after the stream has finished. In the case where long-running streams are being consumed, you can let it run in the background just as you'd do it in bash.

```
> now &
```

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

### Process streams

Any external process that runs dumps their output to a process stream with the same name as their PID (`$<PID>`). Process streams doesn't use back-pressure, hence when consuming it, only new values will be emitted. A process stream is always created for the stream pipeline itself, mirroring the stream output, with a generated name (`$<shell PID>-<UID>`).

These streams are useful in order to listen in to the progress of backgrounded streams, or concurrently running pipelines in a multiplex enironment.


```
> stsh ./count_sheep.st
> sleep 100
> $1234
103 sheeps
104 sheeps
```

```
> iota | { i -> `$i sheeps` }
> sleep 100
> $1234-abcd
103 sheeps
104 sheeps
```


## Strings

```
>"It's not like using string $NOTAVARIABLE"
It's not like using string $NOTAVARIABLE
```
```
>'But you can use "quotes" if needed'
But you can use "quotes" if needed
```
```
> `Backticks interpolates stream variables: $HOME`
Backticks interpolates stream variables: /Users/marcus
```

## Scripting
You can define and consume streams in a script file and run it:
```
#! /bin/stsh

$PATH = $PATH `$HOME/add_one/bin`

add_one $*
```

```
> add_one_wrapper.st 1..3
2
3
4
```

### Configuration

The configuration script for interactive stream-shell (`config.st`) is loaded from `$XDG_CONFIG_HOME/stream-shell` if set, otherwise `~/.config/stream-shell/`. You can open it in your default text editor (`$EDITOR`) by running the `config` command.

## Shell Prompt

The `$PROMPT` environment variable can be set to customize the prompt of the shell. Similar to slicing consumption (`:1`), it uses the last emitted value to print the prompt.

### Oh My Posh

TODO: support is planned
