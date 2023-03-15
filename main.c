#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/inotify.h>
#include <sys/epoll.h>
#include <sys/stat.h>


#define nv_file_path "/sys/class/backlight/nvidia_0/brightness"
#define amd_file_path "/sys/class/backlight/amdgpu_bl0/brightness"
#define _throw_return(info, return_val...) \
    perror(info); \
    return return_val;
#define _throw_exit(info) \
    perror(info); \
    exit(EXIT_FAILURE);


int init_inotify_fd(int*, int*, u_int32_t);
int init_epoll_fd(int*, int);
int get_inode(int);
bool check_inotify_event(char*, ssize_t, const struct inotify_event*, u_int32_t);
void change_brightness();


int main() {
    while (access(nv_file_path, F_OK) == -1) sleep(1);

    char buf[4096] 
        __attribute__ ((aligned(__alignof__(struct inotify_event))));
    const struct inotify_event *inotify_events;
    ssize_t len;

    struct epoll_event epoll_events[1];

    int fd, wd, inode;
    u_int32_t mask = IN_CLOSE_WRITE;
    if (init_inotify_fd(&fd, &wd, mask) == -1) {
        return -1;
    }
    inode = get_inode(fd);

    int epoll_fd;
    if (init_epoll_fd(&epoll_fd, fd) == -1) {
        return -1;
    }

    int nr;
    for (;;) {
        nr = epoll_wait(epoll_fd, epoll_events, 1, -1);
        if (nr < 0) {
            if (errno == EINTR) {
                execl("/proc/self/exe", "/proc/self/exe", NULL);
                exit(1);
            }
            continue;
        }

        len = read(fd, buf, sizeof(buf));
        if (len == -1) {
            continue;
        }
        
        if (check_inotify_event(buf, len, inotify_events, mask)) {
            change_brightness();
        }
    }
    return 0;
}


int init_inotify_fd(int *fd, int *wd, u_int32_t mask) {
    /*
        *fd: the file descriptor to be init
        *wd: the watch descriptor to be init
        mask: inotify mask
    */
    *fd = inotify_init1(IN_NONBLOCK);
    if (*fd == -1) {
        _throw_return("inotify_init1", -1);
    }
    *wd = inotify_add_watch(*fd, nv_file_path, mask);
    if (*wd == -1) {
        _throw_return("inotify_add_watch", -1);
    }
    return 0;
}


int init_epoll_fd(int *epoll_fd, int fd) {
    /*
        *epoll_fd: as its name
        fd: as its name 
    */
    struct epoll_event ev;
    *epoll_fd = epoll_create1(0);
    if (*epoll_fd == -1) {
        _throw_return("epoll_create1", -1);
    }

    ev.events = EPOLLIN;
    ev.data.fd = fd;
    if (epoll_ctl(*epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1) {
        _throw_return("epoll_ctl", -1);
    }
    return 0;
}


int get_inode(int fd) {
    struct stat file_stat;
    if (fstat(fd, &file_stat) < 0) {
        _throw_exit("fstat");
    }
    return file_stat.st_ino;
}


bool check_inotify_event(char* buf, ssize_t len, const struct inotify_event* inotify_events, u_int32_t mask) {
    for (char *ptr = buf; ptr < buf + len; ptr += sizeof(struct inotify_event) + inotify_events->len) {
        inotify_events = (const struct inotify_event*) ptr;
        if (inotify_events->mask & mask) { 
            return true;
        }
    }
    return false;
}


void change_brightness() {
    FILE *fp = fopen(nv_file_path, "r");
    if (fp == NULL) {
        _throw_return("fopen nvidia");
    }

    int brightness_nvidia, brightness_amd;
    if (fscanf(fp, "%d", &brightness_nvidia) == EOF) {
        return;
    }
    brightness_amd = 255 * brightness_nvidia / 100;
    fclose(fp);

    fp = fopen(amd_file_path, "w");
    if (fp == NULL) {
        _throw_return("fopen amd");
    }
    fprintf(fp, "%d", brightness_amd);
    fflush(fp);
    fclose(fp);
}