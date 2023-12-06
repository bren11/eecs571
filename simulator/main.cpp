#include <iostream>
#include <fstream>

#include <sstream>
#include "Scheduler.h"

using namespace std;



int main() {
    float bound, overrunP, slackRatio;
    int clockPeriods, taskSetNum, numTasks;

    taskSetNum = 1;

    vector<Scheduler*> schedulers;
    schedulers.push_back(new H_FMC());
    schedulers.push_back(new EDF());
    schedulers.push_back(new EDFVD());
    schedulers.push_back(new FMC());
    schedulers.push_back(new FMC_Drop());
    schedulers.push_back(new RED());

    vector<float> lowPFJ(schedulers.size(), 0.0f);
    vector<float> highPFJ(schedulers.size(), 0.0f);
    vector<float> switchRatios(schedulers.size(), 0.0f);

    ofstream myfile;
    myfile.open("output.txt");

    for (int fileNum = 0; fileNum < taskSetNum; fileNum++) {
        ifstream file;
        file.open("tasks/task_set_" + to_string(fileNum) + ".txt");
        file >> bound >> overrunP >> slackRatio >> clockPeriods >> taskSetNum >> numTasks;
        //if (fileNum == 0) continue;

        vector<Task> tasks;

        string line;
        getline(file, line);

        for (int i = 0; i < numTasks; i++) {
            getline(file, line);

            istringstream iss(line);

            Task t;

            char crit;
            iss >> t.period >> crit >> t.lowC >> t.highC;
            t.crit = crit == 'L' ? Low : High;
            while (!iss.eof()) {
                int val;
                iss >> val;
                t.exeTimes.push_back(val);
            }
            tasks.push_back(t);
        }

        myfile << "********************** TEST CASE: " << fileNum << " *************************\n";
        cout << "********************** TEST CASE: " << fileNum << " *************************\n";
        int switches = -1;
        for (int i = 0; i < schedulers.size(); i++) {
            Scheduler* sch = schedulers[i];
            sch->reset(tasks);
            sch->schedule(100, clockPeriods);
            lowPFJ[i] += sch->getLowPFJ();
            highPFJ[i] += sch->getHighPFJ();
            if (switches == -1) {
                switches = sch->getContextSwitches();
            }
            switchRatios[i] += (float) sch->getContextSwitches() / (float) switches;
            myfile << sch->getName() << ":\tLow PFJ: " << sch->getLowPFJ() << ",  High PFJ: " << sch->getHighPFJ() << ",  Switches: " << sch->getContextSwitches() << '\n';
        }
        /*Scheduler* sch = new FMC_Drop(tasks);
        sch->schedule(1, clockPeriods);
        cout << "Low PFJ: " << sch->getLowPFJ() << ",  High PFJ: " << sch->getHighPFJ() << ",  Switches: " << sch->getContextSwitches() << '\n';*/

        tasks.clear();
    }

    myfile << bound << " " << overrunP << " " << slackRatio << " " << clockPeriods << '\n';

    myfile << "********************** TOTALS: *************************\n";

    for (int i = 0; i < lowPFJ.size(); i++) {
        myfile << schedulers[i]->getName() << ":\tLow PFJ: " << lowPFJ[i] / taskSetNum << ",  High PFJ: " << highPFJ[i] / taskSetNum << ",  Switches: " << switchRatios[i] / taskSetNum << '\n';
    }

    for (Scheduler* sch : schedulers) {
        delete sch;
    }
    schedulers.clear();


    return 0;
}
