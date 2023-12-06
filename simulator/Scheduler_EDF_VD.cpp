#include "Scheduler.h"

using namespace std;

EDFVD::EDFVD() {
    name = "EDF-VD";
}

EDFVD::EDFVD(const vector<Task> &tasksIn) : Scheduler(tasksIn) {
    name = "EDF-VD";
}

void EDFVD::schedule(int quantum, int maxTime) {
    int runningId = -1;

    taskStates.clear();
    CritState mode = LowMode;

    float uHigh = 0.0f;
    float uHighLowMode = 0.0f;
    float uLow = 0.0f;

    for (const Task& t : tasks) {
        taskStates.emplace_back(TaskState {Ready, 0, t.period, t.period, 0, 0});
        if (t.crit == Low) {
            uLow += (float) t.lowC / (float) t.period;
        } else {
            uHighLowMode += (float) t.lowC / (float) t.period;
            uHigh += (float) t.highC / (float) t.period;
        }
    }

    float lamda = uHighLowMode / (1 - uLow);
    float xLim = uLow + uHighLowMode / (1 - uHigh);

    for (int i = 0; i < tasks.size(); i++) {
        if (tasks[i].crit == High) {
            taskStates[i].schedulingDeadline = tasks[i].period * lamda;
        }
    }

    reset();

    for (int time = 0; time <= maxTime; time++) {

        if (time % quantum == 0 ||
            runningId >= 0 && (taskStates[runningId].exeTime >= tasks[runningId].exeTimes[taskStates[runningId].exeNum] ||
                    (taskStates[runningId].exeTime > tasks[runningId].lowC && mode == LowMode) ||
                    time > taskStates[runningId].absoluteDeadline)) {

            switches += 2;

            if (runningId >= 0 && taskStates[runningId].exeTime >= tasks[runningId].exeTimes[taskStates[runningId].exeNum]) {
                completeTask(runningId, true);
                runningId = -1;
            }

            if (runningId >= 0 && taskStates[runningId].exeTime > tasks[runningId].lowC && mode == LowMode) {
                mode = HighMode;
                for (int i = 0; i < tasks.size(); i++) {
                    if (tasks[i].crit == High && taskStates[i].state != Idle) {
                        taskStates[i].schedulingDeadline = taskStates[i].wakeupTime + tasks[i].period;
                    } else if (tasks[i].crit == Low && taskStates[i].state != Idle) {
                        completeTask(i, false);
                    }
                }
            }

            for (int i = 0; i < tasks.size(); i++) {
                if ((taskStates[i].state == Ready || taskStates[i].state == Running) && time > taskStates[i].absoluteDeadline) {
                    completeTask(i, false);
                    if (i == runningId) {
                        runningId = -1;
                    }
                }
            }

            for (int i = 0; i < tasks.size(); i++) {
                if (taskStates[i].state == Idle && time >= taskStates[i].wakeupTime) {
                    if (mode == HighMode && tasks[i].crit == Low) {
                        completeTask(i, false);
                    } else {
                        taskStates[i].state = Ready;
                        if (mode == LowMode && tasks[i].crit == High) {
                            taskStates[i].schedulingDeadline =
                                    taskStates[i].wakeupTime + (int) (tasks[i].period * lamda);
                        } else {
                            taskStates[i].schedulingDeadline = taskStates[i].wakeupTime + tasks[i].period;
                        }
                        taskStates[i].absoluteDeadline = taskStates[i].wakeupTime + tasks[i].period;
                    }
                }
            }

            if (mode == HighMode && runningId >= 0 && tasks[runningId].crit == Low) {
                taskStates[runningId].state = Ready;
                runningId = -1;
            }

            int minId = runningId;
            for (int i = 0; i < tasks.size(); i++) {
                if (taskStates[i].state == Ready && (mode == LowMode || tasks[i].crit != Low) &&
                    (minId < 0 || taskStates[i].schedulingDeadline < taskStates[minId].schedulingDeadline)) {
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

            if (runningId == -1 && mode == HighMode) {
                mode = LowMode;
                for (int i = 0; i < tasks.size(); i++) {
                    if (taskStates[i].state == Ready && (runningId < 0 || taskStates[i].schedulingDeadline < taskStates[runningId].schedulingDeadline)) {
                        runningId = i;
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

void EDFVD::completeTask(int id, bool success) {
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
