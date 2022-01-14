#include "umba/umba.h"
#include "umba/simple_formatter.h"
#include "umba/char_writers.h"

#include "umba/debug_helpers.h"

#include <iostream>
#include <iomanip>
#include <string>
// #include <cstdio>
#include <filesystem>

#include "umba/debug_helpers.h"
#include "umba/string_plus.h"
#include "umba/program_location.h"
#include "umba/scope_exec.h"
#include "umba/macro_helpers.h"
#include "umba/macros.h"

#include "umba/time_service.h"


#include "clang.h"
#include "marty_clang_helpers.h"


umba::StdStreamCharWriter coutWriter(std::cout);
umba::StdStreamCharWriter cerrWriter(std::cerr);
umba::NulCharWriter       nulWriter;

umba::SimpleFormatter logMsg(&coutWriter);
umba::SimpleFormatter logErr(&cerrWriter);
umba::SimpleFormatter logNul(&nulWriter);

bool logWarnType   = true;
bool logGccFormat  = false;
bool logSourceInfo = false;


#include "log.h"
#include "compile_flags_parser.h"
#include "utils.h"
#include "scan_folders.h"

#include "scan_sources.h"

umba::program_location::ProgramLocation<std::string>   programLocationInfo;


#include "umba/cmd_line.h"


#include "app_ver_config.h"
#include "print_ver.h"

#include "arg_parser.h"



int main(int argc, char* argv[])
{
    umba::time_service::init();
    umba::time_service::start();

    umba::time_service::TimeTick startTick = umba::time_service::getCurTimeMs();


    using namespace umba::omanip;


    auto argsParser = umba::command_line::makeArgsParser( ArgParser()
                                                        , CommandLineOptionCollector()
                                                        , argc, argv
                                                        , umba::program_location::getProgramLocation
                                                            ( argc, argv
                                                            , false // useUserFolder = false
                                                            , "umba-pretty-headers" // overrideExeName
                                                            )
                                                        );

    // Force set CLI arguments while running under debugger
    if (umba::isDebuggerPresent())
    {
        argsParser.args.clear();
        argsParser.args.push_back("@..\\tests\\data\\umba-pretty-headers-09.rsp");
        // argsParser.args.push_back(umba::string_plus::make_string(""));
        // argsParser.args.push_back(umba::string_plus::make_string(""));
        // argsParser.args.push_back(umba::string_plus::make_string(""));
    }

    programLocationInfo = argsParser.programLocationInfo;

    // Job completed - may be, --where option found
    if (argsParser.mustExit)
        return 0;

    if (!argsParser.quet)
    {
        printNameVersion();
    }

    if (!argsParser.parseStdBuiltins())
        return 1;
    if (argsParser.mustExit)
        return 0;

    if (!argsParser.parse())
        return 1;
    if (argsParser.mustExit)
        return 0;


    #include "zz_generation.h"


    std::vector<std::string> foundFiles, excludedFiles;
    std::set<std::string>    foundExtentions;
    scanFolders(appConfig, foundFiles, excludedFiles, foundExtentions);



    if (!appConfig.getOptQuet())
    {
        if (!foundFiles.empty())
            printInfoLogSectionHeader(logMsg, "Files for Processing");

        for(const auto & name : foundFiles)
        {
            logMsg << name << endl;
        }


        if (!excludedFiles.empty())
            printInfoLogSectionHeader(logMsg, "Files Excluded from Processing");

        for(const auto & name : excludedFiles)
        {
            logMsg << name << endl;
        }


        if (!foundExtentions.empty())
            printInfoLogSectionHeader(logMsg, "Found File Extentions");

        for(const auto & ext : foundExtentions)
        {
            if (ext.empty())
                logMsg << "<EMPTY>" << endl;
            else
                logMsg << "." << ext << endl;
        }
    }

    // Phases: Init, Scaning, Processing, Generating

    if (!appConfig.getOptQuet())
    {
        printInfoLogSectionHeader(logMsg, "Scaning completed");
        auto tickDiff = umba::time_service::getCurTimeMs() - startTick;
        logMsg << "Time elapsed: " << tickDiff << "ms" << "\n";
        startTick = umba::time_service::getCurTimeMs();
    }


    for(auto compileFlagsTxtFile: generatedCompileFlagsTxtFiles)
    {
    
        // std::vector<std::string> foundFiles
        std::string errRecipientStr;
       
        auto pcdb = clang::tooling::FixedCompilationDatabase::loadFromFile(compileFlagsTxtFile, errRecipientStr);
         
        if (pcdb==0 || !errRecipientStr.empty())
        {
            marty::clang::helpers::printError( llvm::errs(), errRecipientStr );
            return -1;
        }
       
       
        auto pActionFactory = clang::tooling::newFrontendActionFactory
                                  < marty::clang::helpers::DeclFindingActionTemplate
                                      < marty::clang::helpers::DeclFinderTemplate
                                          < DeclVisitor
                                          , marty::clang::helpers::DeclFinderMode::printSourceFilename
                                          >
                                      > 
                                  >(); // std::unique_ptr
       
        auto pActionFactoryRawPtr = pActionFactory.get();

        for(auto file : foundFiles)
        {
            std::vector<std::string> inputFiles;
            inputFiles.push_back(file);

            clang::tooling::ClangTool clangTool(*pcdb, inputFiles);
           
            auto res = clangTool.run( pActionFactoryRawPtr ); // pass raw ptr

            if (res)
            {
                LOG_ERR_OPT << "Clang returns error: " << res << endl;
                return res;
            }
        }
    }

    return 0;
}


