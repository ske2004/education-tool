#!/bin/python
import os
import sys
import shutil

class Arguments:
  def __init__(self, target, switches):
    self.target = target
    self.switches = switches

class CompileProperties:
  def __init__(self):
    self.test_mode = False
    self.testbed = False
    self.release_mode = False
    self.runtime_mode = False
    self.testbed_name = ""
    self.run_mode = False
    self.prof = False

  def set_test(self):
    self.test_mode = True

  def set_testbed(self, name):
    self.testbed = True
    self.testbed_name = name

  def set_runtime(self):
    self.runtime_mode = True

  def set_release(self):
    self.release_mode = True

  def set_run(self):
    self.run_mode = True

  def set_prof(self):
    self.prof = True

def parse_arguments(argv):
  target = ""
  switches = []

  for arg in argv:
    if arg.startswith("-"):
      switches.append(arg[1:])
    else:
      if target == "":
        target = arg
      else:
        raise Exception(f"Conflicting targets: {target} and {arg}")

  if target == "":
    raise Exception("No target specified")

  return Arguments(target, switches)

system_features = {
  "has_emcc": False
}

def target_build_wasm():
  if os.name != "nt" and system_features["has_emcc"]:
    return "./scripts/build-wasm.sh"
  else:
    if os.name == "nt":
      raise Exception("Can not build WASM on Windows, please, install WSL2 with Emscripten SDK")
    if not system_features["has_emcc"]:
      raise Exception("Can not build WASM because you need to install Emscripten SDK")


def target_build(p: CompileProperties):
  test_string = ""
  if p.test_mode and p.testbed:
    test_string += f"-DENV_TYPE=TESTBED -DTESTBED_NAME={p.testbed_name}"
  elif p.test_mode:
    test_string += "-DENV_TYPE=UNITTEST"
  elif p.runtime_mode:
    test_string += "-DENV_TYPE=RUNTIME"
  else:
    test_string += "-DENV_TYPE=NORMAL"

  command = ""
  mode_string = "Release" if p.release_mode else "Debug"
  print(f"Building in {mode_string} mode")

  if os.name == "nt":
    # NOTE: The new line is important, because using && in the end of if
    # statement will only execute the rest if the if statement is true.
    command += "if not exist bin mkdir bin\r\n"
    command += "lib\\sokol-tools-bin\\bin\\win32\\sokol-shdc.exe --input catedu/shaders/amalgamation.glsl --output catedu/shaders.hxx --slang hlsl5 "
    command += "&& cd bin "
    command += f"&& cmake -G Ninja -DCMAKE_BUILD_TYPE={mode_string} {test_string} .. "
    command += f"&& cmake --build ."
    command += "&& cd .. "
    if p.prof:
      sleepy = "C:\\PROGRA~1\\VERYSL~1\\sleepy.exe"
      if not os.path.isfile(sleepy):
        raise Exception("Very Sleepy is not installed, it is required for profiling on Windows")
      command += f"&& {sleepy} /r .\\bin\\educore.exe"
      print(command)
    elif p.run_mode:
      command += f"&& .\\bin\\educore.exe"
  elif sys.platform == "darwin":
    command += "mkdir -p bin "
    command += "&& lib/sokol-tools-bin/bin/osx/sokol-shdc --input catedu/shaders/amalgamation.glsl --output catedu/shaders.hxx --slang metal_macos "
    command += "&& cd bin "
    command += f"&& cmake -DCMAKE_BUILD_TYPE={mode_string} {test_string} .. "
    command += "&& make "
    command += "&& cd .. "
    if p.prof:
      raise Exception("Profiling is not yet supported on macOS")
    elif p.run_mode:
      command += "&& ./bin/educore"
  else:
    command += "mkdir -p bin "
    command += "&& lib/sokol-tools-bin/bin/linux/sokol-shdc --input catedu/shaders/amalgamation.glsl --output catedu/shaders.hxx --slang glsl410 "
    command += "&& cd bin "
    command += f"&& cmake -DCMAKE_BUILD_TYPE={mode_string} {test_string} .. "
    command += "&& make -j4 "
    command += "&& cd .. "
    if p.prof:
      raise Exception("Profiling is not yet supported on Linux/other")
    elif p.run_mode:
      command += "&& ./bin/educore"
  return command

def init_system_features():
  if shutil.which("emcc") is not None:
    system_features["has_emcc"] = True

def main():
  init_system_features()

  arguments = parse_arguments(sys.argv[1:])
  script = ""

  p = CompileProperties()

  if "release" in arguments.switches:
    p.set_release()
  if "prof" in arguments.switches:
    p.set_prof()
  if "runtime" in arguments.switches:
    p.set_runtime()

  if arguments.target == "up":
    script = "git pull"
    script += "&& git submodule update --init --recursive"
    script += "&& git lfs install"
    script += "&& git lfs pull"
  elif arguments.target == "build-wasm":
    script = target_build_wasm()
  elif arguments.target == "test":
    p.set_test()
    p.set_run()
  elif arguments.target == "testbed":
    p.set_testbed(arguments.switches[0])
    p.set_test()
    p.set_run()
  elif arguments.target == "run":
    p.set_run()
  elif arguments.target == "build":
    pass
  elif arguments.target == "version":
    with open("docs/version", "rb") as f:
      print(f.read().decode("utf-8").strip())
    exit(0)
  else:
    raise Exception(f"Unknown target: {arguments.target}")

  if arguments.target != "build-wasm" and arguments.target != "up":
    script = target_build(p)

  lines = script.split('\n')
  for line in lines:
    status = os.system(line)
    if status != 0:
      exit(status)

if __name__ == "__main__":
  main()
