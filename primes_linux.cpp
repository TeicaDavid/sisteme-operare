#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <vector>
#include <algorithm>
#include <cmath>

using namespace std;

struct Interval {
    int start;
    int end;
};

bool prim(int n) {
    if (n < 2) return false;
    if (n == 2) return true;
    if (n % 2 == 0) return false;
    int lim = (int)sqrt((double)n);
    for (int d = 3; d <= lim; d += 2) {
        if (n % d == 0) return false;
    }
    return true;
}

bool writeAll(int fd, const void* buf, int size) {
    const char* p = (const char*)buf;
    int total = 0;
    while (total < size) {
        int r = write(fd, p + total, size - total);
        if (r <= 0) return false;
        total += r;
    }
    return true;
}

bool readAll(int fd, void* buf, int size) {
    char* p = (char*)buf;
    int total = 0;
    while (total < size) {
        int r = read(fd, p + total, size - total);
        if (r <= 0) return false;
        total += r;
    }
    return true;
}

int main() {
    const int NR_PROC = 10;
    const int MAX_NR = 10000;
    const int PAS = 1000;

    int toChild[NR_PROC][2];
    int fromChild[NR_PROC][2];

    for (int i = 0; i < NR_PROC; i++) {
        pipe(toChild[i]);
        pipe(fromChild[i]);
    }

    for (int i = 0; i < NR_PROC; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            for (int j = 0; j < NR_PROC; j++) {
                if (j != i) {
                    close(toChild[j][0]);
                    close(toChild[j][1]);
                    close(fromChild[j][0]);
                    close(fromChild[j][1]);
                }
            }

            close(toChild[i][1]);
            close(fromChild[i][0]);

            Interval in;
            readAll(toChild[i][0], &in, sizeof(Interval));

            for (int n = in.start; n <= in.end; n++) {
                if (prim(n)) {
                    writeAll(fromChild[i][1], &n, sizeof(int));
                }
            }

            int stop = -1;
            writeAll(fromChild[i][1], &stop, sizeof(int));
            return 0;
        }
    }

    for (int i = 0; i < NR_PROC; i++) {
        close(toChild[i][0]);
        close(fromChild[i][1]);
    }

    for (int i = 0; i < NR_PROC; i++) {
        Interval in;
        in.start = i * PAS + 1;
        in.end = (i + 1) * PAS;
        writeAll(toChild[i][1], &in, sizeof(Interval));
        close(toChild[i][1]);
    }

    vector<int> prime;
    for (int i = 0; i < NR_PROC; i++) {
        while (true) {
            int x;
            readAll(fromChild[i][0], &x, sizeof(int));
            if (x == -1) break;
            prime.push_back(x);
        }
        close(fromChild[i][0]);
    }

    for (int i = 0; i < NR_PROC; i++) wait(NULL);

    sort(prime.begin(), prime.end());
    for (int x : prime) cout << x << " ";
    cout << endl;

    return 0;
}
