// I don't really know how exceptions work, but searching online it seems like no one else really does either so whatever
#pragma once
#include <stdexcept>

class ParserException : public std::exception
{
private:
    char const * const msg;
public:
    // Constructor
    ParserException(char const* const message) : msg(message) {};
    const char* what() const throw() override
    {
        return msg;    
    }
};

class TokenizerException : public std::exception
{
private:
    char const * const msg;
public:
    // Constructor
    TokenizerException(char const* const message) : msg(message) {};
    const char* what() const throw() override
    {
        return msg;    
    }
};