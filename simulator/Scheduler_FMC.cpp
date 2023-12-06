#include "Scheduler.h"

using namespace std;

FMC::FMC() {
    name = "FMC";
}

FMC::FMC(const vector<Task> &tasksIn) : Scheduler(tasksIn) {
    name = "FMC";
}

void FMC::schedule(int quantum, int maxTime) {
    int runningId = -1;

    std::vector<TaskState> taskStates;
    int mode = 0;

    float uHigh = 0.0f;
    float uHighLowMode = 0.0f;
    float uLow = 0.0f;

    for (const Task& t : tasks) {
        taskStates.emplace_back(TaskState {Ready, LowMode, 0, t.period, t.period, t.lowC, 0, 0});
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

    reset();
    float budget = 1.0;

    for (int time = 0; time <= maxTime; time++) {

        if (time % quantum == 0 ||
            runningId >= 0 && (taskStates[runningId].exeTime >= tasks[runningId].exeTimes[taskStates[runningId].exeNum] ||
                    (taskStates[runningId].exeTime > taskStates[runningId].lowBudget && taskStates[runningId].level == LowMode) ||
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

            if (runningId >= 0 && taskStates[runningId].exeTime > taskStates[runningId].lowBudget && taskStates[runningId].level == LowMode && tasks[runningId].crit == High) {
                mode++;
                taskStates[runningId].level = HighMode;
                taskStates[runningId].schedulingDeadline = taskStates[runningId].wakeupTime + tasks[runningId].period;
                float uLowTask = (float) tasks[runningId].lowC / tasks[runningId].period;
                float uHighTask = (float) tasks[runningId].highC / tasks[runningId].period;
                float newBudget = min(0.0f, ((uLowTask / uHighLowMode) * (1 - uLow) - uHighTask) / ((1 - lamda) * uLow));
                budget += newBudget;
                for (int i = 0; i < tasks.size(); i++) {
                    if (tasks[i].crit == Low) {
                        taskStates[i].lowBudget = budget * tasks[i].lowC;
                    }
                }
            }

            for (int i = 0; i < tasks.size(); i++) {
                if ((taskStates[i].state == Ready || taskStates[i].state == Running) && (time > taskStates[i].absoluteDeadline ||
                        (taskStates[i].exeTime > taskStates[i].lowBudget && taskStates[i].level == LowMode))) {
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
                if (taskStates[i].state == Ready && (minId < 0 || taskStates[i].schedulingDeadline < taskStates[minId].schedulingDeadline)) {
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
                budget = 1.0;
                for (int i = 0; i < tasks.size(); i++) {
                    if (taskStates[i].state == Ready && (runningId < 0 || taskStates[i].schedulingDeadline < taskStates[runningId].schedulingDeadline)) {
                        runningId = i;
                    }
                    if (tasks[i].crit == Low) {
                        taskStates[i].lowBudget = tasks[i].lowC;
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
