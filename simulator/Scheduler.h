//
// Created by brend on 11/17/2023.
//

#ifndef SIMULATOR_SCHEDULER_H
#define SIMULATOR_SCHEDULER_H

enum Criticality { Low, High, Interrupt };

class Scheduler {
public:
    virtual void addTask(int period, int deadline, Criticality level, int lowC, int highC) = 0;
};


#endif //SIMULATOR_SCHEDULER_H
