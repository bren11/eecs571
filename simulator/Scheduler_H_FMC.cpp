#include "Scheduler.h"

using namespace std;

H_FMC::H_FMC() {
    name = "H-FMC";
}

H_FMC::H_FMC(const vector<Task> &tasksIn) : Scheduler(tasksIn) {
    name = "H-FMC";
}

void H_FMC::schedule(int quantum, int maxTime) {
    int runningId = -1;
    quantum = 1;

    int mode = 0;

    float uHigh = 0.0f;
    float uHighLowMode = 0.0f;
    float uLow = 0.0f;

    taskStates.clear();
    for (const Task& t : tasks) {
        taskStates.emplace_back(TaskState {Ready, LowMode, 0, t.period, t.period, 0, 0, true});
        if (t.crit == Low) {
            uLow += (float) t.lowC / (float) t.period;
        } else {
            uHighLowMode += (float) t.lowC / (float) t.period;
            uHigh += (float) t.highC / (float) t.period;
        }
    }

    float lamda = uHighLowMode / (1 - uLow);

    for (int i = 0; i < tasks.size(); i++) {
        if (tasks[i].crit == High) {
            taskStates[i].schedulingDeadline = tasks[i].period * lamda;
        }
    }

    switches = failedHigh = failedLow = succeedHigh = succeedLow = 0;
    float budget = uLow;
    float curULow = uLow;

    for (int time = 0; time <= maxTime; time += quantum) {

        if (runningId >= 0 && taskStates[runningId].exeTime > tasks[runningId].lowC && taskStates[runningId].level == LowMode && tasks[runningId].crit == High) {
            mode++;
            taskStates[runningId].level = HighMode;
            taskStates[runningId].schedulingDeadline = taskStates[runningId].wakeupTime + tasks[runningId].period;
            float uLowTask = (float) tasks[runningId].lowC / tasks[runningId].period;
            float uHighTask = (float) tasks[runningId].highC / tasks[runningId].period;
            float newBudget = min(0.0f, ((uLowTask / uHighLowMode) * (1 - uLow) - uHighTask) / (1 - lamda));
            budget += newBudget;
        }

        if (runningId >= 0 && taskStates[runningId].exeTime >= tasks[runningId].exeTimes[taskStates[runningId].exeNum]) {
            completeTask(runningId, true);
            runningId = -1;
        } else {
            for (int i = 0; i < tasks.size(); i++) {
                if ((taskStates[i].state == Ready || taskStates[i].state == Running) && (time > taskStates[i].absoluteDeadline ||
                                                                                         (taskStates[i].exeTime > tasks[i].lowC && tasks[i].crit == Low))) {
                    completeTask(i, false);
                    if (i == runningId) {
                        switches++;
                        runningId = -1;
                    }
                    break;
                }
            }
        }

        if (curULow > budget && curULow > .0001) {
            int maxId = -1;
            float maxU = 0.0f;
            for (int i = 0; i < tasks.size(); i++) {
                float u = (float) tasks[i].lowC / tasks[i].period;
                if (tasks[i].crit == Low && taskStates[i].enabled && (maxId == -1 || u > maxU)) {
                    maxId = i;
                    maxU = u;
                }
            }
            taskStates[maxId].enabled = false;
            curULow -= maxU;
        }

        for (int i = 0; i < tasks.size(); i++) {
            if (taskStates[i].state == Idle && time >= taskStates[i].wakeupTime) {
                if (taskStates[i].enabled) {
                    taskStates[i].state = Ready;
                    if (tasks[i].crit == High && taskStates[i].level == LowMode) {
                        taskStates[i].schedulingDeadline = taskStates[i].wakeupTime + (int) (tasks[i].period * lamda);
                    } else {
                        taskStates[i].schedulingDeadline = taskStates[i].wakeupTime + tasks[i].period;
                    }
                    taskStates[i].absoluteDeadline = taskStates[i].wakeupTime + tasks[i].period;
                    break;
                } else {
                    completeTask(i, false);
                }
            }
        }

        int minId = runningId;
        for (int i = 0; i < tasks.size(); i++) {
            if (taskStates[i].state == Ready && taskStates[i].enabled && (minId < 0 || taskStates[i].schedulingDeadline < taskStates[minId].schedulingDeadline)) {
                minId = i;
            }
        }
        if (minId != runningId) {
            if (runningId >= 0) {
                taskStates[runningId].state = Ready;
                switches++;
            }
            taskStates[minId].state = Running;
            runningId = minId;
            switches++;
        }

        if (runningId == -1 && mode > 0) {
            mode = 0;
            budget = curULow = uLow;
            for (int i = 0; i < tasks.size(); i++) {
                if (taskStates[i].state == Ready && (runningId < 0 || taskStates[i].schedulingDeadline < taskStates[runningId].schedulingDeadline)) {
                    runningId = i;
                }
                if (tasks[i].crit == Low) {
                    taskStates[i].enabled = true;
                } else {
                    taskStates[i].level = LowMode;
                }
            }
            if (runningId >= 0) {
                taskStates[runningId].state = Running;
            }
        }
        if (runningId >= 0) {
            taskStates[runningId].exeTime += quantum;
        }
    }
}

void H_FMC::completeTask(int id, bool success) {
    if (success) {
        if (tasks[id].crit == Low) {
            succeedLow++;
        } else {
            succeedHigh++;
        }
    } else {
        if (tasks[id].crit == Low) {
            failedLow++;
        } else {
            failedHigh++;
        }
    }
    taskStates[id].exeNum++;
    taskStates[id].state = Idle;
    taskStates[id].wakeupTime += tasks[id].period;
    taskStates[id].exeTime = 0;
}
