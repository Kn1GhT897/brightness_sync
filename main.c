#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/inotify.h>
#include <sys/epoll.h>

#define _throw_return(info, return_val...) \
    perror(info); \
    return return_val;


void change_brightness() {
    FILE *fp = fopen("/sys/class/backlight/nvidia_0/brightness", "r");
    if (fp == NULL) {
        _throw_return("fopen nvidia");
    }

    int brightness_nvidia, brightness_amd;
    if (fscanf(fp, "%d", &brightness_nvidia) == EOF) {
        return;
    }
    brightness_amd = 255 * brightness_nvidia / 100;
    fclose(fp);

    fp = fopen("/sys/class/backlight/amdgpu_bl0/brightness", "w");
    if (fp == NULL) {
        _throw_return("fopen amd");
    }
    fprintf(fp, "%d", brightness_amd);
    fflush(fp);
    fclose(fp);
}


int main() {
    char *file_path = "/sys/class/backlight/nvidia_0/brightness";
    
    int fd = inotify_init1(IN_NONBLOCK);
    if (fd == -1) {
        _throw_return("inotify_init1", -1);
    }

    if (inotify_add_watch(fd, file_path, IN_CLOSE_WRITE) == -1) {
        _throw_return("add_watch", -1);
    }

    char buf[4096] 
        __attribute__ ((aligned(__alignof__(struct inotify_event))));
    const struct inotify_event *event;
    ssize_t len;
    
    struct epoll_event ev, events[1];
    int epfd = epoll_create1(0);
    if (epfd == -1) {
        _throw_return("epoll_create1", -1);
    }

    ev.events = EPOLLIN;
    ev.data.fd = fd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) == -1) {
        _throw_return("epoll_ctl", -1);
    }

    for (;;) {
        if (epoll_wait(epfd, events, 1, -1) == -1) {
            _throw_return("epoll_wait", -1);
        }
        
        len = read(fd, buf, sizeof(buf));
        if (len == -1) {
            _throw_return("read inofd", -1);
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
