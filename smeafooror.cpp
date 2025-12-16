#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <semaphore.h>
#include <cstdlib>
#include <ctime>

using namespace std;

struct Shared {
    int x;
};

int main() {
    const char* SHM_NAME = "/so_shm_1000";
    const char* SEM_NAME = "/so_sem_1000";

    bool creator = false;

    int fd = shm_open(SHM_NAME, O_RDWR | O_CREAT | O_EXCL, 0666);
    if (fd >= 0) {
        creator = true;
        ftruncate(fd, sizeof(Shared));
    } else {
        fd = shm_open(SHM_NAME, O_RDWR, 0666);
        if (fd < 0) return 1;
    }

    void* p = mmap(NULL, sizeof(Shared), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (p == MAP_FAILED) return 1;

    Shared* sh = (Shared*)p;

    sem_t* sem = sem_open(SEM_NAME, O_CREAT, 0666, 1);
    if (sem == SEM_FAILED) return 1;

    srand((unsigned)time(NULL) ^ (unsigned)getpid());

    if (creator) sh->x = 0;

    while (true) {
        sem_wait(sem);

        if (sh->x >= 1000) {
            sem_post(sem);
            break;
        }

        int coin = (rand() % 2) + 1;
        while (coin == 2 && sh->x < 1000) {
            sh->x++;
            cout << "PID " << getpid() << " -> " << sh->x << "\n";
            coin = (rand() % 2) + 1;
        }

        sem_post(sem);
        usleep(1000 * (rand() % 30 + 5));
    }

    munmap(p, sizeof(Shared));
    close(fd);
    sem_close(sem);

    if (creator) {
        usleep(300000);
        shm_unlink(SHM_NAME);
        sem_unlink(SEM_NAME);
    }

    return 0;
}
