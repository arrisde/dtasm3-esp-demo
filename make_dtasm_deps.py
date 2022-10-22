import subprocess
import shutil
import os

Import("env")

'''
trigger dtasm header generation
'''
def before_build():
    cwd = os.path.join(os.getcwd(), "extern", "dtasm.git")
    subproc = subprocess.run(["make", "deps"], cwd=cwd)
    
'''
build and copy dpend_cpp.wasm
'''
def before_build_fs(source, target, env):
    cwd = os.path.join(os.getcwd(), "extern", "dtasm.git")
    subproc = subprocess.run(["make", "module/dpend_cpp/target/dpend_cpp.wasm", "CONFIG=release"], cwd=cwd)
    shutil.copy("extern/dtasm.git/module/dpend_cpp/target/dpend_cpp.wasm", "data/dpend_cpp.wasm")

env.AddPreAction("buildfs", before_build_fs)
before_build()