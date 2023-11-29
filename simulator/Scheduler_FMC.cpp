#include "Scheduler.h"

using namespace std;

FMC::FMC(const vector<Task> &tasksIn) : Scheduler(tasksIn) {}

void FMC::schedule(int quantum, int maxTime) {
    int runningId = -1;

    std::vector<TaskState> taskStates;
    CritState mode = LowMode;

    float uHigh = 0.0f;
    float uHighLowMode = 0.0f;
    float uLow = 0.0f;

    for (const Task& t : tasks) {
        taskStates.emplace_back(TaskState {Ready, 0, t.period, 0, 0});
        if (t.crit == Low || t.crit == Interrupt) {
            uLow += (float) t.lowC / (float) t.period;
        }
        if (t.crit == High || t.crit == Interrupt){
            uHighLowMode += (float) t.lowC / (float) t.period;
            uHigh += (float) t.highC / (float) t.period;
        }
    }

    float lamda = uHighLowMode / (1 - uLow);

    for (int i = 0; i < tasks.size(); i++) {
        if (tasks[i].crit == High) {
            taskStates[i].schedulingDeadline = tasks[i].period * lamda;
        } else {
            taskStates[i].schedulingDeadline = tasks[i].period;
        }
    }

    switches = failedHigh = failedLow = succeedHigh = succeedLow = 0;

    for (int time = 0; time <= maxTime; time++) {

        bool interrupt = false;
        if (runningId < 0 || tasks[runningId].crit != Interrupt) {
            for (int i = 0; i < tasks.size(); i++) {
                if (tasks[i].crit == Interrupt && taskStates[i].state == Idle && time >= taskStates[i].wakeupTime) {
                    interrupt = true;
                    break;
                }
            }
        }

        if (time % quantum == 0 || interrupt ||
            runningId >= 0 && (taskStates[runningId].exeTime >= tasks[runningId].exeTimes[taskStates[runningId].exeNum] ||
                    (taskStates[runningId].exeTime > tasks[runningId].lowC && mode == LowMode) ||
                    time >= taskStates[runningId].absoluteDeadline)) {

            switches++;

            if (runningId >= 0 && taskStates[runningId].exeTime >= tasks[runningId].exeTimes[taskStates[runningId].exeNum]) {
                if (tasks[runningId].crit == Low) {
                    succeedLow++;
                } else if (tasks[runningId].crit == High) {
                    succeedHigh++;
                } else {
                    succeedInt++;
                }
                taskStates[runningId].exeNum++;
                taskStates[runningId].state = Idle;
                taskStates[runningId].wakeupTime += tasks[runningId].period;
                taskStates[runningId].exeTime = 0;
                runningId = -1;
            }

            if (runningId >= 0 && taskStates[runningId].exeTime > tasks[runningId].lowC && mode == LowMode) {
                mode = HighMode;
                for (int i = 0; i < tasks.size(); i++) {
                    if (tasks[i].crit == High && taskStates[i].state != Idle) {
                        taskStates[i].schedulingDeadline = taskStates[i].wakeupTime + tasks[i].period;
                    }
                }
            }

            for (int i = 0; i < tasks.size(); i++) {
                if ((taskStates[i].state == Ready || taskStates[i].state == Running) && time >= taskStates[i].absoluteDeadline) {
                    taskStates[i].exeNum++;
                    taskStates[i].state = Idle;
                    taskStates[i].wakeupTime += tasks[i].period;
                    taskStates[i].exeTime = 0;
                    if (tasks[i].crit == Low) {
                        failedLow++;
                    } else if (tasks[i].crit == High) {
                        failedHigh++;
                    } else {
                        failedInt++;
                    }
                    if (i == runningId) {
                        runningId = -1;
                    }
                }
            }

            for (int i = 0; i < tasks.size(); i++) {
                if (taskStates[i].state == Idle && time >= taskStates[i].wakeupTime) {
                    taskStates[i].state = Ready;
                    if (mode == LowMode && tasks[i].crit == High) {
                        taskStates[i].schedulingDeadline = taskStates[i].wakeupTime + (int)(tasks[i].period * lamda);
                    } else {
                        taskStates[i].schedulingDeadline = taskStates[i].wakeupTime + tasks[i].period;
                    }
                    taskStates[i].absoluteDeadline = taskStates[i].wakeupTime + tasks[i].period;
                }
            }

            if (mode == HighMode && runningId >= 0 && tasks[runningId].crit == Low) {
                taskStates[runningId].state = Ready;
                runningId = -1;
            }

            int minId = runningId;
            if (runningId < 0 || tasks[runningId].crit != Interrupt) {
                for (int i = 0; i < tasks.size(); i++) {
                    if (taskStates[i].state == Ready && (mode == LowMode || tasks[i].crit != Low) &&
                        (minId < 0 || taskStates[i].schedulingDeadline < taskStates[minId].schedulingDeadline)) {
                        minId = i;
                    }
                    if (taskStates[i].state == Ready && tasks[i].crit == Interrupt) {
                        minId = i;
                        break;
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
            if (runningId == -1 && mode == HighMode) {
                mode = LowMode;
                for (int i = 0; i < tasks.size(); i++) {
                    if (taskStates[i].state == Ready && (runningId < 0 || taskStates[i].absoluteDeadline < taskStates[runningId].absoluteDeadline)) {
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
