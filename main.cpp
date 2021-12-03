#include <iostream>
#include <vector>
#include <cstring>
#include <fstream>

#include "convert.hpp"
#include "apgnFileSys.hpp"

#define DEBUG_PRINT(MSG) std::cerr << MSG << "\n"

#define ASSERT_MISSING_FLAGVALUE(INDEX,ARG_COUNT,ARGUMENT) \
if(INDEX+1>=ARG_COUNT) \
{ \
    std::cerr << \
        "INVALID ARGUMENT:\n\n" \
        "\tThe given flag '" << ARGUMENT << "' does not have a value\n\n"; \
    exit(1); \
}

#define ASSERT_INVALID(WHAT,PARAM_FLAG,VALUE) \
{ \
    std::cerr << \
        "Argument Error: " << PARAM_FLAG << " " << VALUE << "\n\n" \
        "\tDetected invalid " << WHAT << " given\n" \
        "\tin the parameter flag " << PARAM_FLAG << "\n"; \
    exit(1); \
}

#define APGN_VERSION "version 1.0"
#define SIZE_T_32BIT 4

#define FLAG_VERSION "--version"
#define FLAG_HELP "--help"
#define FLAG_ANALYSE "-analyse"

#define ANALYSE_ENGINE "-engine"
#define ANALYSE_THREADS "-threads"
#define ANALYSE_DEPTH "-depth"
#define ANALYSE_COLOR "-color"
#define ANALYSE_OPENNING_SKIP "-oskip"

#define DEFAULT_THREAD 1
#define DEFAULT_DEPTH 11
#define DEFAULT_COLOR 'A'
#define DEAFULT_OPENNING_MOVE_SKIP 4

static std::string PGN_EXT = ".pgn";

std::string DEFAULT_ENGINE()
{
    #if defined(__linux__)
    return apgnFileSys::getExecpath()+"/bin/engines/stockfish11_x64";
    #elif defined(_WIN64)
    return apgnFileSys::getExecpath()+"/bin/engines/stockfish11_x64";
    #elif defined(_WIN32)
    return apgnFileSys::getExecpath()+"/bin/engines/stockfish11_x86";
    #endif
}

bool isNumber(const std::string& input)
{
    std::string::const_iterator i = input.begin();
    while (i != input.end() && std::isdigit(*i)) ++i;
    return !input.empty() && i == input.end();
}

bool isEngine(const std::string& input)
{
    std::FILE *filereader = std::fopen(input.c_str(), "rb");
    if(!filereader)
    {
        throw std::invalid_argument(
            "The given engine '"+input+"' was not found"
        );
    }
    // I still don't know a way on how to validate if a file is a UCI chess engine,
    // so for now I'll just always return this to true
    return true;
}

/// checks if the filename has the proper pgn extension
bool isPGN(const std::string& input)
{
    if(input.size()<PGN_EXT.size())
    {
        return false;
    }

    std::string file_extension = input.substr(input.size()-PGN_EXT.size(),PGN_EXT.size());
    if(file_extension==PGN_EXT)
    {
        std::FILE *filereader = std::fopen(input.c_str(), "rb");
        if(!filereader)
        {
            std::cout << "The pgn file '"+input+"' was not found. (skipped in analysis)\n";
            return false;
        }
        return true;
    }
    
    return false;
}

/// color should just ba a one character string, options are [b,B,w,W,a,A]
bool isValidColor(const std::string& input) {
    if(input.size()==1)
    {
        switch (input[0])
        {
            case 'B': return true;
            case 'W': return true;
            case 'A': return true;
        }
    }
    return false;
}

int main(int argc, char* argv[])
{
    if(argc==2)
    {
        if(strcmp(argv[1],FLAG_HELP)==0)
        {
            std::cerr<<
                "    HELP MENU\n\n"

                "    Modifier Flags:\n\n"
                
                "\t" << ANALYSE_ENGINE << " [PATH]  - the directory location with the\n"
                "\t                  filename of your pgn file\n\n"
                "\t" << ANALYSE_COLOR << " [A,W,B]  - select one letter from A, W or B, where\n"
                "\t                  W = white, B = black, and A = both\n\n"
                "\t" << ANALYSE_OPENNING_SKIP << " [+I>=0]  - this is the number of moves in the opening\n"
                "\t                  that the engine will not analyse\n"
                "\t                  this value should be >= 0 and < the total moves\n\n"
                "\t" << ANALYSE_DEPTH << " [+I>0]   - this is how deep the chess engine will analyse\n"
                "\t                  the given pgn file, the larger the number the\n"
                "\t                  the better the analysis, but will also take\n"
                "\t                  more time to finish, this value should be >= 1\n\n"
                "\t" << ANALYSE_THREADS << " [+I>0] - this is the number of the worker threads you want\n"
                "\t                  your engine to use, the more threads the faster\n"
                "\t                  the analysis, given that you did not exceed your CPUs\n"
                "\t                  maximum thread, but if you did a bigger thread will\n"
                "\t                  also slow down the analysis\n\n"

                "\tif a flags is not specified, the default value of that flag will be used,\n"
                "\tbelow are the default value of each flags\n\n"
                "\t    engine  - " << DEFAULT_ENGINE() << "\n"
                "\t    color   - " << DEFAULT_COLOR << "\n"
                "\t    oskip   - " << DEAFULT_OPENNING_MOVE_SKIP << "\n"
                "\t    depth   - " << DEFAULT_DEPTH << "\n"
                "\t    threads - " << DEFAULT_THREAD << "\n"

                "\n\n\tExample Using Default Values:\n\n"
                "\t\tapgn myGame1.pgn myGame2.pgn\n\n\n"

                "\tExample Using Costum Values:\n\n"
                "\t    apgn -color B -threads 4 myGame1.pgn myGame2.pgn\n\n\t    another one\n\n"
                "\t    apgn -engine C:/User/Downloads/Stockfish8.exe -depth 25 myGame1.pgn\n\n"
                "\t    NOTE: this examples will use the costum values for the specified flags\n"
                "\t          and use the default values for the unspecified flags.\n";

            return 0;
        }
        else if(strcmp(argv[1],FLAG_VERSION)==0)
        {
            int executable_bit = sizeof(size_t)==SIZE_T_32BIT ? 32 : 64;
            std::cerr << "apgn " << executable_bit << "-bit : " << APGN_VERSION << "\n";
            return 0;
        }
    }

    std::string engine = DEFAULT_ENGINE();
    int thread = DEFAULT_THREAD;
    int depth  = DEFAULT_DEPTH;
    char color = DEFAULT_COLOR;
    int openning_move_skip = DEAFULT_OPENNING_MOVE_SKIP;

    std::vector<std::string> ARGUMENTS;
    std::vector<std::string> FILENAME;
    std::vector<std::string> PGN_GAMES;

    for(int i=1; i<argc; ++i) ARGUMENTS.push_back(argv[i]);

    for(size_t i=0; i<ARGUMENTS.size(); ++i)
    {
        if(ARGUMENTS[i]==ANALYSE_ENGINE)
        {
            DEBUG_PRINT("ENGINE FLAG DETECTED");
            ASSERT_MISSING_FLAGVALUE(i,ARGUMENTS.size(),ARGUMENTS[i]);
            if(isEngine(ARGUMENTS[i]))
            {
                engine = ARGUMENTS[i];
            }
            else ASSERT_INVALID("engine", ARGUMENTS[i], ARGUMENTS[i+1]);
        }
        else if(ARGUMENTS[i]==ANALYSE_THREADS)
        {
            DEBUG_PRINT("THREAD FLAG DETECTED");
            ASSERT_MISSING_FLAGVALUE(i,ARGUMENTS.size(),ARGUMENTS[i]);
            if(isNumber(ARGUMENTS[i+1]))
            {
                thread = std::atoi(ARGUMENTS[i+1].data());
            }
            else ASSERT_INVALID("thread value", ARGUMENTS[i], ARGUMENTS[i+1]);
        }
        else if(ARGUMENTS[i]==ANALYSE_DEPTH)
        {
            DEBUG_PRINT("DEPTH FLAG DETECTED");
            ASSERT_MISSING_FLAGVALUE(i,ARGUMENTS.size(),ARGUMENTS[i]);
            if(isNumber(ARGUMENTS[i+1]))
            {
                depth = std::atoi(ARGUMENTS[i+1].data());
            }
            else ASSERT_INVALID("depth value", ARGUMENTS[i], ARGUMENTS[i+1]);
        }
        else if(ARGUMENTS[i]==ANALYSE_COLOR)
        {
            DEBUG_PRINT("COLOR FLAG DETECTED");
            ASSERT_MISSING_FLAGVALUE(i,ARGUMENTS.size(),ARGUMENTS[i]);
            if(isValidColor(ARGUMENTS[i+1]))
            {
                color = ARGUMENTS[i+1][0];
            }
            else ASSERT_INVALID("color", ARGUMENTS[i], ARGUMENTS[i+1]);
        }
        else if(ARGUMENTS[i]==ANALYSE_OPENNING_SKIP)
        {
            DEBUG_PRINT("OPENNING SKIP FLAG DETECTED");
            ASSERT_MISSING_FLAGVALUE(i,ARGUMENTS.size(),ARGUMENTS[i]);
            if(isNumber(ARGUMENTS[i+1]))
            {
                openning_move_skip = std::atoi(ARGUMENTS[i+1].data());
            }
            else ASSERT_INVALID("openning skip counts", ARGUMENTS[i], ARGUMENTS[i+1]);
        }
        else if(isPGN(ARGUMENTS[i]))
        {
            DEBUG_PRINT("A PGN FILE IS DETECTED");
            std::string CURRENT_PGN_FILE(ARGUMENTS[i]);
            std::string BASE_FILENAME = CURRENT_PGN_FILE.substr(0,CURRENT_PGN_FILE.size()-PGN_EXT.size());
            std::string ANALYZED_PGN = BASE_FILENAME+".analyzed.pgn";
            std::string ANALYZED_STATS = BASE_FILENAME+".stats.txt";
            
            FILENAME.push_back(BASE_FILENAME);
            PGN_GAMES.push_back(CURRENT_PGN_FILE);
        }
        else
        {
            std::cout << "Ignoring the unknown parameter '" << ARGUMENTS[i] << "\n";
        }
    }

    // display analysis information
    std::cout <<
        "Engine  : " << engine << "\n"
        "Threads : " << thread << "\n"
        "Depth   : " << depth << "\n"
        "Color   : " << color << "\n"
        "Openning Moves to Skip : " << openning_move_skip << "\n\n";

    if(depth>12)
    {
        std::cout <<
            "NOTE: depth is set greater than 12!!!\n"
            "      analysis might take longer...\n\n";
    }
    
    // start analysing games
    for(size_t i=0; i<PGN_GAMES.size(); ++i)
    {
        std::cout << "Analysing " << PGN_GAMES[i] << " please wait...\n";

        apgn_convert::pgn_to_uci(PGN_GAMES[i],FILENAME[i]);

        /* clear the stats file if it exists */ {
            std::ofstream existing_stat_file;
            existing_stat_file.open(FILENAME[i]+".stats.txt",std::ios_base::out | std::ios_base::trunc);
            existing_stat_file.close();
        }

        apgn_convert::analyse_game(
            FILENAME[i],
            FILENAME[i]+".analyzed",
            engine,
            std::to_string(depth).data(),
            std::to_string(thread).data(),
            std::to_string(openning_move_skip).data(),
            color
        );
        apgn_convert::uci_to_pgn(FILENAME[i]+".analyzed",FILENAME[i]+".analyzed.pgn");

        // clean temporary files
        apgnFileSys::deleteFile(FILENAME[i]);
        apgnFileSys::deleteFile(FILENAME[i]+".analyzed");
    }

}