#include "Scheduler.h"

using namespace std;

RED::RED() {
    name = "RED";
}

RED::RED(const vector<Task> &tasksIn) : Scheduler(tasksIn) {
    name = "RED";
    for (const Task &t: tasks) {
        taskStates.emplace_back(TaskState{Idle, 0, t.period, 0, 0, 0});
    }
}

void RED::reset() {
    for (TaskState &t: taskStates) {
        t.state = Idle;
        t.wakeupTime = 0;
        t.absoluteDeadline = 0;
        t.wcet = 0;
        t.exeTime = 0;
        t.exeNum = 0;
    }
    readyQueue.clear();
    switches = failedHigh = failedLow = succeedHigh = succeedLow = 0;
}


void RED::reset(const vector<Task> &tasksIn) {
    tasks = tasksIn;
    taskStates.clear();
    for (const Task &t: tasks) {
        taskStates.emplace_back(TaskState{Idle, 0, t.period, 0, 0, 0});
    }
    reset();
}

void RED::schedule(int quantum, int maxTime) {
    reset();
    quantum = 1;

    for (int time = 0; time <= maxTime; time++) {
        switches++;

        int runningId = readyQueue.front();

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
            removeRunning();
        }

        for (int i = 0; i < tasks.size(); i++) {
            if ((taskStates[i].state == Ready || taskStates[i].state == Reject) &&
                time > taskStates[i].absoluteDeadline) {
                if (taskStates[i].state == Ready) {
                    removeId(i);
                }
                taskStates[i].exeNum++;
                taskStates[i].state = Idle;
                taskStates[i].wakeupTime += tasks[i].period;
                taskStates[i].exeTime = 0;
                if (tasks[i].crit == Low) {
                    failedLow++;
                } else {
                    failedHigh++;
                }
            }
        }

        for (int i = 0; i < tasks.size(); i++) {
            if (taskStates[i].state == Idle && time >= taskStates[i].wakeupTime) {
                taskStates[i].state = Ready;
                taskStates[i].absoluteDeadline = taskStates[i].wakeupTime + tasks[i].period;
                addToQueue(i);
                while (!removeVictim()) {}
            }
        }

        if (!readyQueue.empty()) {
            taskStates[readyQueue.front()].exeTime += quantum;
        }
    }
}

bool RED::addToQueue(int id) {
    auto it = readyQueue.rbegin();
    bool ret = true;
    int wcet = tasks[id].crit == High ? tasks[id].highC : tasks[id].lowC;
    for (; it != readyQueue.rend() && taskStates[id].absoluteDeadline < taskStates[*it].absoluteDeadline; it++) {
        taskStates[*it].wcet += wcet;
        if (taskStates[*it].wcet > tasks[*it].period) {
            ret = false;
        }
    }

    if (it != readyQueue.rend()) {
        wcet += taskStates[*it].wcet;
    }
    taskStates[id].wcet = wcet;
    readyQueue.insert(it.base(), id);
    if (wcet > tasks[id].period) {
        ret = false;
    }
    return ret;
}

bool RED::removeVictim() {
    int overloadId = -1;
    auto it = readyQueue.begin();
    auto removeIt = readyQueue.end();
    for (; it != readyQueue.end(); it++) {
        if (taskStates[*it].wcet > tasks[*it].period) {
            overloadId = *it;
            break;
        }
        if (tasks[*it].crit == Low) {
            removeIt = it;
        }
    }
    if (overloadId == -1) {
        return true;
    }
    if (tasks[overloadId].crit == Low || removeIt == readyQueue.end()) {
        removeIt = it;
    }

    it = removeIt;
    bool ret = true;
    int wcet = tasks[*it].crit == High ? tasks[*it].highC : tasks[*it].lowC;

    for (it++; it != readyQueue.end(); it++) {
        taskStates[*it].wcet -= wcet;
        if (taskStates[*it].wcet > tasks[*it].period && ret) {
            ret = false;
        }
    }
    taskStates[*removeIt].state = Reject;
    readyQueue.erase(removeIt);
    return ret;
}

void RED::removeRunning() {
    auto it = readyQueue.begin();
    if (it == readyQueue.end()) {
        return;
    }

    bool ret = true;
    int wcet = tasks[*it].crit == High ? tasks[*it].highC : tasks[*it].lowC;
    for (it++; it != readyQueue.end(); it++) {
        taskStates[*it].wcet -= wcet;
        if (taskStates[*it].wcet > tasks[*it].period && ret) {
            ret = false;
        }
    }
    readyQueue.erase(readyQueue.begin());

    if (ret) {
        int chosenLowId = -1;
        int chosenHighId = -1;
        for (int i = 0; i < tasks.size(); i++) {
            if (taskStates[i].state == Reject) {
                if (tasks[i].crit == High && (chosenHighId == -1 || taskStates[i].absoluteDeadline > taskStates[chosenHighId].absoluteDeadline)) {
                    chosenHighId = i;
                }
                if (tasks[i].crit == Low && (chosenLowId == -1 || taskStates[i].absoluteDeadline > taskStates[chosenLowId].absoluteDeadline)) {
                    chosenLowId = i;
                }
            }
        }
        int chosenId = chosenHighId >= 0 ? chosenHighId : chosenLowId;
        if (chosenId == -1) {
            return;
        }
        if (!addToQueue(chosenId)) {
            removeId(chosenId);
        } else {
            taskStates[chosenId].state = Ready;
        }
    }
}

void RED::removeId(int id) {
    auto it = readyQueue.begin();
    auto removeIt = readyQueue.end();
    int wcet = 0;
    for (; it != readyQueue.end(); it++) {
        if (*it == id) {
            removeIt = it;
            wcet = tasks[*it].crit == High ? tasks[*it].highC : tasks[*it].lowC;
        } else if (removeIt != readyQueue.end()) {
            taskStates[*it].wcet -= wcet;
        }
    }
    readyQueue.erase(removeIt);
}
