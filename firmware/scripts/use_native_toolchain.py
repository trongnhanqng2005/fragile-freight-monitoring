import os
import sys

Import("env")

# Windows builds use the project-local MSYS2 UCRT64 toolchain. Other hosts
# (for example Ubuntu CI) keep the system gcc/g++ found by PlatformIO.
if sys.platform == "win32":
    project_root = os.path.dirname(env.subst("$PROJECT_DIR"))
    toolchain_bin = os.path.join(project_root, ".tools", "msys64", "ucrt64", "bin")

    if not os.path.isfile(os.path.join(toolchain_bin, "gcc.exe")) or not os.path.isfile(
        os.path.join(toolchain_bin, "g++.exe")
    ):
        raise RuntimeError(
            "Project-local MSYS2 UCRT64 GCC/G++ not found at " + toolchain_bin
        )

    os.environ["PATH"] = toolchain_bin + os.pathsep + os.environ.get("PATH", "")
    env.PrependENVPath("PATH", toolchain_bin)
    env.Append(LINKFLAGS=["-static"])
