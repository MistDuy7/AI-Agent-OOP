#include "harness.h"
#include <iostream>

using namespace std;

Harness::Harness(Agent& agent)
    : agent(agent) {
}

void Harness::runSmokeTest() {
    cout << "Running Agent smoke test..." << endl;

    try {
        string result =
            agent.run("Calculate 12 + 30. Use the add tool if appropriate.");

        cout << "Agent result: " << result << endl;
        cout << "Smoke test finished." << endl;
    }
    catch (const exception& e) {
        cerr << "Smoke test failed: " << e.what() << endl;
    }
}