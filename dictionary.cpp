#include <chrono>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string.h>

using namespace std;

#define DICTIONARY "rockyou.txt"
#define NUM_PASSWORDS 14344391 /* the number of lines in the rockyou dataset */
#define TERMINAL_WIDTH 80 /* used for progress bar, assume default width */

/* 
* rules to apply for finding variations (each node can be assigned a rule):
* 1. adding numbers
*   - password -> 1password -> password1 -> password2 -> ...
* 2. reverse spelling
*  - password -> drowssap
* 3. repeating password twice
*  - password -> passwordpassword
* 4. changing casing
*  - password -> Password -> PASSWORD -> paSSword -> ...
* 5. leet spellings
*  - password -> p4ssw0rd -> p@ssword -> ...
* 7. deleting characters
*  - password -> passwrd -> assword -> ...
* 8. adding all prefixes and suffixes
*  - password -> apassword -> passwordx -> xpasswordy -> ...
*/

/*
 * tries to find the correct password by appending numbers to the base password
 * return values:
 *  - 0: password was NOT found
 *  - 1: password found
 */
int
find_password_add_int(const string start, const int max_len, const string password)
{
    size_t len = start.length();

    if ((int)len > max_len) {
        return 0;
    }

    for (size_t i = 0; i <= len; i++) {
        for (int j = 0; j < 10; j++) {
            string check_pass = start.substr(0, i);
            check_pass += to_string(j);
            check_pass += start.substr(i, len - i);

            if (check_pass == password) {
                cout << endl << "Your password is: " << check_pass << endl;
                return 1;
            }

            find_password_add_int(check_pass, max_len, password);
        }
    }

   return 0;
}

/*
 * tries to find the correct password by reversing a common password. 
 * return values:
 *  - 0: password was NOT found
 *  - 1: password found
 */
int
find_password_reverse(const string start, const string password)
{
    string reversed = string(start.rbegin(), start.rend());

    if (reversed == password) {
        return 1;
    }
    return 0;
}

int
find_password(const string password, const int max_len)
{
    ifstream common_passwords(DICTIONARY);
    string curr_pass;
    int counter = 0;
    /* number of passwords needed to print each '#' to keep track of progress */
    int progress = NUM_PASSWORDS / TERMINAL_WIDTH;

    cout << "Looking up password... " << endl;

    while (common_passwords >> curr_pass) {
        if (curr_pass == password) {
            return 1;
        }

        if (++counter % progress == 0) {
            if (counter ==  progress) {
                cout << "Dictionary attack progress:" << endl;
            }
            cout << unitbuf;
            cout << "#";
        }
    }

    cout << ">" << endl;

    /* BRUTE FORCE attack starts here */
    common_passwords.clear();
    common_passwords.seekg(0, ios::beg);
    cout << "Your password is not that common..." << endl;
    cout << "Starting a brute force attack" << endl;
    counter = 0;

    while (common_passwords >> curr_pass) {
        /* each if statement will be assigned to a node */
        if (find_password_add_int(curr_pass, max_len, password)) {
            cout << endl << "Found the password by adding numbers!" << endl;
            return 1;
        }

        if (find_password_reverse(curr_pass, password)) {
            cout << endl << "Found the password by reversing a common password!" << endl;
            return 1;
        }

        if (++counter % progress == 0) {
            cout << unitbuf;
            cout << "#";
        }
    }

    return 0;
}

int
main(int argc, char* argv[])
{
    if (argc < 2 || argc > 3) {
        cerr << "Usage: " << argv[0] << " <password> [maximum length]" << endl;
        return 1;
    }

    string password = argv[1];
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
    if (!find_password(password, max_len)) {
        cout << "Nice password, we couldn't crack it!" << endl;
    }

    auto end = chrono::high_resolution_clock::now();
    double time_taken = 
      chrono::duration_cast<chrono::nanoseconds>(end - start).count() * 1e-9;

    cout << "Password lookup completed in " << time_taken << " seconds." << endl;

    return 0;
}
