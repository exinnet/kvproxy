#include "util.h"

string get_cwd(string bin_path){
    string cwd;
    int position = bin_path.find_last_of("/");
    if (position == string::npos){
        cout << "error: the command should have seperator / " << endl;
        exit(EXIT_FAILURE);
    }
    cwd = bin_path.substr(0,position) + "/";
    return cwd;
}
