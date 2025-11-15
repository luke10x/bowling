#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <optional>

#include "api/mesh_data.h"

#include "cmd_mesh.cpp"

struct CmdArgs {
    std::vector<std::string> positionals;
    std::unordered_map<std::string, std::string> options;
};

using CommandHandler = int(*)(const CmdArgs&);


// ----------------------
// Small argument parser
// ----------------------
CmdArgs parse_subcommand_args(int argc, char** argv, int start)
{
    CmdArgs out;

    for (int i = start; i < argc; ++i) {
        std::string a = argv[i];

        if (a.find('-', 0) == 0 && a.size() >= 1) {
            // key must have a value
            if (i + 1 >= argc)
                throw std::runtime_error("Missing value for option: " + a);

            out.options[a] = argv[++i];
        } else {
            out.positionals.push_back(a);
        }
    }

    return out;
}


// -----------------------------------------
// Handlers for mesh/font/animation commands
// -----------------------------------------
int handle_mesh(const CmdArgs& args)
{
    if (args.positionals.size() < 2) {
        std::cerr << "mesh: requires <input_glb> <meshName>\n";
        return 1;
    }

    const std::string input   = args.positionals[0];
    const std::string meshName = args.positionals[1];

    auto it = args.options.find("-o");
    if (it == args.options.end()) {
        std::cerr << "mesh: missing -o <output>\n";
        return 1;
    }
    const std::string output = it->second;


    std::cout << "→ mesh command\n";
    std::cout << "   input:  " << input << "\n";
    std::cout << "   mesh:   " << meshName << "\n";
    std::cout << "   output: " << output << "\n";

    cmd_mesh(input, meshName, output);

    return 0;
}


int handle_font(const CmdArgs& args)
{
    if (args.positionals.size() < 1) {
        std::cerr << "font: requires <font_file>\n";
        return 1;
    }

    const std::string font = args.positionals[0];

    auto sizeIt = args.options.find("-s");
    if (sizeIt == args.options.end()) {
        std::cerr << "font: missing -s <size>\n";
        return 1;
    }
    float size = std::stof(sizeIt->second);

    auto outIt = args.options.find("-o");
    if (outIt == args.options.end()) {
        std::cerr << "font: missing -o <output>\n";
        return 1;
    }

    std::cout << "→ font command\n";
    std::cout << "   font:   " << font << "\n";
    std::cout << "   size:   " << size << "\n";
    std::cout << "   output: " << outIt->second << "\n";

    // call your font builder…
    return 0;
}


int handle_animation(const CmdArgs& args)
{
    if (args.positionals.size() < 1) {
        std::cerr << "animation: requires <input_glb>\n";
        return 1;
    }

    const std::string input = args.positionals[0];

    auto cfgIt = args.options.find("-cfg");
    auto outIt = args.options.find("-o");

    if (cfgIt == args.options.end()) {
        std::cerr << "animation: missing -cfg <config_path>\n";
        return 1;
    }
    if (outIt == args.options.end()) {
        std::cerr << "animation: missing -o <output>\n";
        return 1;
    }

    std::cout << "→ animation command\n";
    std::cout << "   input:   " << input << "\n";
    std::cout << "   cfg:     " << cfgIt->second << "\n";
    std::cout << "   output:  " << outIt->second << "\n";

    // call your animation builder…
    return 0;
}



// ------------------
// Subcommand table
// ------------------
int main(int argc, char** argv)
{
    if (argc < 2) {
        std::cerr << "Usage: assman <command> ...\n";
        return 1;
    }

    const std::string cmd = argv[1];

    static std::unordered_map<std::string, CommandHandler> table = {
        { "mesh",      handle_mesh },
        { "font",      handle_font },
        { "animation", handle_animation },
    };

    auto it = table.find(cmd);
    if (it == table.end()) {
        std::cerr << "Unknown command: " << cmd << "\n";
        return 1;
    }

    CmdArgs parsed = parse_subcommand_args(argc, argv, 2);
    return it->second(parsed);
}
