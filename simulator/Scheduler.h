#include <vector>
#include <queue>

#ifndef SIMULATOR_SCHEDULER_H
#define SIMULATOR_SCHEDULER_H

enum Criticality { Low, High, Interrupt };

struct Task {
    int period;
    Criticality crit;
    int lowC;
    int highC;
    std::vector<int> exeTimes;
};

class Scheduler {
public:
    Scheduler(const std::vector<Task>& tasksIn);
    virtual void schedule(int quantum, int maxTime) = 0;
    float getLowPFJ() const;
    float getHighPFJ() const;
    float getIntPFJ() const;
    int getContextSwitches() const;

protected:
    int failedLow = 0;
    int succeedLow = 0;
    int failedHigh = 0;
    int succeedHigh = 0;
    int failedInt = 0;
    int succeedInt = 0;
    int switches = 0;
    std::vector<Task> tasks;
};

class EDF : public Scheduler {
public:
    EDF(const std::vector<Task>& tasksIn);
    void schedule(int quantum, int maxTime) override;

private:
    enum State { Idle, Ready, Running };

    struct TaskState {
        State state;
        int wakeupTime;
        int absoluteDeadline;
        int exeTime;
        int exeNum;
    };
};

class EDFVD : public EDF {
public:
    EDFVD(const std::vector<Task>& tasksIn);
    void schedule(int quantum, int maxTime) override;

private:
    enum State { Idle, Ready, Running};
    enum CritState { HighMode, LowMode};

    struct TaskState {
        State state;
        int wakeupTime;
        int absoluteDeadline;
        int exeTime;
        int exeNum;
    };
};

#endif //SIMULATOR_SCHEDULER_H
