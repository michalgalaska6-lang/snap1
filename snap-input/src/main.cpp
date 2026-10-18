// Survivable Input Snap - v0.1 scaffold
//
// Idea: when a click arrives, simulate a few steps ahead. If the click as-is
// would kill the player, try delaying it by 1..W steps and take the first
// offset that survives.
//
// Scope of v0.1:
//   * practice mode only (completions there are not submitted)
//   * forward-only (delays the input; never moves it earlier)
//   * NOT compiled or tested: every function in the "ENGINE ADAPTER" block
//     uses names that must be checked against the Geode bindings for your
//     GD version.

#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <vector>

using namespace geode::prelude;

namespace snap {
    inline bool simulating = false;  // true while the lookahead runs
    inline bool simDied    = false;  // set by the destroyPlayer hook
    inline int  stepCount  = 0;      // our own step counter

    struct Pending {
        int  applyAtStep;
        bool down;
        int  button;
        bool isP1;
    };
    inline std::vector<Pending> pending;

    inline bool active(PlayLayer* pl) {
        return pl
            && pl->m_isPracticeMode // practice only
            && Mod::get()->getSettingValue<bool>("enabled");
    }
}

// ---- Death detection during simulation -----------------------------------
class $modify(SnapPlayLayer, PlayLayer) {
    void destroyPlayer(PlayerObject* p, GameObject* o) {
        if (snap::simulating) {
            snap::simDied = true;   // record the death, don't actually die
            return;
        }
        PlayLayer::destroyPlayer(p, o);
    }

    void resetLevel() {
        snap::pending.clear();
        snap::stepCount = 0;
        PlayLayer::resetLevel();
    }
};

// ---- Input interception ---------------------------------------------------
class $modify(SnapGameLayer, GJBaseGameLayer) {

    // ===== ENGINE ADAPTER: verify each of these against your bindings =====

    // Capture full state. Practice checkpoints are the easiest base, but they
    // don't cover everything (some trigger state, RNG, moving objects).
    CheckpointObject* takeSnapshot(PlayLayer* pl) {
        return pl->createCheckpoint();
    }

    void restoreSnapshot(PlayLayer* pl, CheckpointObject* cp) {
        pl->loadFromCheckpoint(cp);
    }

    // Advance exactly one physics step with no visual side effects.
    // Placeholder: must be replaced with a real single-step call.
    void stepOnce(PlayLayer* pl) {
        // TODO: call the game's per-step physics routine here.
    }

    // ===== Solver =========================================================

    // Returns true if applying the input after `delay` steps survives `horizon` steps.
    bool survives(PlayLayer* pl, int delay, int horizon,
                  bool down, int button, bool isP1) {
        auto snapshot = takeSnapshot(pl);
        snap::simulating = true;
        snap::simDied = false;

        for (int i = 0; i < horizon && !snap::simDied; ++i) {
            if (i == delay) {
                GJBaseGameLayer::handleButton(down, button, isP1);
            }
            stepOnce(pl);
        }

        snap::simulating = false;
        bool ok = !snap::simDied;
        restoreSnapshot(pl, snapshot);
        return ok;
    }

    void handleButton(bool down, int button, bool isP1) {
        auto pl = PlayLayer::get();
        if (snap::simulating || !snap::active(pl)) {
            return GJBaseGameLayer::handleButton(down, button, isP1);
        }

        int window  = (int)Mod::get()->getSettingValue<int64_t>("window");
        int horizon = (int)Mod::get()->getSettingValue<int64_t>("horizon");

        // delay 0 first: if the click as-is survives, don't touch it.
        for (int d = 0; d <= window; ++d) {
            if (!survives(pl, d, horizon, down, button, isP1)) continue;

            if (d == 0) {
                return GJBaseGameLayer::handleButton(down, button, isP1);
            }
            snap::pending.push_back({snap::stepCount + d, down, button, isP1});
            return;
        }

        // Nothing survives: pass the input through untouched.
        GJBaseGameLayer::handleButton(down, button, isP1);
    }

    // Drain delayed inputs. If your bindings run several physics steps per
    // call, hook the per-step function instead so stepCount stays per-step.
    void processCommands(float dt) {
        GJBaseGameLayer::processCommands(dt);
        if (snap::simulating) return;

        ++snap::stepCount;
        for (auto it = snap::pending.begin(); it != snap::pending.end();) {
            if (it->applyAtStep <= snap::stepCount) {
                GJBaseGameLayer::handleButton(it->down, it->button, it->isP1);
                it = snap::pending.erase(it);
            } else {
                ++it;
            }
        }
    }
};
