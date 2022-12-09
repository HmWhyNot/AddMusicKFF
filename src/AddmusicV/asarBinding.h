#pragma once

#include <string>
#include <filesystem>
#include <exception>
#include <vector>
#include <cstdint>

namespace AddMusic
{

namespace fs = std::filesystem;

/**
 * @brief A class to capture errors generated by Asar.
 */
class AsarException : public std::runtime_error
{
	using std::runtime_error::runtime_error;
};

/**
 * @brief A class to facilitate management of ASAR functions and error
 * management.
 * 
 * If you supply this class with only a string with assembly instructions, this
 * class may create temporary files to relieve you of the burden of
 * creating/deleting files.
 * 
 * If you supply a path, it will read and compile the assembly file as usual. 
 */
class AsarBinding
{
public:
	/**
	 * Initializes an Asar patch with a patch content. It will work fine
	 * only if you are compiling a single file.
	 * If the file has includes, they will be resolved by using the "environment_dir"
	 * parameter.
	 */
	AsarBinding(const std::string& patchcontent, const fs::path& environment_dir);

	/**
	 * Conventional way to invoke AsarBinding, with a file already located in
	 * your filesystem, whose includes will be resolved accordingly.
	 */
	AsarBinding(const fs::path& file);

	/**
	 * @brief Destructor. Deletes any temporary file(s) created.
	 */
	~AsarBinding();

	/**
	 * @brief Compiles the assembly file this object was build with.
	 * 
	 * Designed to replace the asarCompileToBIN method.
	 */
	bool compileToBin();

	/**
	 * @brief Patches a certain ROM with the assembly data this object was
	 * built with. By default it will overwrite the ROM file, but if `overwrite`
	 * is set as false, it will generate a new, patched, ROM file with an unique
	 * name, in the same folder as the input ROM.
	 * 
	 * Designed to replace the asarPatchToROM method.
	 */
	bool patchToRom(fs::path rompath, bool overwrite = true);

	/**
	 * @brief Get the result of compileToBin function. Will return an empty
	 * vector if nothing has been generated yet.
	 */
	std::vector<uint8_t> getCompiledBin() const;

	/**
	 * @brief Get the errors thrown by the last time Asar was run.
	 */
	std::string getStderr() const;

	/**
	 * @brief Get the log messages thrown by the last time Asar was run.
	 */
	std::string getStdout() const;

private:
	bool is_using_tmpfile;	// Does it use a temporary file? (will be deleted afterwards)

	std::filesystem::path _patchfilename;	// Path to the patch, can be supplied or auto-generated.
	std::vector<uint8_t> _compiledbin;		// Contents of the result of the compilation.

	std::vector<std::string> asar_stdout;	// Whatever returned Asar as result of a normal execution.
	std::vector<std::string> asar_stderr;	// Whatever returned Asar as result of an erroneous execution.
};

}