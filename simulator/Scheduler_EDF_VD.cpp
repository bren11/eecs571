#include "Scheduler.h"

using namespace std;

EDFVD::EDFVD(const vector<Task> &tasksIn) : EDF(tasksIn) {}

void EDFVD::schedule(int quantum, int maxTime) {
    int runningId = -1;

    std::vector<TaskState> taskStates;
    CritState mode = LowMode;

    float uHigh = 0.0f;
    float uHighLowMode = 0.0f;
    float uLow = 0.0f;

    for (const Task& t : tasks) {
        taskStates.emplace_back(TaskState {Ready, 0, t.period, 0, 0});
        uHigh += (float) t.highC / (float) t.period;
        uLow += (float) t.lowC / (float) t.period;
    }

    switches = failedHigh = failedLow = succeedHigh = succeedLow = 0;

    int lastTime = 0;
    for (int time = 0; time <= maxTime; time = (time / quantum + 1) * quantum) {
        switches += 2;
        if (runningId >= 0) {
            taskStates[runningId].exeTime += time - lastTime;
        }
        if (runningId >= 0 && taskStates[runningId].exeTime >= tasks[runningId].exeTimes[taskStates[runningId].exeNum]) {
            time -= taskStates[runningId].exeTime - tasks[runningId].exeTimes[taskStates[runningId].exeNum];
            if (time >= taskStates[runningId].absoluteDeadline) {
                time = taskStates[runningId].absoluteDeadline;
                if (tasks[runningId].crit == Low) {
                    failedLow++;
                } else {
                    failedHigh++;
                }
            } else {
                if (tasks[runningId].crit == Low) {
                    succeedLow++;
                } else {
                    succeedHigh++;
                }
            }
            taskStates[runningId].exeNum++;
            taskStates[runningId].state = Idle;
            taskStates[runningId].wakeupTime += tasks[runningId].period;
            taskStates[runningId].exeTime = 0;
            runningId = -1;
        }
        lastTime = time;
        for (int i = 0; i < tasks.size(); i++) {
            if ((taskStates[i].state == Ready || taskStates[i].state == Running) && time >= taskStates[i].absoluteDeadline) {
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
            if (taskStates[i].state == Ready && (minId < 0 || taskStates[i].absoluteDeadline < taskStates[minId].absoluteDeadline)) {
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
}
