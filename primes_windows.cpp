#include <windows.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <string>

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

bool writeAll(HANDLE h, const void* buf, int size) {
    const char* p = (const char*)buf;
    int total = 0;
    while (total < size) {
        DWORD w = 0;
        WriteFile(h, p + total, size - total, &w, NULL);
        total += w;
    }
    return true;
}

bool readAll(HANDLE h, void* buf, int size) {
    char* p = (char*)buf;
    int total = 0;
    while (total < size) {
        DWORD r = 0;
        ReadFile(h, p + total, size - total, &r, NULL);
        if (r == 0) return false;
        total += r;
    }
    return true;
}

int child() {
    Interval in;
    readAll(GetStdHandle(STD_INPUT_HANDLE), &in, sizeof(Interval));

    for (int n = in.start; n <= in.end; n++) {
        if (prim(n)) {
            writeAll(GetStdHandle(STD_OUTPUT_HANDLE), &n, sizeof(int));
        }
    }

    int stop = -1;
    writeAll(GetStdHandle(STD_OUTPUT_HANDLE), &stop, sizeof(int));
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc == 2 && string(argv[1]) == "child") return child();

    const int NR_PROC = 10;
    const int PAS = 1000;

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    HANDLE toChild[NR_PROC];
    HANDLE fromChild[NR_PROC];
    PROCESS_INFORMATION pi[NR_PROC];

    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);

    for (int i = 0; i < NR_PROC; i++) {
        HANDLE inR, inW, outR, outW;
        CreatePipe(&inR, &inW, &sa, 0);
        CreatePipe(&outR, &outW, &sa, 0);

        STARTUPINFOA si = {};
        si.cb = sizeof(si);
        si.hStdInput = inR;
        si.hStdOutput = outW;
        si.dwFlags = STARTF_USESTDHANDLES;

        string cmd = string(path) + " child";
        CreateProcessA(NULL, cmd.data(), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi[i]);

        CloseHandle(inR);
        CloseHandle(outW);

        toChild[i] = inW;
        fromChild[i] = outR;
    }

    for (int i = 0; i < NR_PROC; i++) {
        Interval in;
        in.start = i * PAS + 1;
        in.end = (i + 1) * PAS;
        writeAll(toChild[i], &in, sizeof(Interval));
        CloseHandle(toChild[i]);
    }

    vector<int> prime;
    for (int i = 0; i < NR_PROC; i++) {
        while (true) {
            int x;
            readAll(fromChild[i], &x, sizeof(int));
            if (x == -1) break;
            prime.push_back(x);
        }
        CloseHandle(fromChild[i]);
    }

    sort(prime.begin(), prime.end());
    for (int x : prime) cout << x << " ";
    cout << endl;

    return 0;
}
