## v0.9.4

* Changed implementation of object class to not depend on undefined behaviour :)
* Implemented exceptions and exception handling for the tokenizer and parser
* Directly mapped a number of mathematical functions to their C++ counterparts. For example all trigonometric functions

## v0.9.3

* You can now use negative values in source code
* Implemented SourceLocation, although it is not yet visible anywhere
* Replaced ExtendedMath.rnt with StandardMath.h functions
	* Sine()
	* Cosine()
* Added functions to StandardLibrary.h
	* Input()

## v0.9.2

* Attempted to properly implement lazy evaluation
* Implemented decimal value support
* Added StandardMath.h library
	* Add()
	* Minus()
	* Multiply()
	* Divide()
	* Mod()
	* LargerThan()
* Added more standard library functions
	* Return() might be removed later, added for now due to laziness in interpreter implementation
	* If()
	* While()
	* Not()
* Added ExtendedMath.rnt library
	* Pi
	* Factorial()
	* Power()
	* Sine() Unfinished for now, only works right for certain values

## Log keeping begins here

If you need to know about the progress in earlier commits, you're going to have to go on commit messages, or manually dig through the code changes.