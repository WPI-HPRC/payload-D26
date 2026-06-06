from pathlib import Path
import re
import sys


ROOT = Path(__file__).resolve().parents[1]
STATE_DIR = ROOT / "src" / "payload_states"

STATES = {
    "PAYLOAD_SELF_RIGHTING": ("payloadSelfRightingInit", "payloadSelfRightingLoop", "PayloadSelfRighting.cpp"),
    "PAYLOAD_DEPLOYING": ("payloadDeployingInit", "payloadDeployingLoop", "PayloadDeploying.cpp"),
    "PAYLOAD_DEPLOYED": ("payloadDeployedInit", "payloadDeployedLoop", "PayloadDeployed.cpp"),
    "PAYLOAD_CONNECTING": ("payloadConnectingInit", "payloadConnectingLoop", "PayloadConnecting.cpp"),
    "PAYLOAD_ROV": ("payloadROVInit", "payloadROVLoop", "PayloadROV.cpp"),
    "PAYLOAD_AUTONOMOUS": ("payloadAutonomousInit", "payloadAutonomousLoop", "PayloadAutonomous.cpp"),
    "CONVENTION_DEMO": ("conventionDemoInit", "conventionDemoLoop", "ConventionDemo.cpp"),
    "PAYLOAD_IDLE": ("payloadIdleInit", "payloadIdleLoop", "PayloadIdle.cpp"),
}


def require(condition, message):
    if not condition:
        print(f"FAIL: {message}")
        return False
    return True


def main():
    ok = True
    states_h = (STATE_DIR / "States.h").read_text()
    states_cpp = (STATE_DIR / "States.cpp").read_text()

    for state_id, (init_func, loop_func, filename) in STATES.items():
        ok &= require(re.search(rf"\b{state_id}\b", states_h) is not None, f"{state_id} missing from StateID")
        ok &= require(re.search(rf"\bvoid\s+{init_func}\s*\(", states_h) is not None, f"{init_func} missing declaration")
        ok &= require(re.search(rf"\bStateID\s+{loop_func}\s*\(", states_h) is not None, f"{loop_func} missing declaration")
        ok &= require(f"initFuncs[{state_id}] = &{init_func};" in states_cpp, f"{state_id} init not attached")
        ok &= require(f"loopFuncs[{state_id}] = &{loop_func};" in states_cpp, f"{state_id} loop not attached")

        state_file = STATE_DIR / filename
        ok &= require(state_file.exists(), f"{filename} missing")
        if state_file.exists():
            contents = state_file.read_text()
            ok &= require(re.search(rf"\bvoid\s+{init_func}\s*\(", contents) is not None, f"{init_func} missing definition")
            ok &= require(re.search(rf"\bStateID\s+{loop_func}\s*\(", contents) is not None, f"{loop_func} missing definition")
            ok &= require(re.search(rf"\breturn\s+{state_id}\s*;", contents) is not None, f"{loop_func} does not self-transition")

    if ok:
        print("PASS: all payload states are declared, attached, and self-transitioning.")
        return 0
    return 1


if __name__ == "__main__":
    sys.exit(main())
