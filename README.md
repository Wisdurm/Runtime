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

This is a standard CMake project, and the only dependencies are Catch2 and Tessil's ordered maps,
which are both vcpkg packages.  
You can choose not to build tests by setting RUNTIME_BUILD_TESTS to false.

### Instructions for building

First make sure Git and Ninja are installed.  
```sudo apt install git ninja-build```  
Then install and setup vcpkg.  
[Vcpkg install](https://learn.microsoft.com/en-gb/vcpkg/get_started/get-started?pivots=shell-bash)  
Cd into this repository and run:  
```cmake --preset=ninja-vcpkg```  
And finally, go into the build folder and simply run:  
```ninja```  

## Examples

The [Collatz Conjecture](https://en.wikipedia.org/wiki/Collatz_conjecture) written in Runtime
```
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

TODO
