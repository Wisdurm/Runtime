<picture>
  <img alt="The Runtime programming language"
       src="Runtime logo.svg"
       width="50%">
</picture>
<br>

[![Catch2 - Tests](https://github.com/Wisdurm/Runtime/actions/workflows/tests.yml/badge.svg)](https://github.com/Wisdurm/Runtime/actions/workflows/tests.yml)

# The Runtime programming language  
Runtime is a very-high-level, questionably-evaluated, spaghetti-typed, purely-object-oriented, interpreted programming language.  
This language was made for fun and practice, and is not intended to be used for practical purposes.

## Notes
The current implementation of the interpreter is VERY inefficient. I am well aware of this, however I'm not particularly smart, and the amount
of time this project has already taken also makes me not want to dive too deep in to optimization at the moment.  
I definitely will keep this project in mind, and there's a chance I'll work on major improvements in the future, but for the time being,
there are problems with this that I am well aware exist, but which I don't intend on fixing right now.

## Building
This is a standard CMake project, and the only dependency is Catch2, if you choose to build tests.

## Documentation
Objects can be called, or evaluated
Commas are syntactic sugar, completely unnecessary

TODO:
* Proper error messages
* Other mathematical functions
* Change members from unordered_map into normal map
* Cleanup comments:
  * Remove useless stuff
  * Comment more thouroughly
* Final todo's
TODO: Fix all "todos" before publishing code
