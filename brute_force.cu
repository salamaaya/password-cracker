#include <unordered_set>

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
