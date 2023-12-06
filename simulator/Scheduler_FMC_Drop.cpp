#include "Scheduler.h"

using namespace std;

FMC_Drop::FMC_Drop() {
    name = "FMC_Drop";
}

FMC_Drop::FMC_Drop(const vector<Task> &tasksIn) : Scheduler(tasksIn) {
    name = "FMC_Drop";
}

void FMC_Drop::schedule(int quantum, int maxTime) {
    int runningId = -1;

    std::vector<TaskState> taskStates;
    int mode = 0;

    float uHigh = 0.0f;
    float uHighLowMode = 0.0f;
    float uLow = 0.0f;

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

    for (int time = 0; time <= maxTime; time++) {

        if (time % quantum == 0 ||
            runningId >= 0 && (taskStates[runningId].exeTime >= tasks[runningId].exeTimes[taskStates[runningId].exeNum] ||
                    (taskStates[runningId].exeTime > tasks[runningId].lowC && taskStates[runningId].level == LowMode) ||
                    time > taskStates[runningId].absoluteDeadline)) {

            switches += 2;

            if (runningId >= 0 && taskStates[runningId].exeTime >= tasks[runningId].exeTimes[taskStates[runningId].exeNum]) {
                if (tasks[runningId].crit == Low) {
                    succeedLow++;
                } else {
                    succeedHigh++;
                }
                taskStates[runningId].exeNum++;
                taskStates[runningId].state = Idle;
                taskStates[runningId].wakeupTime += tasks[runningId].period;
                taskStates[runningId].exeTime = 0;
                runningId = -1;
            }

            if (runningId >= 0 && taskStates[runningId].exeTime > tasks[runningId].lowC && taskStates[runningId].level == LowMode && tasks[runningId].crit == High) {
                mode++;
                taskStates[runningId].level = HighMode;
                taskStates[runningId].schedulingDeadline = taskStates[runningId].wakeupTime + tasks[runningId].period;
                float uLowTask = (float) tasks[runningId].lowC / tasks[runningId].period;
                float uHighTask = (float) tasks[runningId].highC / tasks[runningId].period;
                float newBudget = min(0.0f, ((uLowTask / uHighLowMode) * (1 - uLow) - uHighTask) / (1 - lamda));
                budget += newBudget;
                while (curULow > budget && curULow > .0001) {
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
                    if (runningId == maxId) {
                        runningId = -1;
                    }
                }
            }

            for (int i = 0; i < tasks.size(); i++) {
                if ((taskStates[i].state == Ready || taskStates[i].state == Running) && (time > taskStates[i].absoluteDeadline ||
                        (taskStates[i].exeTime > tasks[i].lowC && taskStates[i].level == LowMode))) {
                    if (tasks[i].crit == Low) {
                        failedLow++;
                    } else {
                        failedHigh++;
                    }
                    taskStates[i].exeNum++;
                    taskStates[i].state = Idle;
                    taskStates[i].wakeupTime += tasks[i].period;
                    taskStates[i].exeTime = 0;
                    if (i == runningId) {
                        runningId = -1;
                    }
                }
            }

            for (int i = 0; i < tasks.size(); i++) {
                if (taskStates[i].state == Idle && time >= taskStates[i].wakeupTime) {
                    taskStates[i].state = Ready;
                    if (tasks[i].crit == High && taskStates[i].level == LowMode) {
                        taskStates[i].schedulingDeadline = taskStates[i].wakeupTime + (int)(tasks[i].period * lamda);
                    } else {
                        taskStates[i].schedulingDeadline = taskStates[i].wakeupTime + tasks[i].period;
                    }
                    taskStates[i].absoluteDeadline = taskStates[i].wakeupTime + tasks[i].period;
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
                }
                taskStates[minId].state = Running;
                runningId = minId;
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
        }
        if (runningId >= 0) {
            taskStates[runningId].exeTime++;
        }
    }
}
