<picture>
  <img alt="The Runtime programming language"
       src="Runtime logo.svg"
       width="50%">
</picture>
<br>

[![Catch2 - Tests](https://github.com/Wisdurm/Runtime/actions/workflows/tests.yml/badge.svg)](https://github.com/Wisdurm/Runtime/actions/workflows/tests.yml)

# The Runtime programming language  

Runtime is a questionably-evaluated, classless yet purely-object-oriented, interpreted programming language.  
This language was made for fun and practice, and is not intended to be used for practical purposes.

## The Present

The current implementation of the interpreter is VERY inefficient. I am well aware of this, however I'm not particularly smart, and the amount
of time this project has already taken also makes me not want to dive too deep in to optimization at the moment.  
I definitely will keep this project in mind, and there's a chance I'll work on major improvements in the future, but for the time being,
there are problems with this that I am well aware exist, but which I don't intend on fixing right now.

## The Future

The standard library will almost definitely be expanded over time, to support basic stuff like io.
I also want to comment to code more thouroughly to make it easier to understand.
Even though this language is just a funny exersize, I still want this to be technically capable of complex things,
even if no one might necessarily want to bother with such.

## Building

### Dependencies 

Dependencies are Catch2, Tessil's ordered maps and GNU readline,
the former two of which which are both vcpkg packages.  
GNU readline must for the time being be installed as a shared library, for example
from your distribution's package manager.  
That is, for Debian 13 you would do:  
```sudo apt install libreadline-dev```  

gcovr is required for building tests

### Instructions for building

You can choose not to build tests by setting RUNTIME_BUILD_TESTS to false.

First make sure Git and Ninja are installed.  
```sudo apt install git ninja-build```  
Then install and setup vcpkg.  
[Vcpkg install](https://learn.microsoft.com/en-gb/vcpkg/get_started/get-started?pivots=shell-bash)  
Cd into this repository and run:  
```cmake --preset=ninja-vcpkg```  
And finally:  
```cmake --build build```  

## Examples

The [Collatz Conjecture](https://en.wikipedia.org/wiki/Collatz_conjecture) written in Runtime
```bash (obviously not actually bash but lying for the sake of syntax highlighting)
#!/usr/bin/Runtime

Object(n Input())
Print(n)
While( LargerThan(n 1)
	If( Equal( Mod(n 2) 0)
		Assign(n 0 Divide(n 2) )
		# Else
		Assign(n 0 Add( Multiply(3 n) 1) )
	)
	Print(n)
)
```

Simple script to print all symbols from a shared library

```bash
#!/usr/bin/Runtime
# First cache the amount of symbols by default
Copy(tmp, GetKeys())
Object(oldCount, Size(tmp-0))
# Then load the library
Include(carg2) # Include the library in the first command line argument
# (carg0 is program path, and carg1 is the script name)
# Now get keys again
Copy(symbols, GetKeys())
Object(symCount, Size(symbols-0))
# Then iterate and print all the ones in the included library
Append(i, oldCount)
While(SmallerThan(i, Minus(symCount,1))
	Assign(i,0,Add(i,1)) # Iterate i
	Print(symbols-0-i())
)
# TODO: The symbol table is not currently ordered...
```

## Documentation

Documentation is available at the official [Runtime site](https://runtime.wisdurm.fi/public/index.php).
