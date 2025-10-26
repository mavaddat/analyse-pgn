#ifndef APGN_CONVERT_HPP
#define APGN_CONVERT_HPP

#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <stdlib.h>
#include <sys/types.h>

#include <string.h>
#include <fstream>

#include "apgnFileSys.hpp"

#if defined(__linux__)
#include <unistd.h>
#include <wait.h>
#else
#include "winProcRun.hpp"
#endif

namespace apgn_convert
{
    namespace COLOR
    {
        const char ALL = 'A';
        const char WHITE = 'W';
        const char BLACK = 'B';
    }
    
    int run_subprog(std::string program, char *const args[])
    {
        #if defined(__linux__)
        int status;

        if(!std::filesystem::exists(program))
            throw std::logic_error("'"+std::string(program)+"' file not found");

        int pid = fork();

        if(pid==-1) throw std::runtime_error("error forking subprocess");

        if(pid==0) execv(program.data(),args);
        else       waitpid(pid, &status, 0);

        return status;
        #elif defined(_WIN32)
        std::vector<std::string> cmdArguments;

        size_t ARGI = 0;
        while(args[ARGI]!=NULL)
        {
            cmdArguments.push_back(args[ARGI]);
            ARGI++;
        }
        apgn::runAndGrabOutput(program.c_str(),cmdArguments);
        return 0;
        #endif
    }

    void uci_to_pgn(const std::string& input, const std::string output)
    {
        std::string PGN_EXTRACT = "pgn-extract", FLG1 = "-WsanPNBRQK", FLG2 = "--output";
        char *const args[] = {PGN_EXTRACT.data(),FLG1.data(),FLG2.data(),(char* const)output.c_str(),(char* const)input.c_str(),NULL};

        #if defined(__linux__)
        run_subprog((std::filesystem::path(apgnFileSys::getExecpath()) / "bin" / "pgn-extract").string(),args);
        #elif defined(_WIN32)
        run_subprog((std::filesystem::path(apgnFileSys::getExecpath()) / "bin" / "pgn-extract" / "pgn-extract.exe").string(),args);
        #endif
    }

    void pgn_to_uci(const std::string& input, const std::string output)
    {
        std::string PGN_EXTRACT = "pgn-extract", FLG1 = "-Wuci", FLG2 = "--output";
        char *const args[] = {PGN_EXTRACT.data(),FLG1.data(),FLG2.data(),(char* const)output.c_str(),(char* const)input.c_str(),NULL};

        #if defined(__linux__)
        run_subprog((std::filesystem::path(apgnFileSys::getExecpath()) / "bin" / "pgn-extract").string(),args);
        #elif defined(_WIN32)
        run_subprog((std::filesystem::path(apgnFileSys::getExecpath()) / "bin" / "pgn-extract" / "pgn-extract.exe").string(),args);
        #endif
    }

    void analyse_game(
        std::string input,
        std::string output,
        std::string engine,
        char* search_depth,
        char* threads,
        char* opening_move_skips,
        char* moves_until,
        char apgn_COLOR
    )   
    {
        std::string analyse_ = "analyse", engine_ = "--engine", searchd_ = "--searchdepth", bookd_ = "--bookdepth", setopt_ = "--setoption",
                    movesuntil_ = "--movesuntil", threads_ = "Threads", annotate_ = "--annotatePGN", white_ = "--whiteonly", black_ = "--blackonly";

        #if defined(__linux__)
        char *const args[] = {
        #else
        std::vector<std::string> args = {
        #endif
            analyse_.data(),
            engine_.data(),engine.data(),
            searchd_.data(),search_depth,
            bookd_.data(),opening_move_skips,
            movesuntil_.data(),moves_until,
            setopt_.data(),threads_.data(),threads,
            #if defined(__linux__)
            annotate_.data(),input.data(),
            NULL
            #else
            annotate_.data(),input.data()
            #endif
        };

        #if defined(__linux__)
        char *const args_white[] = {
        #else
        std::vector<std::string> args_white = {
        #endif
            analyse_.data(),
            engine_.data(),engine.data(),
            white_.data(),
            searchd_.data(),search_depth,
            bookd_.data(),opening_move_skips,
            movesuntil_.data(),moves_until,
            setopt_.data(),threads_.data(),threads,
            #if defined(__linux__)
            annotate_.data(),input.data(),
            NULL
            #else
            annotate_.data(),input.data()
            #endif
        };

        #if defined(__linux__)
        char *const args_black[] = {
        #else
        std::vector<std::string> args_black = {
        #endif
            analyse_.data(),
            engine_.data(),engine.data(),
            black_.data(),
            searchd_.data(),search_depth,
            bookd_.data(),opening_move_skips,
            movesuntil_.data(),moves_until,
            setopt_.data(),threads_.data(),threads,
            #if defined(__linux__)
            annotate_.data(),input.data(),
            NULL
            #else
            annotate_.data(),input.data()
            #endif
        };

        #if defined(__linux__)
        std::string result_pgn = "";
        char std_output[4096+1];
        memset(std_output,0,4096);

        int reader[2];
        if(pipe(reader)==-1) std::runtime_error("error creating pipe for stdout reading in analyse_game()");

        int pid = fork();

        if(pid==-1) std::runtime_error("error forking parent process, unable to create a child process in analyse_game()");

        std::string analyse_executable_path = apgnFileSys::getExecpath()+"/bin/analyse";

        if(pid==0)
        {
            dup2(reader[1], STDOUT_FILENO);
            close(reader[0]);
            switch (apgn_COLOR)
            {
            case COLOR::ALL:
                execv(analyse_executable_path.data(),args);
                break;
            case COLOR::WHITE:
                execv(analyse_executable_path.data(),args_white);
                break;
            case COLOR::BLACK:
                execv(analyse_executable_path.data(),args_black);
                break;
            default:
                throw std::runtime_error("something happend - error in convert.hpp");
                break;
            }
            close(reader[1]);
        }
        else
        {
            close(reader[1]);
            int nbytes = 0;
            while(read(reader[0], std_output, sizeof(std_output)) != 0)
            {
                result_pgn+=std_output;
                memset(std_output,0,4096);
            }
            close(reader[0]);
            wait(NULL);
        }
        #else
        std::string analyse_executable_path = "";
        
        #if defined(__linux__)
        analyse_executable_path = (std::filesystem::path(apgnFileSys::getExecpath()) / "bin" / "analyse").string();
        #elif defined(_WIN32)
        analyse_executable_path = (std::filesystem::path(apgnFileSys::getExecpath()) / "bin" / "analyse" / "analyse.exe").string();
        #endif

        std::string result_pgn;

        switch (apgn_COLOR)
        {
            case COLOR::ALL:
                result_pgn = apgn::runAndGrabOutput(analyse_executable_path.data(),args);
                break;
            case COLOR::WHITE:
                result_pgn = apgn::runAndGrabOutput(analyse_executable_path.data(),args_white);
                break;
            case COLOR::BLACK:
                result_pgn = apgn::runAndGrabOutput(analyse_executable_path.data(),args_black);
                break;
            default:
                throw std::runtime_error("something happend - error in convert.hpp");
                break;
        }
        #endif

        // check if file exist
        std::ifstream readf;
        readf.open(output);

        if(readf.fail()){
            readf.close();

            std::ofstream file;
            file.open(output);
            file.close();
        }

        std::ofstream outfile;
        outfile.open(output,std::ios_base::trunc);
        outfile<<result_pgn;
        outfile.close();
    }
}

#endif