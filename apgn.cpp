#define APGN_VERSION "version 0.8"

#include <iostream>
#include <vector>
#include <exception>

#include <string.h>

#include "convert.hpp"
#include "envgrabber.hpp"
#include "check.hpp"
#include "path.hpp"

#define CHESS_ENGINE 0
#define WORKER_THREADS 1
#define ANALYSIS_DEPTH 2
#define OPENING_MOVE_TO_SKIP 3

int main(int argc, char* args[])
{
    // display help
    if(argc==2)
    {
        if(strcmp(args[1],"--help")==0 || strcmp(args[1],"-h")==0)
        {
            std::cout<<
                "\tThe proper format for analyzing games are either\n"
                "\tof the two commands given below:\n\n"
                
                "\tapgn /PATH/FILE.pgn COLOR\n"
                "\tapgn /PATH/FILE.pgn COLOR OPENING_SKIP DEPTH THREADS\n\n"
                
                "\t/PATH/FILE.pgn - this is the directory location with the\n"
                "\t                 filename of your pgn file\n\n"
                "\tCOLOR          - this can either be the letter W, B or A, where\n"
                "\t                 W = white, B = black, and A = both\n\n"
                "\tOPENING_SKIP   - this is the number of moves in the opening\n"
                "\t                 that the engine will not analyse\n"
                "\t                 this value should be >= 0 and < the total moves\n\n"
                "\tDEPTH          - this is how deep the chess engine will analyse\n"
                "\t                 the given pgn file, the larger the number the\n"
                "\t                 the better the analysis, but will also take\n"
                "\t                 more time to finish, this value should be >= 1\n\n"
                "\tTHREADS        - this is the number of the worker threads you want\n"
                "\t                 your engine to use, the more threads the faster\n"
                "\t                 the analysis, given that you did not exceed your CPUs\n"
                "\t                 maximum thread, but if you did a bigger thread will\n"
                "\t                 also slow down the analysis\n\n"

                "\texample : \n"
                "\tTo analyse the pgn file in pgn_samples for the player color 'white' with\n"
                "\tthe default values inside 'env.txt' as arguments use the command below :\n\n"
                "\t\tapgn "+apgn_path::get_execpath()+"/pgn_samples/first.pgn W\n\n"
                "\tTo analyse the same pgn file but for the player color black, and with\n"
                "\tcostum values of 0 moves to skip in the opening, depth of 20 and uses\n"
                "\t2 threads for analysis\n"
                "\tuse the command given below:\n\n"
                "\t\tapgn "+apgn_path::get_execpath()+"/pgn_samples/first.pgn B 0 20 2\n\n";
        }
        else if(strcmp(args[1],"--version")==0 || strcmp(args[1],"-v")==0)
        {
            std::cout<< "analyse-pgn : " << APGN_VERSION << "\n";
        }

        return 1;
    }
    // if proper arguments are received
    else if(argc==3)
    {
        std::string out_pgn, format = ".pgn", statsFileName, statWriteCache="", destinationPath;
        std::string uci_converted_ = apgn_path::get_execpath()+"/.analyzing_moves", analyse_ = apgn_path::get_execpath()+"/.analyse";

        // checks the first argument if it has a valid .pgn file extension
        if(!apgn_check::match_last(args[1],format))
        {
            // throw error if it doesn't
            throw std::invalid_argument("please provided a pgn file with an extension '.pgn' all lowercase");
        }
        else
        {
            // process the string if it has .pgn in it

            // removes the .pgn substring in the first argument and store the resulting as out_pgn
            out_pgn = apgn_check::removeFormat(args[1],format);

            // get the destination of the output pgn by removing only the name of input pgn
            destinationPath = apgn_check::removeFilename(out_pgn); // output pgn will be saved on the same location as the input pgn
            
            // add the string 'Analyzed' to the input pgn's name and make this as the output pgn's filename 
            statsFileName = out_pgn;
            out_pgn+="Analyzed.pgn";

            // this will be the filename for the output statisics text file of the input pgn
            // this is different from the analysed pgn
            statsFileName+="AnalyzedStats.txt";
        }

        // In total there are two output when analysing a pgn game

        // 1. THE OUTPUT PGN - this is a pgn file that contains the same moves as the input pgn file
        //                     but, this also additional comments for each analyzed moves, the comment
        //                     will tell if your move is a good move, blunder, excellent and etc.

        // 2. THE STATS TXT - this is a text file that will contain the statistics of the analyzed moves
        //                    here will see how many good move, blunders, mistakes and etc. you have made
        //                    this will also contain an accuracy for the total moves you analyzed


        // grabs the value in the env.txt files, and if the engines exist
        std::vector<std::string> env_var = apgn_env::grab(apgn_path::get_execpath()+"/env.txt",{
            #if defined(__linux__)
            "CHESS_ENGINE_LINUX",
            #elif defined(_WIN32)
            "CHESS_ENGINE_WINDOWS",
            #endif
            "WORKER_THREADS",
            "ANALYSIS_DEPTH",
            "OPENING_MOVE_TO_SKIP"
        });
        apgn_check::errorFileNotFound(apgn_path::get_execpath()+"/bin/engines/"+env_var[CHESS_ENGINE]);
        
        // check if the input pgn exist
        apgn_check::errorFileNotFound(args[1]);

        // checks if the 3rd argument passed to the program is valid
        if(((args[2][0]!='W' && args[2][0]!='B') && args[2][0]!='A'))
        {
            throw std::invalid_argument("invalid 3rd  argument, choices are only A,W,B");
        }

        // CONVERSION OF PGN TO UCI PGN
        apgn_convert::pgn_to_uci(args[1],uci_converted_);
        
        // ANALYSIS OF THE CONVERTED UCI PGN
        std::cout<<"analysing game(s)...\n\n";
        int depthValue = stoi(env_var[ANALYSIS_DEPTH]);
        if(depthValue>12)
        {
            std::cout<<"NOTE: env.txt's 'ANALYSIS_DEPTH' value is greater than 12\n";
            std::cout<<"because of this, analysis might take a few minutes\n";
        }
        apgn_convert::analyse_game(
            uci_converted_,
            analyse_,
            apgn_path::get_execpath()+"/bin/engines/"+env_var[CHESS_ENGINE],
            env_var[ANALYSIS_DEPTH].data(),
            env_var[WORKER_THREADS].data(),
            env_var[OPENING_MOVE_TO_SKIP].data(),
            args[2][0]
        );

        // CONVERT BACK THE ANALYZED UCI PGN INTO PROPER PGN NOTATION
        apgn_convert::uci_to_pgn(analyse_,out_pgn);
        
        // WRITE THE STATISTIC TXT FILE INTO THE SAME DESTINATION OF THE OUTPUT PGN
        std::string stats_content = "";
        std::ifstream stats_file_cache(apgn_path::get_execpath()+"/.analyzing_moves_stats");
        if(stats_file_cache.is_open())
        {
            std::string lineBuffer;
            while(getline(stats_file_cache,lineBuffer))
            {
                stats_content.append(lineBuffer+"\n");
            }
            stats_file_cache.close();
            apgn_check::deleteFile(apgn_path::get_execpath()+"/.analyzing_moves_stats");
        }
        else
        {
            throw std::runtime_error(
                "the file '" + apgn_path::get_execpath()+"/.analyzing_moves_stats' was not found"
            );
        }
        stats_content.append("\n\n");
        std::ofstream outfile;
        outfile.open(statsFileName,std::ios_base::trunc);
        outfile<<stats_content;
        outfile.close();

        // CLEAN UP THE GENERATED RESOURCES IN THE BACKGROUND 
        apgn_check::deleteFile(uci_converted_);
        apgn_check::deleteFile(analyse_);
    }
    // if arguments are not meet
    else if(argc==6)
    {
        std::string wholeCommand = "";
        for(size_t i=0; i<argc; ++i)
        {
            wholeCommand.append(args[i]);
            wholeCommand.push_back(' ');
        }

        std::string INPUT_SKIP = args[3],
                    INPUT_DEPTH = args[4],
                    INPUT_THREADS = args[5];

        std::cout<<"\npgn filepath   : "<<args[1]<<"\n";
        std::cout<<"piece color    : "<<args[2]<<"\n";
        std::cout<<"opening skip   : "<<args[3] << " | " << INPUT_SKIP <<"\n";
        std::cout<<"depth search   : "<<args[4] << " | " << INPUT_DEPTH <<"\n";
        std::cout<<"worker threads : "<<args[5] << " | " << INPUT_THREADS <<"\n\n";

        if(!apgn_check::validNumber(INPUT_SKIP))
        {
            throw std::invalid_argument(
                "the given argument '"+INPUT_SKIP+"' in the command '"+wholeCommand+"'\n"
                "\tis not a valid number.\n\n"
            );
        }

        if(!apgn_check::validNumber(INPUT_DEPTH))
        {
            throw std::invalid_argument(
                "the given argument '"+INPUT_DEPTH+"' in the command '"+wholeCommand+"'\n"
                "\tis not a valid number.\n\n"
            );
        }

        if(!apgn_check::validNumber(INPUT_THREADS))
        {
            throw std::invalid_argument(
                "the given argument '"+INPUT_THREADS+"' in the command '"+wholeCommand+"'\n"
                "\tis not a valid number.\n\n"
                
            );
        }

        int input_skip = std::stoi(INPUT_SKIP);
        int input_depth = std::stoi(INPUT_DEPTH);
        int input_threads = std::stoi(INPUT_THREADS);
                
        if(input_skip<0)
        {
            throw std::invalid_argument(
                "the given argument '"+INPUT_SKIP+"' in the command '"+wholeCommand+"'\n"
                "\tshould not be < 0.\n"
            );
        }

        std::string out_pgn, format = ".pgn", statsFileName, statWriteCache="", destinationPath;
        std::string uci_converted_ = apgn_path::get_execpath()+"/.analyzing_moves", analyse_ = apgn_path::get_execpath()+"/.analyse";

        // checks the first argument if it has a valid .pgn file extension
        if(!apgn_check::match_last(args[1],format))
        {
            // throw error if it doesn't
            throw std::invalid_argument("please provided a pgn file with an extension '.pgn' all lowercase");
        }
        else
        {
            // process the string if it has .pgn in it

            // removes the .pgn substring in the first argument and store the resulting as out_pgn
            out_pgn = apgn_check::removeFormat(args[1],format);

            // get the destination of the output pgn by removing only the name of input pgn
            destinationPath = apgn_check::removeFilename(out_pgn); // output pgn will be saved on the same location as the input pgn
            
            // add the string 'Analyzed' to the input pgn's name and make this as the output pgn's filename 
            statsFileName = out_pgn;
            out_pgn+="Analyzed.pgn";

            // this will be the filename for the output statisics text file of the input pgn
            // this is different from the analysed pgn
            statsFileName+="AnalyzedStats.txt";
        }

        // In total there are two output when analysing a pgn game

        // 1. THE OUTPUT PGN - this is a pgn file that contains the same moves as the input pgn file
        //                     but, this also additional comments for each analyzed moves, the comment
        //                     will tell if your move is a good move, blunder, excellent and etc.

        // 2. THE STATS TXT - this is a text file that will contain the statistics of the analyzed moves
        //                    here will see how many good move, blunders, mistakes and etc. you have made
        //                    this will also contain an accuracy for the total moves you analyzed


        // grabs the value in the env.txt files, and if the engines exist
        std::vector<std::string> env_var = apgn_env::grab(apgn_path::get_execpath()+"/env.txt",{
            #if defined(__linux__)
            "CHESS_ENGINE_LINUX"
            #elif defined(_WIN32)
            "CHESS_ENGINE_WINDOWS"
            #endif
        });
        apgn_check::errorFileNotFound(apgn_path::get_execpath()+"/bin/engines/"+env_var[CHESS_ENGINE]);
        
        // check if the input pgn exist
        apgn_check::errorFileNotFound(args[1]);

        // checks if the 3rd argument passed to the program is valid
        if(((args[2][0]!='W' && args[2][0]!='B') && args[2][0]!='A'))
        {
            throw std::invalid_argument("invalid 3rd  argument, choices are only A,W,B");
        }

        // CONVERSION OF PGN TO UCI PGN
        apgn_convert::pgn_to_uci(args[1],uci_converted_);
        
        // ANALYSIS OF THE CONVERTED UCI PGN
        std::cout<<"analysing game(s)...\n\n";
        int depthValue = stoi(INPUT_DEPTH);
        if(depthValue>12)
        {
            std::cout<<"NOTE: env.txt's 'ANALYSIS_DEPTH' value is greater than 12\n";
            std::cout<<"because of this, analysis might take a few minutes\n";
        }

        std::cout<<"running uci-analyse : analyse\n";
        apgn_convert::analyse_game(
            uci_converted_,
            analyse_,
            apgn_path::get_execpath()+"/bin/engines/"+env_var[CHESS_ENGINE],
            INPUT_DEPTH.data(),
            INPUT_THREADS.data(),
            INPUT_SKIP.data(),
            args[2][0]
        );
        std::cout<<"done...\n";

        // CONVERT BACK THE ANALYZED UCI PGN INTO PROPER PGN NOTATION
        apgn_convert::uci_to_pgn(analyse_,out_pgn);
        
        // WRITE THE STATISTIC TXT FILE INTO THE SAME DESTINATION OF THE OUTPUT PGN
        std::string stats_content = "";
        std::ifstream stats_file_cache(apgn_path::get_execpath()+"/.analyzing_moves_stats");
        if(stats_file_cache.is_open())
        {
            std::string lineBuffer;
            while(getline(stats_file_cache,lineBuffer))
            {
                stats_content.append(lineBuffer+"\n");
            }
            stats_file_cache.close();
            apgn_check::deleteFile(apgn_path::get_execpath()+"/.analyzing_moves_stats");
        }
        else
        {
            throw std::runtime_error(
                "the file '" + apgn_path::get_execpath()+"/.analyzing_moves_stats' was not found"
            );
        }
        stats_content.append("\n\n");
        std::ofstream outfile;
        outfile.open(statsFileName,std::ios_base::trunc);
        outfile<<stats_content;
        outfile.close();

        // CLEAN UP THE GENERATED RESOURCES IN THE BACKGROUND 
        apgn_check::deleteFile(uci_converted_);
        apgn_check::deleteFile(analyse_);
    }
    else
    {
        std::cout<< "apgn > analyse-pgn : a pgn chess game analyser\n"
                    "type the command : \n\napgn -h\n\nor\n\napgn --help\n\n"
                    "to display more informations on how to use this program, or \n"
                    "you can visit my github https://github.com/mrdcvlsc/analyse-pgn\n";
        return 1;
    }

    std::cout <<"analysis done ...\n";

    return 0;
}
