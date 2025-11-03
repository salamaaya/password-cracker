#include <algorithm>
#include <cuda.h>
#include <cuda_runtime.h>
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
#include <cuda_runtime.h>

__device__ int my_strlen(const char* s) {
    int len = 0;
    while (s[len] != '\0') len++;
    return len;
}

__device__ int my_strcmp(const char* a, const char* b) {
    int i = 0;
    while (a[i] == b[i]) {
        if (a[i] == '\0') return 0;
        i++;
    }
    return (unsigned char)a[i] - (unsigned char)b[i];
}

__device__ void my_itoa(int num, char* str) {
    int i = 0;
    if (num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }
    char buf[16];
    while (num > 0) {
        buf[i++] = (num % 10) + '0';
        num /= 10;
    }
    // reverse
    for (int j = 0; j < i; j++)
        str[j] = buf[i - j - 1];
    str[i] = '\0';
}

__global__ void find_add_int_kernel(const char* start, int max_len,
                                   const char* pwd, int* found)
{
    int tid = blockIdx.x * blockDim.x + threadIdx.x;
    if (tid >= 100000000) return; // safety limit

    char num_str[16];
    my_itoa(tid, num_str);

    char combo[256];
    int n = 0;

    // prepend num + start
    for (int i = 0; num_str[i]; i++) combo[n++] = num_str[i];
    for (int i = 0; start[i]; i++) combo[n++] = start[i];
    combo[n] = '\0';

    if (my_strcmp(combo, pwd) == 0) {
        atomicExch(found, 1);
        return;
    }

    // append start + num
    n = 0;
    for (int i = 0; start[i]; i++) combo[n++] = start[i];
    for (int i = 0; num_str[i]; i++) combo[n++] = num_str[i];
    combo[n] = '\0';

    if (my_strcmp(combo, pwd) == 0) {
        atomicExch(found, 1);
        return;
    }
}

/*
 * tries to find the correct pwd by deleting characters from the base
 * password (start) and stops when the string is empty
 * return values:
 *  - 0: pwd was NOT found
 *  - 1: pwd found
 */
int
find_pwd_remove(const string start, const string pwd,
                unordered_set<string>& visited,
                int removed, int max_removals)
{
    if (visited.count(start)) {
        return 0;
    }
    visited.insert(start);

    if (start == pwd) {
        cout << endl << "Your password is: " << start << endl;
        return 1;
    }

    // Stop if we've removed too many characters or the string is empty
    if (removed >= max_removals || start.empty()) {
        return 0;
    }

    size_t len = start.length();
    for (size_t i = 0; i < len; i++) {
        string new_pwd = start;
        new_pwd.erase(i, 1);

        if (find_pwd_remove(new_pwd, pwd, visited, removed + 1, max_removals)) {
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
find_pwd_casing(const string start, const string end, const string pwd, unordered_set<string>& visited)
{
    if (visited.count(start)) {
        return 0;
    }

    visited.insert(start);

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
        
        if (find_pwd_casing(new_pwd, end, pwd, visited)) {
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

    string repeated = start + start;
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

    ifstream common_pwd(DICTIONARY);
    string curr_pass;

    while (common_pwd >> curr_pass) {
        if (rank == 1) {
            if (curr_pass.length() < max_len) {
                int threads_per_block = 256;
                int num_blocks = (100000 + threads_per_block - 1) / threads_per_block;

                char* d_pwd;
                char* d_start;
                int* d_found;
                int h_found = 0;

                cudaMalloc((void**)&d_pwd, pwd.length() + 1);
                cudaMalloc((void**)&d_start, curr_pass.length() + 1);
                cudaMalloc((void**)&d_found, sizeof(int));

                cudaMemcpy(d_pwd, pwd.c_str(), pwd.length() + 1, cudaMemcpyHostToDevice);
                cudaMemcpy(d_found, &h_found, sizeof(int), cudaMemcpyHostToDevice);
                cudaMemcpy(d_start, curr_pass.c_str(), curr_pass.length() + 1, cudaMemcpyHostToDevice);

                find_add_int_kernel<<<num_blocks, threads_per_block>>>(d_start, max_len, d_pwd, d_found);

                cudaMemcpy(&h_found, d_found, sizeof(int), cudaMemcpyDeviceToHost);
                if (h_found) {
                    cout << endl
                        << "Rank 1 found the password by adding numbers to '"
                        << curr_pass << "'." << endl;
                    cudaFree(d_pwd);
                    cudaFree(d_found);
                    MPI_Abort(MPI_COMM_WORLD, 4);
                }

                cudaFree(d_pwd);
                cudaFree(d_found);
            }
        } else if (rank == 2) {
            if (curr_pass.length() <= max_len) {
                if (find_pwd_reverse(curr_pass, pwd)) {
                    cout << endl
                        << "Rank 2 found the password by reversing '"
                        << curr_pass << "'." << endl;
                    MPI_Abort(MPI_COMM_WORLD, 0);
                }
            }
        } else if (rank == 3) {
            if (find_pwd_repeat(curr_pass, max_len, pwd)) {
                cout << endl
                    << "Rank 3 found the password by repeating '"
                    << curr_pass << "'." << endl;
                MPI_Abort(MPI_COMM_WORLD, 0);
            }
        } else if (rank == 4) {
            if (curr_pass.length() <= max_len) {
                unordered_set<string> visited;
                string lower = curr_pass;
                transform(curr_pass.begin(), curr_pass.end(), lower.begin(), ::tolower);
                string upper = curr_pass;
                transform(curr_pass.begin(), curr_pass.end(), upper.begin(), ::toupper);
                
                if (find_pwd_casing(lower, upper, pwd, visited)) {
                    cout << endl 
                        << "Rank 4 found the password by changing casing on '"
                        << curr_pass << "'." << endl;
                    MPI_Abort(MPI_COMM_WORLD, 0);
                }
            }
        } else if (rank == 5) {
            unordered_set<string> visited;
            if (find_pwd_remove(curr_pass, pwd, visited, 0, 4)) {
                cout << endl
                    << "Rank 5 found the password by removing characters from '"
                    << curr_pass << "'." << endl;
                MPI_Abort(MPI_COMM_WORLD, 0);
            }
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

