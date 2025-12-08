#pragma once
// This file contains all the I/O functions in Runtime
#include "StandardFiles.h"
#include "../interpreter.h"
// C++
#include <variant>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <fstream>

namespace rt
{
    // Keep track of opened files, since the info can't be stored in the file objects themselves
    std::unordered_map<std::string, std::fstream> openedFiles = {};

    /// <summary>
	/// Creates a file object. The object is written into the first argument. 2 arg is file path
	/// </summary>
	/// <param name="args"></param>
	/// <param name="symtab"></param>
	/// <param name="argState"></param>
	/// <returns></returns>
	objectOrValue FileCreate(std::vector<objectOrValue>& args, SymbolTable* symtab, ArgState& argState)
	{
        // Create file object
        if (args.size() > 1)
	    {
            std::shared_ptr<Object> file = std::get<std::shared_ptr<Object>>(args.at(0));
            auto arg1 = VALUEHELD(args.at(1));
            std::string path;
            if (std::holds_alternative<std::string>(arg1))
                path = std::get<std::string>(arg1);
            else
				return giveException("Path is of wrong type");
            
            file->addMember(path, "path"); // Path to the file
            file->addMember(False, "open"); // Whether open or not
            file->addMember(0.0, "pointer"); // Part in file from where to read and where to write
            return True;
	    }    
		return giveException("Wrong amount of arguments");
	}    
    
    /// <summary>
	/// Opens a file object
	/// </summary>
	/// <param name="args"></param>
	/// <param name="symtab"></param>
	/// <param name="argState"></param>
	/// <returns></returns>
	objectOrValue FileOpen(std::vector<objectOrValue>& args, SymbolTable* symtab, ArgState& argState)
	{
        // Open file object
        if (args.size() > 0)
	    {
            std::shared_ptr<Object> file = std::get<std::shared_ptr<Object>>(args.at(0));
            // Get path
            std::string path;
            auto p = VALUEHELD(*file->getMember(std::string("path"))); // Constructor jank; the ghost of ast::value
            if (std::holds_alternative<std::string>(p))
                path = std::get<std::string>(p);
            else
				return giveException("Path is of wrong type");
            // Open file
			if (openedFiles.contains(path)) {
				return giveException("File is already opened");
			}
            openedFiles.insert({path, std::fstream()});
            std::fstream* f = &openedFiles.at(path);
            f->open(path, std::ios::in | std::ios::out);
            if (f->fail())
            {
				return giveException("Unable to open file");
		    }
            file->setMember("open", True);
            return True;
	    }    
		return giveException("Wrong amount of arguments");
	}  

    /// <summary>
	/// Closes a file object
	/// </summary>
	/// <param name="args"></param>
	/// <param name="symtab"></param>
	/// <param name="argState"></param>
	/// <returns></returns>
	objectOrValue FileClose(std::vector<objectOrValue>& args, SymbolTable* symtab, ArgState& argState)
	{
        // Open file object
        if (args.size() > 0)
	    {
            std::shared_ptr<Object> file = std::get<std::shared_ptr<Object>>(args.at(0));
            // Get path
            std::string path;
            auto p = VALUEHELD(*file->getMember(std::string("path"))); // Constructor jank; the ghost of ast::value
            if (std::holds_alternative<std::string>(p))
                path = std::get<std::string>(p);
            else
				return giveException("Path is of wrong type");
            // Close file
            openedFiles.at(path).close();
            openedFiles.erase(path);
            file->setMember("open", False);
            return True;
	    }    
		return giveException("Wrong amount of arguments");
	}  

    /// <summary>
	/// Reads a line from a file object
	/// </summary>
	/// <param name="args"></param>
	/// <param name="symtab"></param>
	/// <param name="argState"></param>
	/// <returns></returns>
	objectOrValue FileReadLine(std::vector<objectOrValue>& args, SymbolTable* symtab, ArgState& argState)
	{
        // Read line
        if (args.size() > 0)
	    {
            std::shared_ptr<Object> file = std::get<std::shared_ptr<Object>>(args.at(0));
            // Get path
            std::string path;
            auto p = VALUEHELD(*file->getMember(std::string("path"))); // Constructor jank; the ghost of ast::value
            if (std::holds_alternative<std::string>(p))
                path = std::get<std::string>(p);
            else
				return giveException("Path is of wrong type");
			// Open file
            std::fstream* f = &openedFiles.at(path);
            if (not f->is_open())
				return giveException("File failed to open");
			// Read line
            std::string line;
            std::getline(*f, line);
            return line;
	    }    
		return giveException("Wrong amount of arguments");
	}  

	/// <summary>
	/// Writes a line to the end of a file.
	/// </summary>
	/// <param name="args"></param>
	/// <param name="symtab"></param>
	/// <param name="argState"></param>
	/// <returns></returns>
	objectOrValue FileAppendLine(std::vector<objectOrValue>& args, SymbolTable* symtab, ArgState& argState)
	{
		// Append line
        if (args.size() > 1)
	    {
            std::shared_ptr<Object> file = std::get<std::shared_ptr<Object>>(args.at(0));
            // Get path
            std::string path;
            auto p = VALUEHELD(*file->getMember(std::string("path"))); // Constructor jank; the ghost of ast::value
            if (std::holds_alternative<std::string>(p))
                path = std::get<std::string>(p);
            else
				return giveException("Path is of wrong type");
            // Open file
            std::fstream* f = &openedFiles.at(path);
            if (not f->is_open())
				return giveException("File failed to open");
            // Get text to add
            auto write = VALUEHELD(args.at(1));
			if (not std::holds_alternative<std::string>(write))
			{
				return giveException("String is of wrong type");
			}
			// Position at end
			f->seekg (0, std::ios::end);
			// Write value to file
			*f << std::get<std::string>(write) << '\n';
			return True;
	    }    
		return giveException("Wrong amount of arguments");

	}

    /// <summary>
	/// Writes the second argument to the file in the first argument
	/// </summary>
	/// <param name="args"></param>
	/// <param name="symtab"></param>
	/// <param name="argState"></param>
	/// <returns></returns>
	objectOrValue FileWrite(std::vector<objectOrValue>& args, SymbolTable* symtab, ArgState& argState)
	{
		// Write line
        if (args.size() > 1)
	    {
            std::shared_ptr<Object> file = std::get<std::shared_ptr<Object>>(args.at(0));
            // Get path
            std::string path;
            auto p = VALUEHELD(*file->getMember(std::string("path"))); // Constructor jank; the ghost of ast::value
            if (std::holds_alternative<std::string>(p))
                path = std::get<std::string>(p);
            else
				return giveException("Path is of wrong type");
            // Open file
            std::fstream* f = &openedFiles.at(path);
            if (not f->is_open())
				return giveException("File failed to open");
			// Get file position
			auto v = VALUEHELD(*file->getMember(std::string("pointer")));
			if (not std::holds_alternative<double>(v)) {
				return giveException("Pointer is of wrong type");
			}
			const int pos = std::get<double>(v);
			f->seekp(pos);
            // Get text to add
            auto write = VALUEHELD(args.at(1));
			if (not std::holds_alternative<std::string>(write))
			{
				return giveException("String is of wrong type");
			}
			// Write value to file
			*f << std::get<std::string>(write);
			return True;
	    }    
		return giveException("Wrong amount of arguments");
	}

    /// <summary>
	/// Reads a specified amount of data from a file.
	/// </summary>
	/// <param name="args"></param>
	/// <param name="symtab"></param>
	/// <param name="argState"></param>
	/// <returns></returns>
	objectOrValue FileRead(std::vector<objectOrValue>& args, SymbolTable* symtab, ArgState& argState)
	{
        // Read line
        if (args.size() > 0)
	    {
            std::shared_ptr<Object> file = std::get<std::shared_ptr<Object>>(args.at(0));
            // Get path
            std::string path;
            auto p = VALUEHELD(*file->getMember(std::string("path"))); // Constructor jank; the ghost of ast::value
            if (std::holds_alternative<std::string>(p))
                path = std::get<std::string>(p);
            else
				return giveException("Path is of wrong type");
            // Open file
            std::fstream* f = &openedFiles.at(path);
            if (not f->is_open())
                return giveException("File failed to open");
            // Get file position
			auto v = VALUEHELD(*file->getMember(std::string("pointer")));
			if (not std::holds_alternative<double>(v)) {
				return giveException("Pointer is of wrong type");
			}
			const int pos = std::get<double>(v);
			f->seekg(pos);
			// Get data amount
			auto a = VALUEHELD(args.at(1));
			if (not std::holds_alternative<double>(a)) {
				return giveException("Amount is of wrong type");
			}
			const int amount = std::get<double>(a);
			// Read data
			char * memblock = new char [amount];
			f->read(memblock, amount);
			std::string data(memblock, amount);
			delete[] memblock;
			// Update pointer
			file->setMember("pointer", static_cast<double>(f->tellg()));
			return data;
	    }    
		return giveException("Wrong amount of arguments");
	}  
}
