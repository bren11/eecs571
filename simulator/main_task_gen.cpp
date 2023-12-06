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

enum Criticality { Low, High };

struct Task {
    int period;
    Criticality crit;
    int lowC;
    int highC;
};

int main() {

    srand(time(0));

    float bound = 0.9f;
    float overrunP = .5;
    float slackRatio = 1;

    float highP = .5;

    int clockPeriods = 10000000;
    int taskSetNum = 100;

    for (int i = 0; i < taskSetNum; i++) {

        vector<Task> tasks;
        float utilizationLow = 0.0f;
        float utilizationHigh = 0.0f;

        while (utilizationLow < bound - .05f && utilizationHigh < bound - .05f) {
            Task t{};

            float type = randomFloat(0, 1);
            t.crit = type < highP ? High : Low;

            type = randomFloat(0, 1);
            float uLow;
            if (type <= .5f) {
                t.period = randomInt(200, 1000);
                uLow = randomFloat(0.05f, .15f);
            } else if (type <= 1) {
                t.period = randomInt(1000, 100000);
                uLow = randomFloat(0.01f, .05f);
            } else {
                t.period = randomInt(100000, 10000000);
                uLow = randomFloat(0.005f, .01f);
            }

            //t.period = randomInt(200, 1000);
            //float uLow = randomFloat(0.05f, .15f);

            t.lowC = (int) roundf(uLow * (float) t.period);

            if (t.crit == High) {
                t.highC = (int) roundf((float) t.lowC * randomFloat(1, 4));
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

        if (tasks.size() > 32) {
            i--;
            continue;
        }

        ofstream myfile;
        myfile.open("tasks/task_set_" + to_string(i) + ".txt");

        myfile << bound << " " << overrunP << " " << slackRatio << " " << clockPeriods << " " << taskSetNum << " " << tasks.size() << '\n';

        for (Task& t : tasks) {
            myfile << t.period << (t.crit == Low ? " L " : " H ") << t.lowC << " " << t.highC;
            for (int j = 0; j <= clockPeriods / t.period; j++) {
                int exTime = randomInt(slackRatio * t.lowC, t.lowC);
                if (t.crit == High && randomFloat(0, 1) < overrunP) {
                    exTime = randomInt(max(t.lowC, (int)(slackRatio * t.highC)), t.highC);
                }
                myfile << " " << exTime;
            }
            myfile << '\n';
        }
        myfile.close();

    }
    return 0;
}
