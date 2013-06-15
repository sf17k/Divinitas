#include "generate.h"
#include "optionparser.h"

#include <iostream>
#include <string>
#include <cstdlib>
using namespace std;

// optionparser extensions
struct Arg : public option::Arg
{
    static void printError(const char* msg1, const option::Option& opt, const char* msg2) {
        cerr << msg1 << string(opt.name, opt.namelen) << msg2;
    }
    static option::ArgStatus Unknown(const option::Option& option, bool msg) {
        if (msg)
            printError("Unknown option '", option, "'\n");
        return option::ARG_ILLEGAL;
    }
    static option::ArgStatus Required(const option::Option& option, bool msg) {
        if (option.arg != 0)
            return option::ARG_OK;
        if (msg)
            printError("Option '", option, "' requires an argument\n");
        return option::ARG_ILLEGAL;
    }
    static option::ArgStatus NonEmpty(const option::Option& option, bool msg) {
        if (option.arg != 0 && option.arg[0] != 0)
            return option::ARG_OK;
        if (msg)
            printError("Option '", option, "' requires a non-empty argument\n");
        return option::ARG_ILLEGAL;
    }
    static option::ArgStatus Numeric(const option::Option& option, bool msg) {
        char* endptr = 0;
        if (option.arg != 0 && strtol(option.arg, &endptr, 10)) {}
        if (endptr != option.arg && *endptr == 0)
            return option::ARG_OK;
        if (msg)
            printError("Option '", option, "' requires a numeric argument\n");
        return option::ARG_ILLEGAL;
    }
};

// options accepted by program
enum optionIndex {
    UNKNOWN, HELP, SIZE, PADDING, PT_SCALEH, PT_SCALEV
};
const option::Descriptor usage[] = {
{ UNKNOWN, 0,"","",        Arg::Unknown, "USAGE:\n   divinitas [options] world_name\n\nOptions:" },
{ HELP,    0,"h","help",   Arg::None,    "  -h, \t--help  \tPrint usage and exit." },
{ SIZE,    0,"s","size",   Arg::Numeric, "  -s <num>, \t--size=<num>  \tSize of world in chunks." },
{ PADDING, 0,"p","padding",Arg::Numeric, "  -p <num>, \t--padding=<num>  \tWidth of border of empty chunks around world (default 0)." },
{ PT_SCALEH,0,"","ptscaleh",Arg::Numeric,"   \t--ptscaleh=<num>  \tPlaTec horizontal scale (default 2)." },
{ PT_SCALEV,0,"","ptscalev",Arg::Numeric,"   \t--ptscalev=<num>  \tPlaTec vertical scale (default 4)." },
/*
{ OPTIONAL,0,"o","optional",Arg::Optional,"  -o[<arg>], \t--optional[=<arg>]"
                                          "  \tTakes an argument but is happy without one." },
{ REQUIRED,0,"r","required",Arg::Required,"  -r <arg>, \t--required=<arg>  \tMust have an argument." },
{ NUMERIC, 0,"n","numeric", Arg::Numeric, "  -n <num>, \t--numeric=<num>  \tRequires a number as argument." },
{ NONEMPTY,0,"1","nonempty",Arg::NonEmpty,"  -1 <arg>, \t--nonempty=<arg>  \tCan NOT take the empty string as argument." },
*/
{0,0,0,0,0,0}
};


int main(int argc, char* argv[])
{
    argc-=(argc>0); argv+=(argc>0); // skip program name argv[0] if present
    option::Stats stats(usage, argc, argv);
    option::Option* options = new option::Option[stats.options_max];
    option::Option* buffer = new option::Option[stats.buffer_max];
    option::Parser parse(true, usage, argc, argv, options, buffer);

    if (parse.error())
        return 1;

    // show help/usage
    if (options[HELP] || argc == 0 || parse.nonOptionsCount() < 1) {
        option::printUsage(cout, usage);
        return 0;
    }

    // parameters
    const char* name = parse.nonOption(0);
    int size = 64;
    int padding = 0;
    int pt_scaleh = 2;
    int pt_scalev = 4;

    for (int i = 0; i < parse.optionsCount(); ++i) {
        option::Option& opt = buffer[i];
        switch (opt.index()) {
        /*
        case OPTIONAL:
            if (opt.arg)
                cout <<"--optional with optional argument '"<<opt.arg<<"'\n";
            else
                cout <<"--optional without the optional argument\n";
            break;
        */
        case SIZE:
            size = strtol(opt.arg, NULL, 10);
            break;
        case PADDING:
            padding = strtol(opt.arg, NULL, 10);
            break;
        case PT_SCALEH:
            pt_scaleh = strtol(opt.arg, NULL, 10);
            break;
        case PT_SCALEV:
            pt_scalev = strtol(opt.arg, NULL, 10);
            break;

        case HELP:
            // not possible, because handled further above and exits the program
        case UNKNOWN:
            // not possible because Arg::Unknown returns ARG_ILLEGAL
            // which aborts the parse with an error
            break;
        }
    }

    /*
    for (int i = 0; i < parse.nonOptionsCount(); ++i)
        cout <<"Non-option argument #"<<i<<" is "<<parse.nonOption(i)<<"\n";
    */

    switch (generateWorld(name, size, padding, pt_scaleh, pt_scalev)) {
    case ERR::NONE:
        break;
    case ERR::PATH_EXISTS:
        cerr << "error: path already exists: " << name << "\n";
        break;
    case ERR::NBT_ERROR:
        cerr << "error: some problem with NBT\n";
        break;
    case ERR::OPEN_FILE:
        cerr << "error: could not open file for writing\n";
        break;
    case ERR::WRITING_CHUNKS:
        cerr << "error: writing chunks did not go as expected\n";
        break;
    default:
        cerr << "error: unknown error\n";
        break;
    }

    delete[] options;
    delete[] buffer;

    return 0;
}

