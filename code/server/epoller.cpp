#include "epoller.h"
#include <sys/epoll.h>

Epoller::Epoller(int maxEvent): epollFd_(epoll_create(1024)),
                             events_(maxEvent) {
    assert(epollFd_ >= 0 && events_.size() > 0);
}

Epoller::~Epoller() {
    close(epollFd_);
}

bool Epoller::addFd(int fd, uint32_t events) {
    if (fd < 0) return false;
    epoll_event ev = {0};
    ev.events = events;
    ev.data.fd = fd;
    int res = epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &ev);
    return res == 0 ? true : false;
}

bool Epoller::modFd(int fd, uint32_t events) {
    if (fd < 0) return false;
    epoll_event ev = {0};
    ev.events = events;
    ev.data.fd = fd;
    int res = epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &ev);
    return res == 0 ? true : false;
}

bool Epoller::delFd(int fd) {
    if (fd < 0) return false;
    epoll_event ev = {0};
    int res = epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, &ev);
    return res == 0 ? true : false;
}

int Epoller::wait(int timeoutMs) {
    return epoll_wait(epollFd_, &events_[0], 
                static_cast<int>(events_.size()), timeoutMs);
}

int Epoller::getEventFd(size_t idx) const {
    assert(idx < events_.size() && idx >= 0);
    return events_[idx].data.fd;
}

uint32_t Epoller::getEvents(size_t idx) const {
    assert(idx < events_.size() && idx >= 0);
    return events_[idx].events;
}