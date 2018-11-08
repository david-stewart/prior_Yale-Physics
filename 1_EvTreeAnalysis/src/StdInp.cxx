#include "StdInp.h"
#include <iostream>

using namespace std;
// implrement class StdInp
StdInp::StdInp ( 
       int argc, 
       const char** argv_, 
       bool is_test_,
       long long int nEvents_,
       TString log_name,
       TString in_file_name
) :
        is_test  { is_test_ },
        nEvents  { nEvents_ }
{
    std::vector<TString> argv;
    for (int i{0}; i<argc; ++i) { argv.push_back(argv_[i]); }

    if (argc > 1 && (argv[1] == "--help" || argv[1] == "-h")) {
        std::cout 
            << " Enter three arguments: [1] # events [2] output log name [3] "
            << "input root file name" << std::endl;
        exit(0);
    }

    if (argc > 1) nEvents = argv[1].Atoi();
    if (argc > 2) log_name = argv[2];
    if (argc > 3) in_file_name = argv[3];
    if (argc > 4) for (int i{4}; i < argc; ++i) options.push_back(argv_[i]);

    // initialize log and file
    flog = fopen(log_name.Data(), "w");
    if (flog == NULL) cout << "fatal: couldn't open output log file \""<< log_name <<"\""<<endl;

    time(&start_time);
    fprintf(flog, " # Starting log on (local time):  %s", ctime(&start_time));

    // initalize the tchain
    fprintf(flog, " # starting TChain from file(s): %s\n", in_file_name.Data());
    chain = new TChain("tree");
    chain->Add(in_file_name.Data());
};

StdInp::~StdInp() {
    time_t end_time;
    time(&end_time);
    double f_seconds { difftime(end_time, start_time) };
    int seconds { (int) f_seconds };
    int hr { seconds/3600 };
    int min { (seconds - 3600*hr)/60 };
    int sec { seconds - 3600*hr - 60*min };
    fprintf(flog," # Ending log on (local time):   %s", ctime(&end_time));
    fprintf(flog," # Time ellapsed: %i seconds or (in hr:min:sec) %02i:%02i:%02i\n",
            (int) seconds, hr, min, sec);
    fclose(flog);
};