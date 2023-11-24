#include <iostream>
#include <fstream>

#include <sstream>
#include "Scheduler.h"

using namespace std;



int main() {
    float bound, overrunP, slackRatio;
    int clockPeriods, taskSetNum, numTasks;

    taskSetNum = 1;

    for (int fileNum = 0; fileNum < taskSetNum; fileNum++) {

        ifstream file;
        file.open("tasks/task_set_" + to_string(fileNum) + ".txt");
        file >> bound >> overrunP >> slackRatio >> clockPeriods >> taskSetNum >> numTasks;

        vector<Task> tasks;

        string line;
        getline(file, line);

        for (int i = 0; i < numTasks; i++) {
            getline(file, line);

            istringstream iss(line);

            Task t;

            char crit;
            iss >> t.period >> crit >> t.lowC >> t.highC;
            t.crit = crit == 'L' ? Low : crit == 'H' ? High : Interrupt;
            while (!iss.eof()) {
                int val;
                iss >> val;
                t.exeTimes.push_back(val);
            }
            tasks.push_back(t);
        }

        Scheduler* sch = new EDF(tasks);
        sch->schedule(10000, clockPeriods);
        cout << "Low PFJ: " << sch->getLowPFJ() << ",  High PFJ: " << sch->getHighPFJ() << ",  Int PFJ: " << sch->getIntPFJ() << ",  Switches: " << sch->getContextSwitches() << '\n';
        sch->schedule(1, clockPeriods);
        cout << "Low PFJ: " << sch->getLowPFJ() << ",  High PFJ: " << sch->getHighPFJ() << ",  Int PFJ: " << sch->getIntPFJ() << ",  Switches: " << sch->getContextSwitches() << '\n';
    }



    return 0;
}
