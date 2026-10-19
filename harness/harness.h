#pragma once

#include "../agent/agent.h"

class Harness {
public:
    explicit Harness(Agent& agent);

    void runSmokeTest();

private:
    Agent& agent;
};