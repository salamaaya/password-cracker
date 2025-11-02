#include <algorithm>
#include <chrono>
#include <iostream>
#include <fstream>
#include <mpi.h>
#include <sstream>
#include <string.h>
#include <unordered_set>

using namespace std;

#define DICTIONARY "rockyou.txt"
#define NUM_PASSWORDS 14344391 /* the number of lines in the rockyou dataset */
#define TERMINAL_WIDTH 80      /* used for progress bar, assume default width */

unordered_set<string> tried_pwds; /* used for memoization */

/*
 * tries to find the correct pwd by pre/appending numbers to the base 
 * password (start)
 * return values:
 *  - 0: pwd was NOT found
 *  - 1: pwd found
 */
int
find_pwd_add_int(const string start, const int max_len, const string pwd)
{
    size_t len = start.length();

    if (start == pwd) {
        cout << endl
             << "Your password is: " << start << endl;
        return 1;
    }

    if ((int)len >= max_len) {
        return 0;
    }

    for (size_t i = 0; i < len; i++) {
        for (int j = 0; j < 10; j++) {
            string prepend = to_string(j);
            string append = start;

            prepend += start;
            append += to_string(j);

            /* memoize! */
            if (tried_pwds.find(prepend) != tried_pwds.end()) {
                return 0;
            } else {
                tried_pwds.insert(prepend);
            }

            if (tried_pwds.find(append) != tried_pwds.end()) {
                return 0;
            } else {
                tried_pwds.insert(append);
            }

            if (find_pwd_add_int(append, max_len, pwd)
                || find_pwd_add_int(prepend, max_len, pwd)) {
                    return 1;
            }
        }
    }

    return 0;
}

/*
 * tries to find the correct pwd by deleting characters from the base
 * password (start) and stops when the string is empty
 * return values:
 *  - 0: pwd was NOT found
 *  - 1: pwd found
 */
int
find_pwd_remove(const string start, const string pwd)
{
    size_t len = start.length();

    if (start == pwd) {
        cout << endl << "Your password is: " << start << endl;
        return 1;
    }

    if (start == "") {
        return 0;
    }

    for (size_t i = 0; i < len; i++) {
        string new_pwd = start;
        new_pwd.erase(i, 1);

        if (find_pwd_remove(new_pwd, pwd)) {
            return 1;
        }
    }

    return 0;
}

/*
 * tries to find the correct pwd by changing the casing on the base
 * password (start) and stops
 * return values:
 *  - 0: pwd was NOT found
 *  - 1: pwd found
 */
int
find_pwd_casing(const string start, const string end, const string pwd)
{
    size_t len = start.length();

    if (start == pwd) {
        cout << endl << "Your password is: " << start << endl;
        return 1;
    }

    if (start == end) {
        return 0;
    }

    for (size_t i = 0; i < len; i++) {
        char curr = start[i];
        string new_pwd = start.substr(0, i);

        if (!isalpha(curr)) {
            continue; /* not a letter, just ignore */
        }

        if (isupper(curr)) {
            new_pwd += tolower(curr);
        } else {
            new_pwd += toupper(curr);
        }

        if (i < len - 1) {
            new_pwd += start.substr(i + 1, len - i - 1);
        }

        /* memoize! */
        if (tried_pwds.find(new_pwd) != tried_pwds.end()) {
            return 0;
        } else {
            tried_pwds.insert(new_pwd);
        }
        
        if (find_pwd_casing(new_pwd, end, pwd)) {
            return 1;
        }
    }

    return 0;
}

/*
 * tries to find the correct pwd by reversing a common pwd.
 * return values:
 *  - 0: pwd was NOT found
 *  - 1: pwd found
 */
int
find_pwd_reverse(const string start, const string pwd)
{
    string reversed = string(start.rbegin(), start.rend());

    if (reversed == pwd) {
        return 1;
    }
    return 0;
}

/*
 * tries to find the correct pwd by repeating a common pwd twice.
 * return values:
 *  - 0: pwd was NOT found
 *  - 1: pwd found
 */
int
find_pwd_repeat(const string start, const int max_len, const string pwd)
{
    size_t len = start.length();
    if ((int)len * 2 > max_len) {
        return 0;
    }

    string repeated = pwd + pwd;
    if (repeated == pwd) {
        return 1;
    }
    return 0;
}


/* tries to find the correct password by applying brute force variations 
 * such as the following:
 *  rules to apply for finding variations (each node can be assigned a rule):
 *  1. adding numbers
 *     Example: pwd -> 1pwd -> pwd1 -> pwd2 -> ...
 *  2. reverse spelling
 *      Example: pwd -> dwp
 *  3. repeating pwd twice
 *      Example: pwd -> pwdpwd
 *  4. changing casing
 *      Example: pwd -> Pwd -> PWD -> pWD -> ...
 *  5. deleting characters
 *      Example: password -> passwrd -> assword -> ...
 */
void
brute_force(const string pwd, const int max_len, int rank) {
    cout << "Rank " << rank << ": starting a brute force attack..." << endl;

    unordered_set<string> lowered_pwds; /* used to void repeating casing */
    ifstream common_pwd(DICTIONARY);
    string curr_pass;

    while (common_pwd >> curr_pass) {
        if (rank == 1) {
            if (find_pwd_add_int(curr_pass, max_len, pwd)) {
                cout << endl
                    << "Rank 1 found the password by adding numbers to '"
                    << curr_pass << "'." << endl;
                MPI_Abort(MPI_COMM_WORLD, 0);
            }
            tried_pwds.clear(); /* reset after every entry */
        } else if (rank == 2) {
            if (find_pwd_reverse(curr_pass, pwd)) {
                cout << endl
                    << "Rank 2 found the password by reversing '"
                    << curr_pass << "'." << endl;
                MPI_Abort(MPI_COMM_WORLD, 0);
            }
        } else if (rank == 3) {
            if (find_pwd_repeat(curr_pass, max_len, pwd)) {
                cout << endl
                    << "Rank 3 found the password by repeating '"
                    << curr_pass << "'." << endl;
                MPI_Abort(MPI_COMM_WORLD, 0);
            }
        } else if (rank == 4) {
            string lower = curr_pass;
            transform(curr_pass.begin(), curr_pass.end(), lower.begin(), ::tolower);
            string upper = curr_pass;
            transform(curr_pass.begin(), curr_pass.end(), upper.begin(), ::toupper);
            
            if (lowered_pwds.find(lower) == lowered_pwds.end()) {
                if (find_pwd_casing(lower, upper, pwd)) {
                    cout << endl 
                        << "Rank 4 found the password by changing casing on '"
                        << curr_pass << "'." << endl;
                    MPI_Abort(MPI_COMM_WORLD, 0);
                }
                lowered_pwds.insert(lower);
            }

            tried_pwds.clear(); /* reset after every entry */
        } else if (find_pwd_remove(curr_pass, pwd)) {
            cout << endl
                << "Rank 5 found the password by removing characters from '"
                << curr_pass << "'." << endl;
            MPI_Abort(MPI_COMM_WORLD, 0);
        }
    }
}

/*
 * tries to find the right password by looking it up in the rockyou dictionary
 */
void
find_pwd(const string pwd, const int max_len)
{
    ifstream common_pwd(DICTIONARY);
    string curr_pass;
    int counter = 0;
    /* number of passwords needed to print each '#' to keep track of progress */
    int progress = NUM_PASSWORDS / TERMINAL_WIDTH;

    cout << "Rank 0 is looking up the password in the dictionary... " << endl;

    while (common_pwd >> curr_pass) {
        if (curr_pass == pwd) {
            cout << endl << "Your password is: " << curr_pass << endl;
            MPI_Abort(MPI_COMM_WORLD, 0);
        }

        if (++counter % progress == 0) {
            if (counter == progress) {
                cout << "Dictionary attack progress:" << endl;
            }
            cout << unitbuf;
            cout << "#";
        }
    }

    cout << ">" << endl;
}

int
main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);

    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    int my_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    if (argc < 2 || argc > 3) {
        cerr << "Usage: " << argv[0] << " <password> [maximum length]" << endl;
        return 1;
    }

    string pwd = argv[1];
    int max_len = 128;
    if (argc == 3) {
        istringstream iss;
        iss.str(argv[2]);

        if (!(iss >> max_len)) {
            cerr << "Error: Second argument must be a size_t." << endl;
            return 1;
        }
        if (max_len < 0) {
            cerr << "Error: Second argument must be greater than 0." << endl;
            return 1;
        }
    }

    auto start = chrono::high_resolution_clock::now();
    if (my_rank == 0) {
        find_pwd(pwd, max_len);
    } else {
        brute_force(pwd, max_len, my_rank);
    }

    cout << "Rank " << my_rank << ": could not find the password." << endl; 

    auto end = chrono::high_resolution_clock::now();
    double time_taken =
        chrono::duration_cast<chrono::nanoseconds>(end - start).count() * 1e-9;

    cout << "Rank " << my_rank << ": password lookup completed in "
        << time_taken << " seconds." << endl;

    MPI_Finalize();
    return 0;
}
