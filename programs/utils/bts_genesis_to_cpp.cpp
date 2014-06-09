#include <bts/utilities/string_escape.hpp>
#include <fc/optional.hpp>
#include <fc/filesystem.hpp>
#include <fc/io/json.hpp>
#include <fc/variant_object.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/exception/exception.hpp>
#include <fc/log/logger.hpp>

#include <boost/program_options.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/tokenizer.hpp>

#include <iostream>
#include <fstream>
#include <list>
#include <set>

void compile_genesis_block_to_source(const fc::path& source_file_name, const fc::path& genesis_block_file_name)
{
  std::ifstream genesis_block_file(genesis_block_file_name.string());
  std::ofstream source_file(source_file_name.string());

  source_file << "// This file is generated by bts_genesis_to_cpp from " << genesis_block_file_name.string() << "\n";
  source_file << "#include <bts/blockchain/genesis_json.hpp>\n";
  source_file << "#include <string>\n\n";
  source_file << "namespace bts { namespace blockchain {\n\n";
  source_file << "static const char* const genesis_json_lines[] =\n";
  source_file << "{\n";
  std::string line;
  bool first = true;
  while (std::getline(genesis_block_file, line))
  {
    if (first)
      first = false;
    else
      source_file << ",\n";
    source_file << "  " << bts::utilities::escape_string_for_c_source_code(line);
  }
  source_file << "};\n\n";

  source_file << "std::string get_builtin_genesis_json_as_string()\n";
  source_file << "{\n";
  source_file << "  std::ostringstream result;\n";
  source_file << "  for (unsigned i = 0; i < sizeof(genesis_json_lines)/sizeof(genesis_json_lines[0]); ++i)\n";
  source_file << "    result << genesis_json_lines[i] << \"\\n\";\n";
  source_file << "  return result.str();\n";
  source_file << "}\n\n";
  source_file << "} } // end namespace bts::blockchain\n";
}

int main(int argc, char*argv[])
{
  // parse command-line options
  boost::program_options::options_description option_config("Allowed options");
  option_config.add_options()("help",                                                             "display this help message")
                             ("genesis-json",  boost::program_options::value<std::string>(), "The genesis.json file to convert to C++ source code")
                             ("output-file",   boost::program_options::value<std::string>(), "The file to generate");
  boost::program_options::variables_map option_variables;
  try
  {
    boost::program_options::store(boost::program_options::command_line_parser(argc, argv).
      options(option_config).run(), option_variables);
    boost::program_options::notify(option_variables);
  }
  catch (boost::program_options::error&)
  {
    std::cerr << "Error parsing command-line options\n\n";
    std::cerr << option_config << "\n";
    return 1;
  }

  if (option_variables.count("help"))
  {
    std::cout << option_config << "\n";
    return 0;
  }

  if (!option_variables.count("genesis-json"))
  {
    std::cout << "Missing argument --genesis-json\n";
    return 1;
  }
  if (!option_variables.count("output-file"))
  {
    std::cout << "Missing argument --output-file\n";
    return 1;
  }
  
  try
  {
    compile_genesis_block_to_source(option_variables["output-file"].as<std::string>(), option_variables["genesis-json"].as<std::string>());
  }
  catch (const fc::exception& e)
  {
    elog("Caught while compiling genesis block: ${msg}", ("msg", e.to_detail_string()));
    return 1;
  }
  return 0;
}