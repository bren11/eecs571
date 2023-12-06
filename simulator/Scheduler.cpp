#include "Scheduler.h"

using namespace std;

Scheduler::Scheduler(const vector<Task> &tasksIn) {
    tasks = tasksIn;
}

int Scheduler::getContextSwitches() const {
    return switches;
}

float Scheduler::getLowPFJ() const {
    if (succeedLow + failedLow == 0) {
        return 1;
    }
    return (float) succeedLow / (float) (succeedLow + failedLow);
}

float Scheduler::getHighPFJ() const {
    if (succeedHigh + failedHigh == 0) {
        return 1;
    }
    return (float) succeedHigh / (float) (succeedHigh + failedHigh);
}

std::string Scheduler::getName() const {
    return name;
}

void Scheduler::reset() {
    switches = failedHigh = failedLow = succeedHigh = succeedLow = 0;
}

void Scheduler::reset(const vector<Task> &tasksIn) {
    tasks = tasksIn;
    reset();
}

EDF::EDF() {
    name = "EDF";
}

EDF::EDF(const std::vector<Task> &tasksIn) : Scheduler(tasksIn) {
    name = "EDF";
}

void EDF::schedule(int quantum, int maxTime) {
    int runningId = -1;

    std::vector<TaskState> taskStates;

    reset();

    for (const Task &t: tasks) {
        taskStates.emplace_back(TaskState{Ready, 0, t.period, 0, 0});
    }

    for (int time = 0; time <= maxTime; time++) {

        if (time % quantum == 0 ||
            runningId >= 0 &&
            (taskStates[runningId].exeTime >= tasks[runningId].exeTimes[taskStates[runningId].exeNum] ||
             time > taskStates[runningId].absoluteDeadline)) {

            switches += 2;

            if (runningId >= 0 &&
                taskStates[runningId].exeTime >= tasks[runningId].exeTimes[taskStates[runningId].exeNum]) {
                if (tasks[runningId].crit == Low) {
                    succeedLow++;
                } else if (tasks[runningId].crit == High) {
                    succeedHigh++;
                }
                taskStates[runningId].exeNum++;
                taskStates[runningId].state = Idle;
                taskStates[runningId].wakeupTime += tasks[runningId].period;
                taskStates[runningId].exeTime = 0;
                runningId = -1;
            }

            for (int i = 0; i < tasks.size(); i++) {
                if ((taskStates[i].state == Ready || taskStates[i].state == Running) &&
                    time > taskStates[i].absoluteDeadline) {
                    taskStates[i].exeNum++;
                    taskStates[i].state = Idle;
                    taskStates[i].wakeupTime += tasks[i].period;
                    taskStates[i].exeTime = 0;
                    if (tasks[i].crit == Low) {
                        failedLow++;
                    } else {
                        failedHigh++;
                    }
                    if (i == runningId) {
                        runningId = -1;
                    }
                }
            }

            for (int i = 0; i < tasks.size(); i++) {
                if (taskStates[i].state == Idle && time >= taskStates[i].wakeupTime) {
                    taskStates[i].state = Ready;
                    taskStates[i].absoluteDeadline = taskStates[i].wakeupTime + tasks[i].period;
                }
            }

            int minId = runningId;
            for (int i = 0; i < tasks.size(); i++) {
                if (taskStates[i].state == Ready &&
                    (minId < 0 || taskStates[i].absoluteDeadline < taskStates[minId].absoluteDeadline)) {
                    minId = i;
                }
            }
            if (minId != runningId) {
                if (runningId >= 0) {
                    taskStates[runningId].state = Ready;
                }
                taskStates[minId].state = Running;
                runningId = minId;
            }
        }
        if (runningId >= 0) {
            taskStates[runningId].exeTime++;
        }
    }
}
