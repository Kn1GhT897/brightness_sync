#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/inotify.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <stdlib.h>

#define BUFFER_SIZE 4096
#define NV_FILE_PATH "/sys/class/backlight/nvidia_0/brightness"
#define AMD_FILE_PATH "/sys/class/backlight/amdgpu_bl0/brightness"

int init_inotify_fd(const char* path, uint32_t mask);
int init_epoll_fd(int fd);
bool check_inotify_event(char* buf, ssize_t len, const struct inotify_event* inotify_events, uint32_t mask);
void change_brightness(FILE* fp_nv, FILE* fp_amd);

int main() {
    while (access(NV_FILE_PATH, F_OK) == -1) sleep(1);

    int inotify_fd = init_inotify_fd(NV_FILE_PATH, IN_CLOSE_WRITE);
    int epoll_fd = init_epoll_fd(inotify_fd);
    uint32_t mask = IN_CLOSE_WRITE;
    struct epoll_event epoll_events[1];
    const struct inotify_event* inotify_events;
    char buf[BUFFER_SIZE] __attribute__ ((aligned(__alignof__(struct inotify_event))));
    ssize_t len;
    int nr;
    for (;;) {
        nr = epoll_wait(epoll_fd, epoll_events, 1, -1);
        if (nr < 0) {
            if (errno == EINTR) {
                execl("/proc/self/exe", "/proc/self/exe", NULL);
                exit(EXIT_FAILURE);
            }
            continue;
        }
        len = read(inotify_fd, buf, BUFFER_SIZE);
        if (len == -1) {
            continue;
        }
        if (check_inotify_event(buf, len, inotify_events, mask)) {
            FILE* fp_nv = fopen(NV_FILE_PATH, "r");
            if (fp_nv == NULL) {
                perror("fopen nvidia");
                continue;
            }
            FILE* fp_amd = fopen(AMD_FILE_PATH, "w");
            if (fp_amd == NULL) {
                perror("fopen amd");
                fclose(fp_nv);
                continue;
            }
            change_brightness(fp_nv, fp_amd);
            fclose(fp_nv);
            fclose(fp_amd);
        }
    }
    return 0;
}

int init_inotify_fd(const char* path, uint32_t mask) {
    int inotify_fd = inotify_init1(IN_NONBLOCK);
    if (inotify_fd == -1) {
        perror("inotify_init1");
        return -1;
    }
    int wd = inotify_add_watch(inotify_fd, path, mask);
    if (wd == -1) {
        perror("inotify_add_watch");
        close(inotify_fd);
        return -1;
    }
    return inotify_fd;
}

int init_epoll_fd(int fd) {
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        close(fd);
        return -1;
    }
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1) {
        perror("epoll_ctl");
        close(epoll_fd);
        close(fd);
        return -1;
    }
    return epoll_fd;
}

bool check_inotify_event(char* buf, ssize_t len, const struct inotify_event* inotify_events, uint32_t mask) {
    for (char *ptr = buf; ptr < buf + len; ptr += sizeof(struct inotify_event) + inotify_events->len) {
        inotify_events = (const struct inotify_event*) ptr;
        if (inotify_events->mask & mask) { 
            return true;
        }
    }
    return false;
}

void change_brightness(FILE* fp_nv, FILE* fp_amd) {
    int brightness_nvidia, brightness_amd;
    if (fscanf(fp_nv, "%d", &brightness_nvidia) == EOF) {
        return;
    }
    brightness_amd = 255 * brightness_nvidia / 100;
    fprintf(fp_amd, "%d", brightness_amd);
    fflush(fp_amd);
}
