<picture>
  <img alt="The Runtime programming language"
       src="Runtime logo.svg"
       width="50%">
</picture>
<br>

[![Catch2 - Tests](https://github.com/Wisdurm/Runtime/actions/workflows/tests.yml/badge.svg)](https://github.com/Wisdurm/Runtime/actions/workflows/tests.yml)

(master is at the moment not stable, please check the tags for "fully functional" versions)

# The Runtime programming language  

Runtime is a questionably-evaluated, classless yet purely-object-oriented, interpreted programming language.  
Imagine Lisp, but with objects instead of lists.  
This language was made for fun and practice, and is not intended to be used for practical purposes.

## The Present

The current implementation of the interpreter is VERY inefficient, and the standard library is also a bit lacking.
I am well aware of this, however I'm not particularly smart, and due to my highly fluctuating motivation to work on this,
I can't promise it being improved upon very soon (or necessarily ever).

## The Future

None of the following are promises, but I have reason to believe many of them will \*\*eventually\*\* be done:
- Proper struct support for FFI (v0.13.0)
- Refactored documentation
- Larger standard library (although it will ideally still stay quite minimal)
- Refactor as a library for easy embedding
- Optimization (size, speed, safety, code readability)

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

Variables, functions and callbacks
```bash (obviously not actually bash but lying for the sake of syntax highlighting)
#!/usr/bin/Runtime

# Create variable test with the value of 1
Object(test 1)

# Function which adds two args
Object(AddArgs
	Object(result Add(arg1 arg2))
	result)

# Same thing, but more compact
Object(AddArgs2
	Add(arg1 arg2))

# Callbacks
Object(CallFuncs
	Print(Format("Function 1: $\nFunction 2: $" 
		func1(test 5)
		func2(test 5))))

Print("AddArgs: " AddArgs(test 1))
Print("AddArgs2: " AddArgs2(test 2))
CallFuncs(AddArgs AddArgs2)
CallFuncs(AddArgs)

# Output:
# AddArgs: 2.000000
# AddArgs2: 3.000000
# Function 1: 6.000000
# Function 2: 6.000000
# Function 1: 6.000000
# Function 2: 0.000000
```

The [Collatz Conjecture](https://en.wikipedia.org/wiki/Collatz_conjecture) written in Runtime
```bash
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

## Documentation

Documentation is available at the official [Runtime site](https://runtime.wisdurm.fi/public/index.php).
(CURRENTLY DOWN!)
