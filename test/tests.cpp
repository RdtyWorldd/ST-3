#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <thread>
#include <chrono>

#include "TimedDoor.h"

using ::testing::Exactly;
using ::testing::AtLeast;
using ::testing::InSequence;

class MockTimerClient : public TimerClient {
public:
    MOCK_METHOD(void, Timeout, (), (override));
};

class MockTimedDoor : public TimedDoor {
public:
    MockTimedDoor(int timeout) : TimedDoor(timeout) {}

    MOCK_METHOD(bool, isDoorOpened, (), (override));
    MOCK_METHOD(void, throwState, (), (override));
};


class TimedDoorTest : public ::testing::Test {
protected:
    TimedDoor* door;

    void SetUp() override {
        door = new TimedDoor(50);
    }

    void TearDown() override {
        delete door;
    }
};

TEST(ConstructorTest, ThrowsOnInvalidTimeout) {
    EXPECT_THROW(TimedDoor(0), std::runtime_error);
}

TEST_F(TimedDoorTest, InitiallyClosed) {
    EXPECT_FALSE(door->isDoorOpened());
}

TEST_F(TimedDoorTest, UnlockOpensDoor) {
    door->unlock();
    EXPECT_TRUE(door->isDoorOpened());

    door->lock();
    std::this_thread::sleep_for(std::chrono::milliseconds(door->getTimeOut()));
}

TEST_F(TimedDoorTest, LockClosesDoor) {
    door->unlock();
    door->lock();
    EXPECT_FALSE(door->isDoorOpened());
}

TEST_F(TimedDoorTest, ThrowsIfDoorStillOpened) {
    EXPECT_DEATH({
        TimedDoor door(50);
        door.unlock();

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }, "");
}

TEST_F(TimedDoorTest, NoThrowIfClosedBeforeTimeout) {
    door->unlock();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    door->lock();

    std::this_thread::sleep_for(std::chrono::milliseconds(door->getTimeOut()));

    EXPECT_FALSE(door->isDoorOpened());
    SUCCEED();
}

TEST_F(TimedDoorTest, GetTimeout) {
    EXPECT_EQ(door->getTimeOut(), 50);
}

TEST(TimerTest, CallsTimeoutOnClient) {
    Timer timer;
    MockTimerClient client;

    EXPECT_CALL(client, Timeout()).Times(1);

    timer.tregister(10, &client);

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    timer.stop();
}

TEST(TimerTest, MultipleTimeoutCalls) {
    Timer timer;
    MockTimerClient client;

    EXPECT_CALL(client, Timeout()).Times(1);

    timer.tregister(10, &client);
    EXPECT_ANY_THROW(timer.tregister(10, &client));

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    timer.stop();
}

TEST(DoorTimerAdapterTest, CallsThrowState) {
    MockTimedDoor door(50);
    DoorTimerAdapter adapter(door);

    EXPECT_CALL(door, isDoorOpened())
        .Times(1)
        .WillOnce(::testing::Return(true));

    EXPECT_CALL(door, throwState())
        .Times(1);

    adapter.Timeout();
}

TEST(DoorTimerAdapterTest, CallsThrowStateIfDoorUnlocked) {
    MockTimedDoor door(50);

    EXPECT_CALL(door, isDoorOpened())
        .Times(1)
        .WillOnce(::testing::Return(true));

    EXPECT_CALL(door, throwState())
        .Times(1);

    door.unlock();
    std::this_thread::sleep_for(std::chrono::milliseconds(door.getTimeOut() + 10));
    door.lock();
}

TEST(DoorTimerAdapterTest, CallsIfDoorClosedAfterWait) {
    MockTimedDoor door(50);

    EXPECT_CALL(door, isDoorOpened())
        .Times(1)
        .WillOnce(::testing::Return(false));

    door.unlock();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    door.lock();
}
