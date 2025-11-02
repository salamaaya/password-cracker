#include <chrono>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string.h>
#include <unordered_set>

using namespace std;

#define DICTIONARY "rockyou.txt"
#define NUM_PASSWORDS 14344391 /* the number of lines in the rockyou dataset */
#define TERMINAL_WIDTH 80      /* used for progress bar, assume default width */

unordered_set<string> tried_passwords; /* used for memoization */

/*
 * tries to find the correct password by pre/appending numbers to the base 
 * password (start)
 * return values:
 *  - 0: password was NOT found
 *  - 1: password found
 */
int
find_password_add_int(const string start, const int max_len, const string password)
{
    size_t len = start.length();

    if (start == password) {
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
            if (tried_passwords.find(prepend) != tried_passwords.end()) {
                return 0;
            } else {
                tried_passwords.insert(prepend);
            }

            if (tried_passwords.find(append) != tried_passwords.end()) {
                return 0;
            } else {
                tried_passwords.insert(append);
            }

            if (find_password_add_int(append, max_len, password)
                || find_password_add_int(prepend, max_len, password)) {
                    return 1;
            }
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

/*
 * tries to find the correct password by repeating a common password twice.
 * return values:
 *  - 0: password was NOT found
 *  - 1: password found
 */
int
find_password_repeat(const string password, const int max_len)
{
    string repeated = password + password;
    size_t len = repeated.length();

    if ((int)len > max_len) {
        return 0;
    }

    if (repeated == password) {
        return 1;
    }
    return 0;
}

/* tries to find the correct password by looking it up in the dictionary and then
 * applying brute force variations such as the following:
 *  rules to apply for finding variations (each node can be assigned a rule):
 *  1. adding numbers
 *     Example: password -> 1password -> password1 -> password2 -> ...
 *  2. reverse spelling
 *      Example: password -> drowssap
 *  3. repeating password twice
 *      Example: password -> passwordpassword
 *  4. changing casing
 *      Example: password -> Password -> PASSWORD -> paSSword -> ...
 *  5. leet spellings
 *      Example: password -> p4ssw0rd -> p@ssword -> ...
 *  7. deleting characters
 *      Example: password -> passwrd -> assword -> ...
 *  8. adding all prefixes and suffixes
 *      Example: password -> apassword -> passwordx -> xpasswordy -> ...
 */
int
find_password(const string password, const int max_len)
{
    ifstream common_password(DICTIONARY);
    string curr_pass;
    int counter = 0;
    /* number of passwords needed to print each '#' to keep track of progress */
    int progress = NUM_PASSWORDS / TERMINAL_WIDTH;

    cout << "Looking up password... " << endl;

    while (common_password >> curr_pass) {
        if (curr_pass == password) {
            cout << endl
                 << "Your password is: " << curr_pass << endl;
            return 1;
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

    /* BRUTE FORCE attack starts here */
    common_password.clear();
    common_password.seekg(0, ios::beg);
    cout << "Your password is not that common..." << endl;
    cout << "Starting a brute force attack..." << endl;
    counter = 0;

    while (common_password >> curr_pass) {
        /* each if statement will be assigned to a node */
        if (find_password_add_int(curr_pass, max_len, password)) {
            cout << endl
                 << "Found the password by adding numbers!" << endl;
            return 1;
        }

        if (find_password_reverse(curr_pass, password)) {
            cout << endl
                 << "Found the password by reversing a common password!" << endl;
            return 1;
        }

        if (find_password_repeat(curr_pass, max_len)) {
            cout << endl
                 << "Found the password by reversing a common password!" << endl;
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
main(int argc, char *argv[])
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
        cout << "Nice password, couldn't crack it!" << endl;
    }

    auto end = chrono::high_resolution_clock::now();
    double time_taken =
        chrono::duration_cast<chrono::nanoseconds>(end - start).count() * 1e-9;

    cout << "Password lookup completed in " << time_taken << " seconds." << endl;

    return 0;
}
