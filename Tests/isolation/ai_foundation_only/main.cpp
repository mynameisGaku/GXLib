/// @file Tests/isolation/ai_foundation_only/main.cpp
/// @brief Compile- + link-time isolation check for GXLib_AI (E1 regression).
///
/// This TU includes only AI headers and calls only AI public API. It is
/// built as an executable that links ONLY `GXLib_AI` + Foundation + Core
/// — NO `GXLib_Graphics`. If the ADR-0018 §Constraints "Foundation-only"
/// claim regresses (i.e., someone adds a Graphics include to AI/*.cpp
/// again), the link step of this target will fail with unresolved
/// symbols from the Graphics chain.
///
/// Added 2026-04-19 as the final piece of sprint-002 Task 6 (deferred
/// test hardening for the 2026-04-18 arch-fix epic).
///
/// Note: DebugDraw / BuildFromTerrain are intentionally NOT called here
/// — those live in GXLib_AIDebug and require Graphics. This TU tests
/// that the CORE AI surface has no Graphics dependency.

#include "AI/NavMesh.h"
#include "AI/NavMesh3D.h"
#include "AI/NavAgent.h"
#include "AI/BehaviorTree.h"
#include "AI/GOAPPlanner.h"
#include "AI/RVOSolver.h"

#include <cstdio>

int main()
{
    // Exercise a minimal slice of each AI layer. This is a smoke link
    // check, not a behavioural test — successful linkage is the success
    // condition; GXLibTests.exe covers the behaviour.

    // NavMesh (2D grid)
    gx::NavMesh navmesh;
    const bool built = navmesh.Build(-10.0f, -10.0f, 10.0f, 10.0f, 0.5f);
    if (!built) return 1;

    gx::Vector<gx::Vector3> path;
    (void)navmesh.FindPath({ -5, 0, -5 }, { 5, 0, 5 }, path);

    // NavMesh3D (voxel)
    gx::NavMesh3D nav3d;
    (void)nav3d.Build({ -10, -10, -10 }, { 10, 10, 10 }, 1.0f);

    // NavAgent
    gx::NavAgent agent;
    agent.Initialize(&navmesh);
    agent.SetDestination({ 5, 0, 5 });
    agent.Update(0.016f);

    // BehaviorTree
    gx::BehaviorTree bt;
    (void)bt.Tick(0.016f);

    // GOAPPlanner
    gx::GOAPPlanner planner;

    // RVO — stateless free function
    gx::Vector<gx::RVO::AgentState> others;
    (void)gx::RVO::ComputeAvoidanceVelocity(
        gx::RVO::AgentState{}, { 1, 0, 0 }, others, 2.0f);

    std::puts("AI foundation-only isolation link check: OK");
    return 0;
}
