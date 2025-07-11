# 🦎 Lizard Programming Language

**Lizard** is a interpreted programming language with a clean syntax, static type, and friendly error messages. It's inspired by modern languages like Rust and Python, but focused on ease of use and readability. The `.lz` file extension is used for Lizard source files.

> Lizard is still under development. Contributions, suggestions, and feedback are welcome!

## Installation 
Make sure `make` is installed on your system, Run `make help` to see all the options.

## Features

### Comments

Lizard supports both single-line and multi-line comments:

```lz
# This is a single-line comment

###
This is a 
multi-line comment
###
```

## Functions

Lizard supports function definitions with both automatic and explicit return type declarations:
```lz
# Function with auto type detection
fnc sayHello(string name) {
    return "Hai " + name;
}

println(sayHello("Adit"));

# Function with explicit return type
fnc add(int a, int b) -> int {
    return a + b;
}

println(add(30, 20));
```

## Arithmetic Operations

```lz
fnc minus(int a, int b) -> int {
    return a - b;
}

fnc times(int a, int b) -> int {
    return a * b;
}

fnc division(int a, int b) -> float {
    return a / b;
}

fnc div_int(int a, int b) -> int {
   return a %% b
}

fnc modulo(int a, int b) {
   return a % b
}

println(div_int(100, 23))
println(modulo(2, 100))
println(minus(30, 20));
println(times(300, 800));
println(division(100, 20));
```

## Printing

Output values using the println() function:
```lz
println("Hello World");

```

## Variables
Lizard supports constant (immutable) variables using the `fixed` syntax.
Constants can only be assigned once and are enforced at runtime:
```lz
fixed let int: age = 21;

fixed let string: name;
name = "Adit";     # ✅ valid (first assignment)
name = "Alice";    # ❌ Runtime Error: Cannot reassign fixed variable

# Mutable variable
let string: country;
country = "Indonesia"
```
Lizard will display a friendly runtime error if a fixed variable is reassigned, but will continue executing the rest of the program.

## Roadmap / TODO

- [ ] import statement
- [ ] if / else control structures
- [ ] for loop
- [ ] while / do loop
- [ ] Macro functions (under consideration for interpreted language)
- [ ] Fully featured Object-Oriented Programming (OOP)
- [ ] Standard Library (0% complete)
- [x] String formatting (e.g., "Hello ${name}")
- [ ] Assignment operators (+=, -=, etc.)
- [ ] Logical and comparison operators (==, !=, &&, ||, etc.)
- [ ] Boolean data type and expressions
- [ ] Error handling (try/catch, custom errors)
- [ ] Arrays / Lists
- [ ] Hash maps / Dictionaries
- [ ] Function overloading
- [ ] Pattern matching / match expression
- [ ] Module system


## Examples

See the `tests/` directory for more example code written in Lizard.

## Development Status

Lizard is still a work in progress, with a focus on simplicity, clarity, and developer-friendly error messages. Expect breaking changes and rapid improvements as the language evolves.


## License

This project is licensed under the GNU General Public License v3.0 (GPLv3).
See the LICENSE file for more information.


## Contributions

Feel free to fork, open issues, or suggest ideas! Your input will help shape Lizard into a powerful and user-friendly language.