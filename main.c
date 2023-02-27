#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/inotify.h>
#include <sys/epoll.h>

void change_brightness() {
    FILE *fp = fopen("/sys/class/backlight/nvidia_0/brightness", "r");
    if (fp == NULL) {
        perror("fopen nvidia");
        return;
    }
    int brightness_nvidia;
    if (fscanf(fp, "%d", &brightness_nvidia) == EOF) {
        return;
    }
    int brightness_amd = 255 * brightness_nvidia / 100;
    fclose(fp);

    fp = fopen("/sys/class/backlight/amdgpu_bl0/brightness", "w");
    if (fp == NULL) {
        perror("fopen amd");
        return;
    }
    fprintf(fp, "%d", brightness_amd);
    fflush(fp);
    fclose(fp);
}


int main() {
    char *file_path = "/sys/class/backlight/nvidia_0/brightness";
    
    int fd = inotify_init1(IN_NONBLOCK);
    if (fd == -1) {
        perror("inotify_init1 failed");
        fflush(stderr);
        exit(EXIT_FAILURE);
    }

    int wd = inotify_add_watch(fd, file_path, IN_CLOSE_WRITE);
    if (wd == -1) {
        perror("add watch");
        exit(EXIT_FAILURE);
    }
    char buf[4096] 
        __attribute__ ((aligned(__alignof__(struct inotify_event))));
    const struct inotify_event *event;
    ssize_t len;
    
    struct epoll_event ev, events[1];
    int nfds;
    int epfd = epoll_create1(0);
    if (epfd == -1) {
        perror("epoll_create1 failed");
        fflush(stderr);
        exit(EXIT_FAILURE);
    }

    ev.events = EPOLLIN;
    ev.data.fd = fd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) == -1) {
        perror("epoll_ctl: listen_sock");
        fflush(stderr);
        exit(EXIT_FAILURE);
    }

    for (;;) {
        nfds = epoll_wait(epfd, events, 1, -1);
        if (nfds == -1) {
            perror("epoll_wait");
            fflush(stderr);
            exit(EXIT_FAILURE);
        }
        
        len = read(fd, buf, sizeof(buf));
        if (len == -1) {
            perror("read inofd");
            exit(EXIT_FAILURE);
        }
        
        for (char *ptr = buf; ptr < buf + len; ptr += sizeof(struct inotify_event) + event->len) {
            event = (const struct inotify_event*) ptr;
            if (event->mask & IN_CLOSE_WRITE) { 
                change_brightness();
            }
        }
    }

    return 0;
}
