// I don't really know how exceptions work, but searching online it seems like no one else really does either so whatever
#pragma once
#include <stdexcept>

class ParserException : public std::exception
{
private:
    const char * const msg;
    int const line;
    const std::string file;
public:
    // Constructor
    ParserException(const char * const message, const int line, const std::string& file) : msg(message), line(line), file(file) {};
    const char* what() const throw() override
    {
        return msg;    
    }
    std::string where() const 
    {
        std::string loc = "File \"" + file; 
        loc += "\", line ";
        loc += std::to_string(line);
        return loc;    
    }
};

class TokenizerException : public std::exception
{
private:
    const char * const msg;
    int const line;
    const std::string file;
public:
    // Constructor
    TokenizerException(const char * const message, const int line, const std::string& file) : msg(message), line(line), file(file) {};
    const char* what() const throw() override
    {
        return msg;    
    }
    std::string where() const
    {
        std::string loc = "File \"" + file; 
        loc += "\", line ";
        loc += std::to_string(line);
        return loc;    
    }
};

class InterpreterException : public std::exception
{
private:
    const char * const msg;
    int const line;
    const std::string file;
public:
    // Constructor
    InterpreterException(const char * const message, const int line, const std::string& file) : msg(message), line(line), file(file) {};
    const char* what() const throw() override
    {
        return msg;    
    }
    std::string where() const
    {
        std::string loc = "File \"" + file; 
        loc += "\", line ";
        loc += std::to_string(line);
        return loc;    
    }
};
