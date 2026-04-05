// Copyright 2021 GHA Test Team
#include "TimedDoor.h"

#include <thread>
#include <chrono>
#include <stdexcept>
#include <memory.h>

// ================= DoorTimerAdapter =================

DoorTimerAdapter::DoorTimerAdapter(TimedDoor& d) : door(d) {}

void DoorTimerAdapter::Timeout() {
    // после "срабатывания" проверяем состояние
    if (door.isDoorOpened()) {
        door.throwState();
    }
}

// ================= TimedDoor =================

TimedDoor::TimedDoor(int timeout) : isOpened(false) {
    if (timeout <= 0)
        throw std::runtime_error("Timeout <= 0");

    iTimeout = timeout;
    adapter = new DoorTimerAdapter(*this);
}

bool TimedDoor::isDoorOpened() {
    return isOpened;
}

void TimedDoor::unlock() {
    isOpened = true;
    timer.tregister(iTimeout, adapter);
}

void TimedDoor::lock() {
    isOpened = false;
    timer.stop();
}

int TimedDoor::getTimeOut() const {
    return iTimeout;
}

void TimedDoor::throwState() {
    throw std::runtime_error("Door is opened too long!");
}

// ================= Timer =================

void Timer::sleep(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void Timer::tregister(int timeout, TimerClient* client) {
    if (worker) {
        throw std::runtime_error("Timer Already running");
    }
    started.store(false, std::memory_order_relaxed);
    std::unique_lock<std::mutex> uniq_lock(mut);

    worker = std::make_unique<std::thread>([this, timeout, client]() {
        {
            std::lock_guard<std::mutex> lock(this->mut);
            this->started.store(true, std::memory_order_relaxed);
        }
        this->cv.notify_one();

        sleep(timeout);
        client->Timeout();
    });

    cv.wait(uniq_lock, 
        [this] 
        { 
            return this->started.load(std::memory_order::memory_order_relaxed); 
        }
    );
}

void Timer::stop() {
    if (worker->joinable())
        worker->join();
}
