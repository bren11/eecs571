#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>
#include <cmath>

using namespace std;

float randomFloat(float min, float max) {
    if (abs (max - min) < .000001) {
        return max;
    }
    return ((float)(rand()) / (float)(RAND_MAX)) * (max - min) + min;
}

float randomInt(int min, int max) {
    if (max == min) {
        return max;
    }
    return rand() % (max - min) + min;
}

enum Criticality { Low, High, Interrupt };

struct Task {
    int period;
    Criticality crit;
    int lowC;
    int highC;
};

int main() {

    srand(time(0));

    float bound = 0.9f;
    float overrunP = .7f;
    float slackRatio = 1;

    int clockPeriods = 10000;
    int taskSetNum = 2;

    for (int i = 0; i < taskSetNum; i++) {

        vector<Task> tasks;
        float utilizationLow = 0.0f;
        float utilizationHigh = 0.0f;

        while (utilizationLow < bound - .05f && utilizationHigh < bound - .05f) {
            Task t{};
            t.period = randomInt(100, 1000);
            t.crit = randomFloat(0, 1) > 0.5f ? Low : High;
            float uLow = randomFloat(0.05f, .15);
            t.lowC = (int) roundf(uLow * (float) t.period);
            if (t.crit == High) {
                t.highC = (int) roundf((float) t.lowC * randomFloat(1, 5));
            } else {
                t.highC = 0;
            }
            float uHigh = (float) t.highC / (float) t.period;
            uLow = (float) t.lowC / (float) t.period;
            if (uLow + utilizationLow < bound && uHigh + utilizationHigh < bound) {
                utilizationHigh += uHigh;
                utilizationLow += uLow;
                tasks.emplace_back(t);
            }
        }

        ofstream myfile;
        myfile.open("tasks/task_set_" + to_string(i) + ".txt");

        myfile << bound << " " << overrunP << " " << slackRatio << " " << clockPeriods << " " << taskSetNum << " " << tasks.size() << '\n';

        for (Task& t : tasks) {
            myfile << t.period << (t.crit == Low ? " 0 " : " 1 ") << t.lowC << " " << t.highC;
            for (int j = 0; j <= clockPeriods / t.period; j++) {
                int exTime = randomInt(slackRatio * t.lowC, t.lowC);
                if (t.crit == High && randomFloat(0, 1) < overrunP) {
                    exTime = randomInt(t.lowC, t.highC);
                }
                myfile << " " << exTime;
            }
            myfile << '\n';
        }
        myfile.close();

    }
    return 0;
}
