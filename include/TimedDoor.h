// Copyright 2021 GHA Test Team

#ifndef INCLUDE_TIMEDDOOR_H_
#define INCLUDE_TIMEDDOOR_H_

#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>
#include <thread>

class DoorTimerAdapter;
class Timer;
class Door;
class TimedDoor;

class TimerClient {
 public:
  virtual void Timeout() = 0;
};

class Timer {
  TimerClient *client;
  void sleep(int);

  std::mutex mut;
  std::atomic<bool> started;
  std::condition_variable cv;
  std::unique_ptr<std::thread> worker;
 public:
  void tregister(int, TimerClient*);
  void stop();
};

class Door {
 public:
  virtual void lock() = 0;
  virtual void unlock() = 0;
  virtual bool isDoorOpened() = 0;
};

class DoorTimerAdapter : public TimerClient {
 private:
  TimedDoor& door;
 public:
  explicit DoorTimerAdapter(TimedDoor&);
  void Timeout();
};

class TimedDoor : public Door {
 private:
  DoorTimerAdapter * adapter;
  int iTimeout;
  bool isOpened;
  Timer timer;
 public:
  explicit TimedDoor(int);
  bool isDoorOpened();
  void unlock();
  void lock();
  int  getTimeOut() const;
  virtual void throwState();
};

#endif  // INCLUDE_TIMEDDOOR_H_
