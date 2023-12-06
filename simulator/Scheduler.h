#include <vector>
#include <queue>
#include <string>
#include <list>

#ifndef SIMULATOR_SCHEDULER_H
#define SIMULATOR_SCHEDULER_H

enum Criticality { Low, High };

struct Task {
    int period;
    Criticality crit;
    int lowC;
    int highC;
    std::vector<int> exeTimes;
};

class Scheduler {
public:
    Scheduler() = default;
    explicit Scheduler(const std::vector<Task>& tasksIn);
    virtual void schedule(int quantum, int maxTime) = 0;
    float getLowPFJ() const;
    float getHighPFJ() const;
    int getContextSwitches() const;
    std::string getName() const;
    virtual void reset();
    virtual void reset(const std::vector<Task>& tasksIn);

protected:
    std::string name;
    int failedLow = 0;
    int succeedLow = 0;
    int failedHigh = 0;
    int succeedHigh = 0;
    int switches = 0;
    std::vector<Task> tasks;
};

class EDF : public Scheduler {
public:
    EDF();
    explicit EDF(const std::vector<Task>& tasksIn);
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

class EDFVD : public Scheduler {
public:
    EDFVD();
    explicit EDFVD(const std::vector<Task>& tasksIn);
    void schedule(int quantum, int maxTime) override;

private:
    enum State { Idle, Ready, Running};
    enum CritState { HighMode, LowMode};

    struct TaskState {
        State state;
        int wakeupTime;
        int absoluteDeadline;
        int schedulingDeadline;
        int exeTime;
        int exeNum;
    };
    std::vector<TaskState> taskStates;
    void completeTask(int id, bool success);
};

class FMC : public Scheduler {
public:
    FMC();
    explicit FMC(const std::vector<Task>& tasksIn);
    void schedule(int quantum, int maxTime) override;

private:
    enum State { Idle, Ready, Running};
    enum CritState { HighMode, LowMode};

    struct TaskState {
        State state;
        CritState level;
        int wakeupTime;
        int absoluteDeadline;
        int schedulingDeadline;
        int lowBudget;
        int exeTime;
        int exeNum;
    };
};

class FMC_Drop : public Scheduler {
public:
    FMC_Drop();
    explicit FMC_Drop(const std::vector<Task>& tasksIn);
    void schedule(int quantum, int maxTime) override;

private:
    enum State { Idle, Ready, Running};
    enum CritState { HighMode, LowMode};

    struct TaskState {
        State state;
        CritState level;
        int wakeupTime;
        int absoluteDeadline;
        int schedulingDeadline;
        int exeTime;
        int exeNum;
        bool enabled;
    };
};

class H_FMC : public Scheduler {
public:
    H_FMC();
    explicit H_FMC(const std::vector<Task>& tasksIn);
    void schedule(int quantum, int maxTime) override;

private:
    enum State { Idle, Ready, Running};
    enum CritState { HighMode, LowMode};

    void completeTask(int id, bool success);

    struct TaskState {
        State state;
        CritState level;
        int wakeupTime;
        int absoluteDeadline;
        int schedulingDeadline;
        int exeTime;
        int exeNum;
        bool enabled;
    };
    std::vector<TaskState> taskStates;
};

class RED : public Scheduler {
public:
    RED();
    RED(const std::vector<Task>& tasksIn);
    void schedule(int quantum, int maxTime) override;

    void reset();
    void reset(const std::vector<Task> &tasksIn);

private:
    enum State { Idle, Ready, Reject};
    enum CritState { HighMode, LowMode};

    struct TaskState {
        State state;
        int wakeupTime;
        int absoluteDeadline;
        int wcet;
        int exeTime;
        int exeNum;
    };

    bool addToQueue(int id);
    bool removeVictim();
    void removeRunning();
    void removeId(int id);

    std::vector<TaskState> taskStates;
    std::list<int> readyQueue;
};

#endif //SIMULATOR_SCHEDULER_H
