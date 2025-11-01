#include <chrono>
#include <iostream>
#include <fstream>
#include <string.h>

using namespace std;

#define DICTIONARY "data/rockyou.txt"
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
find_password_add_int(const string start, const size_t len, const string password)
{
    for (size_t i = 0; i <= len; i++) {
        for (int j = 0; j < 10; j++) {
            string check_pass = start.substr(0, i);
            check_pass += to_string(j);
            check_pass += start.substr(i, len - i);

            if (check_pass == password) {
                cout << endl << "Your password is: " << check_pass << endl;
                return 1;
            }
        }
    }

   return 0;
}

int
find_password(const string password)
{
    ifstream common_passwords(DICTIONARY);
    string curr_pass;
    int counter = 0;
    /* number of passwords needed to print each '#' to keep track of progress */
    int progress = NUM_PASSWORDS / TERMINAL_WIDTH;

    cout << "Looking up password... " << endl;
    cout << "Dictionary attack progress:" << endl;

    while (common_passwords >> curr_pass) {
        if (curr_pass == password) {
            return 1;
        }

        if (++counter % progress == 0) {
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
        if (find_password_add_int(curr_pass, curr_pass.length(), password)) {
            cout << endl << "Found the password by adding numbers!" << endl;
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
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <password>" << endl;
        return 1;
    }

    // unordered_set<string> dictionary;
    string password = argv[1];

    // cout << "Creating dictionary..." << endl;
    // time(&begin);
    // dictionary = create_dictionary();
    // time(&end);

    // cout << "Dictionary created in " << difftime(end, begin) << " seconds." << endl;

    // cout << "Dictionary contains " << dictionary.size() << " passwords." << endl;

    // time(&begin);
    auto start = chrono::high_resolution_clock::now();
    if (!find_password(password)) {
        cout << "Nice password, we couldn't crack it!" << endl;
    }

    // time(&end);
    auto end = chrono::high_resolution_clock::now();
    double time_taken = 
      chrono::duration_cast<chrono::nanoseconds>(end - start).count() * 1e-9;

    cout << "Password lookup completed in " << time_taken << " seconds." << endl;

    return 0;
}
