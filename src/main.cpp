#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <iostream>
#include <vector>
#include <span>
#include "GS2Context.h"
#include "visitors/GS2Decompiler.h"

struct Response
{
	CompilerResponse response;
	std::filesystem::path output_file;
	std::string errmsg;
};

struct Arguments
{
	std::vector<std::filesystem::path> input_paths;
	std::filesystem::path output_path;
	bool help = false;
	bool verbose = false;
	bool directory_mode = false;
	bool multi_file_mode = false;
	bool decompile_mode = false;
	std::string error;
};

constexpr const char* HELP_TEXT = R"(
GS2 Script Compiler/Decompiler

Usage:
  %s [OPTIONS] INPUT [OUTPUT]
  %s INPUT -o OUTPUT
  %s --help

Arguments:
  INPUT              Input file (.gs2, .txt, or .gs2bc) or directory
  OUTPUT             Output file (.gs2bc for compile, .gs2 for decompile)

Options:
  -o, --output FILE  Specify output file
  -d, --decompile    Decompile .gs2bc to .gs2 source
  -v, --verbose      Verbose output
  -h, --help         Show this help message

Examples:
  %s script.gs2                    # Creates script.gs2bc (compile)
  %s script.gs2bc -d               # Creates script.gs2 (decompile)
  %s script.gs2 output.gs2bc       # Creates output.gs2bc
  %s script.gs2bc -o output.gs2 -d # Creates output.gs2 (decompile)
  %s scripts/                      # Process directory
  %s file1.gs2 file2.gs2 file3.gs2 # Process multiple files (drag & drop)
)";

constexpr size_t count_placeholders(const std::string_view str)
{
	size_t count = 0;
	for (size_t i = 0; i + 1 < str.size(); i++)
	{
		if (str[i] == '%' && str[i + 1] == 's')
			count++;
	}
	return count;
}

template<size_t... Is>
void print_help_impl(const char* program_name, std::index_sequence<Is...>)
{
	printf(HELP_TEXT, ((void)Is, program_name)...);
}

void showHelp(const char* program_name)
{
	constexpr size_t N = count_placeholders(HELP_TEXT);
	print_help_impl(program_name, std::make_index_sequence<N>{});
}

Arguments parseArguments(int argc, const char* argv[])
{
	Arguments args;
	std::span arg_span(argv, argc);

	if (argc < 2)
	{
		args.error = "No input file specified. Use --help for usage information.";
		return args;
	}

	for (size_t i = 1; i < arg_span.size(); i++)
	{
		std::string_view arg = arg_span[i];

		if (arg == "--help" || arg == "-h")
		{
			args.help = true;
			return args;
		}

		if (arg == "--verbose" || arg == "-v")
		{
			args.verbose = true;
		}
		else if (arg == "--decompile" || arg == "-d")
		{
			args.decompile_mode = true;
		}
		else if (arg == "--output" || arg == "-o")
		{
			if (++i >= arg_span.size())
			{
				args.error = "Missing output file after " + std::string(arg);
				return args;
			}
			args.output_path = arg_span[i];
		}
		else if (arg.starts_with('-'))
		{
			args.error = "Unknown option: " + std::string(arg);
			return args;
		}
		else
			args.input_paths.emplace_back(arg);
	}

	if (args.input_paths.empty())
	{
		args.error = "No input file specified";
		return args;
	}

	// Handle positional INPUT OUTPUT form
	if (args.input_paths.size() == 2 && args.output_path.empty())
	{
		args.output_path = args.input_paths[1];
		args.input_paths.pop_back();
	}

	if (args.input_paths.size() == 1)
	{
		const auto& input_path = args.input_paths[0];

		if (std::filesystem::is_directory(input_path))
		{
			args.directory_mode = true;
			if (!args.output_path.empty())
			{
				args.error = "Output file cannot be specified for directory mode";
				return args;
			}
		}
		else if (args.output_path.empty() && !args.decompile_mode)
		{
			args.output_path = input_path;
			args.output_path.replace_extension(".gs2bc");
		}
	}
	else
	{
		args.multi_file_mode = true;
		if (!args.output_path.empty())
		{
			args.error = "Output file cannot be specified when processing multiple files";
			return args;
		}

		for (const auto& path: args.input_paths)
		{
			if (std::filesystem::is_directory(path))
			{
				args.error = "Cannot mix files and directories in multi-file mode";
				return args;
			}
		}
	}

	return args;
}

Response compileFile(const std::filesystem::path& filePath, const std::filesystem::path& outputPath = {})
{
	static GS2Context context;
	Response result{};

	// Read file using C++ streams
	std::ifstream file(filePath, std::ios::binary);
	if (!file)
	{
		result.errmsg = "Cannot open file.";
		return result;
	}

	std::string script((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

	result.response = context.compile(script);

	if (!result.response.errors.empty())
	{
		for (const auto& err: result.response.errors)
			result.errmsg.append(err.msg()).append("\n");
		return result;
	}

	// Determine output path
	result.output_file = outputPath.empty()
							 ? filePath.parent_path() / filePath.stem().concat(".gs2bc")
							 : outputPath;

	// Write bytecode
	std::ofstream outstream(result.output_file, std::ios::binary);
	outstream.write(reinterpret_cast<const char*>(result.response.bytecode.buffer()),
		static_cast<std::streamsize>(result.response.bytecode.length()));

	return result;
}

struct DecompileResult
{
	std::string source;
	std::filesystem::path output_file;
	std::string errmsg;
};

DecompileResult decompileFile(const std::filesystem::path& filePath, const std::filesystem::path& outputPath = {})
{
	DecompileResult result{};
	gs2decompiler::GS2Decompiler decompiler;

	if (!decompiler.loadBytecode(filePath.string()))
	{
		result.errmsg = decompiler.getError();
		return result;
	}

	result.source = decompiler.decompile();

	// Determine output path
	if (outputPath.empty())
		result.output_file = filePath.parent_path() / (filePath.stem().string() + ".gs2");
	else
		result.output_file = outputPath;

	// Write source
	std::ofstream outstream(result.output_file);
	outstream << result.source;

	return result;
}

bool decompileAndReport(const std::filesystem::path& inputPath, const std::filesystem::path& outputPath = {}, bool verbose = false)
{
	if (!std::filesystem::exists(inputPath))
	{
		printf(" -> [ERROR] File does not exist\n");
		return false;
	}

	if (verbose)
		printf("Decompiling file %s\n", inputPath.c_str());

	auto start = std::chrono::high_resolution_clock::now();
	auto result = decompileFile(inputPath, outputPath);
	auto finish = std::chrono::high_resolution_clock::now();

	if (verbose)
	{
		std::chrono::duration<double> diff = finish - start;
		printf("Decompiled in %f seconds\n", diff.count());
	}

	if (!result.errmsg.empty())
	{
		printf(" -> [ERROR] %s\n", result.errmsg.c_str());
		return false;
	}

	if (verbose)
		printf(" -> saved to %s\n", result.output_file.c_str());

	return true;
}

bool compileAndReport(const std::filesystem::path& inputPath, const std::filesystem::path& outputPath = {}, bool verbose = false)
{
	if (!std::filesystem::exists(inputPath))
	{
		printf(" -> [ERROR] File does not exist\n");
		return false;
	}

	if (verbose)
		printf("Compiling file %s\n", inputPath.c_str());

	auto start = std::chrono::high_resolution_clock::now();
	auto result = compileFile(inputPath, outputPath);
	auto finish = std::chrono::high_resolution_clock::now();

	if (verbose)
	{
		std::chrono::duration<double> diff = finish - start;
		printf("Compiled in %f seconds\n", diff.count());
	}

	if (!result.errmsg.empty())
	{
		printf(" -> [ERROR] %s\n", result.errmsg.c_str());
		return false;
	}

	if (verbose)
		printf(" -> saved to %s\n", result.output_file.c_str());

	return true;
}

void processFileList(const std::vector<std::filesystem::path>& files, bool verbose, std::string_view mode_name = "",
	const std::filesystem::path& single_output = {}, bool decompile_mode = false)
{
	int processed = 0;
	int errors = 0;

	if (!mode_name.empty())
		printf("Processing %zu files (%s mode):\n\n", files.size(), mode_name.data());

	for (const auto& file_path: files)
	{
		if (!mode_name.empty())
			printf("Processing: %s\n", file_path.filename().c_str());

		auto output = files.size() == 1 && !single_output.empty() ? single_output : std::filesystem::path{};
		bool success;

		if (decompile_mode)
			success = decompileAndReport(file_path, output, verbose);
		else
			success = compileAndReport(file_path, output, verbose);

		if (files.size() == 1 && !verbose && success)
		{
			auto final_output = output.empty() ?
				(decompile_mode ?
					file_path.parent_path() / (file_path.stem().string() + ".gs2") :
					file_path.parent_path() / (file_path.stem().string() + ".gs2bc"))
				: output;
			printf("%s successful\n -> saved to %s\n",
				decompile_mode ? "Decompilation" : "Compilation",
				final_output.c_str());
		}

		success ? processed++ : errors++;
	}

	if (!mode_name.empty())
		printf("\n%s processing complete: %d files processed, %d errors\n", mode_name.data(), processed, errors);
}

std::vector<std::filesystem::path> gatherFilesFromDirectory(const std::filesystem::path& dir_path, bool verbose, bool decompile_mode)
{
	std::vector<std::filesystem::path> files;

	for (const auto& entry: std::filesystem::directory_iterator(dir_path))
	{
		const auto& path = entry.path();
		auto ext = path.extension();

		if (decompile_mode)
		{
			if (ext == ".gs2bc")
				files.push_back(path);
			else if (verbose)
				printf("Skipping file %s\n", path.c_str());
		}
		else
		{
			if (ext == ".gs2" || ext == ".txt")
				files.push_back(path);
			else if (verbose)
				printf("Skipping file %s\n", path.c_str());
		}
	}

	return files;
}

int processDirectory(const std::filesystem::path& input_path, bool verbose, bool decompile_mode)
{
	if (!std::filesystem::exists(input_path) || !std::filesystem::is_directory(input_path))
	{
		std::cerr << "Error: Invalid directory: " << input_path << "\n";
		return 1;
	}

	if (verbose)
		printf("Scanning directory: %s\n", input_path.c_str());

	processFileList(gatherFilesFromDirectory(input_path, verbose, decompile_mode), verbose, "Directory", {}, decompile_mode);
	return 0;
}

int main(int argc, const char* argv[])
{
#ifdef YYDEBUG
	//  yydebug = 1;
#endif

	Arguments args = parseArguments(argc, argv);

	if (args.help)
	{
		showHelp(argv[0]);
		return 0;
	}

	if (!args.error.empty())
	{
		std::cerr << "Error: " << args.error << "\n";
		std::cerr << "Use --help for usage information.\n";
		return 1;
	}

	int result;
	if (args.directory_mode)
		result = processDirectory(args.input_paths[0], args.verbose, args.decompile_mode);
	else if (args.multi_file_mode)
	{
		processFileList(args.input_paths, args.verbose, "Multi-file", {}, args.decompile_mode);
		result = 0;
	}
	else
	{
		processFileList(args.input_paths, args.verbose, "", args.output_path, args.decompile_mode);
		result = 0;
	}

#ifdef DBGALLOCATIONS
	checkNodeOwnership();
	checkForNodeLeaks();
#endif

	return result;
}
