#!/usr/bin/env python2

from __future__ import print_function

import __builtin__

import sys
reload(sys)
sys.setdefaultencoding('utf8')
import datetime
import os
import signal
import subprocess
import time


display = "99"
devnull = open(os.devnull, "r+")
ok = ":white_check_mark:"
warning = ":warning:"
error = ":negative_squared_cross_mark:"
stress_duration = 10


def print(*args, **kwargs):
  r = __builtin__.print(*args, **kwargs)
  __builtin__.print("\n")
  return r


def run(cmd, output=False):
  return subprocess.Popen(cmd,
                          stdin=devnull,
                          stdout=devnull if not output else subprocess.PIPE,
                          stderr=devnull if not output else subprocess.STDOUT,
                          shell=isinstance(cmd, basestring),
                          close_fds=True,
                          preexec_fn=os.setsid)


def stop(p):
  os.killpg(os.getpgid(p.pid), signal.SIGTERM)


def sleep(n):
  while n > 0:
    sys.stderr.write(".")
    sys.stderr.flush()
    time.sleep(1)
    n -= 1


def start_xvfb():
  stop_xvfb()
  xvfb = run(["Xvfb", ":{0}".format(display), "-screen", "0", "1280x720x24", "-nolisten", "tcp", "-dpi", "96"])
  if xvfb.poll() != None:
    raise RuntimeError("Xvfb failed to start")
  os.environ["DISPLAY"] = ":{0}".format(display)
  return xvfb


def stop_xvfb():
  run("kill $(netstat -ap 2>/dev/null | grep X{0} | grep LISTENING | grep -o '[0-9]*/Xvfb' | head -n 1 | cut -d / -f 1) 1>/dev/null 2>/dev/null ".format(display)).wait()


def start_xsettings():
  return run(["xsettingsd", "-c", "./configs/xsettingsd.conf"])


def start_wm():
  return run(["openbox", "--replace", "--config-file", "./configs/openbox.xml"])

def start_compositor():
  return run(["compton", "--config", "./configs/compton.conf"])


def start_stressors():
  stressors = []
  stressors.append(run(["./workspaces-stress.sh"]))
  return stressors


def stop_stressors(stressors):
  for s in stressors:
    stop(s)


def compute_min_med_fps(out):
  samples = []
  for line in out.split("\n"):
    if "fps = " in line:
      fps = float(line.split("fps = ", 1)[-1].split(" ")[0])
      samples.append(fps)
  samples.sort()
  return min(samples), samples[len(samples)/2]


def get_mem_usage(pid):
  value = None
  with open("/proc/{0}/status".format(pid)) as f:
    for line in f:
      if line.startswith("VmRSS:"):
        rss = line.split(":", 1)[-1].strip()
        value, multiplier = rss.split(" ")
        value = float(value)
        if multiplier == "kB":
          value *= 1024
        else:
          raise RuntimeError("Could not parse /proc/[pid]/status")
  if not value:
    raise RuntimeError("Could not parse /proc/[pid]/status")
  return value * 1.0e-6


def find_asan_leaks(out):
  traces = []
  trace = None
  for line in out.split("\n"):
    line = line.strip()
    if " leak of " in line and " allocated from:" in line:
      trace = []
    if trace != None:
      if line.startswith("#"):
        trace.append(line)
      else:
        if any([ "tint2" in frame for frame in trace ]):
          traces.append(trace)
        trace = None
  return traces


def test(tint2path, config):
  start_xvfb()
  sleep(1)
  start_xsettings()
  start_wm()
  sleep(1)
  os.environ["DEBUG_FPS"] = "1"
  os.environ["ASAN_OPTIONS"] = "detect_leaks=1"
  tint2 = run(["tint2", "-c", config], True)
  if tint2.poll() != None:
    raise RuntimeError("tint2 failed to start")
  sleep(1)
  # Handle late compositor start
  compton = start_compositor()
  sleep(2)
  # Stress test with compositor on
  stressors = start_stressors()
  sleep(stress_duration)
  stop_stressors(stressors)
  # Handle compositor stopping
  stop(compton)
  # Stress test with compositor off
  stressors = start_stressors()
  sleep(stress_duration)
  stop_stressors(stressors)
  # Handle WM restart
  start_wm()
  # Stress test with new WM
  stressors = start_stressors()
  sleep(stress_duration)
  stop_stressors(stressors)
  # Collect info
  mem = get_mem_usage(tint2.pid)
  stop(tint2)
  out, _ = tint2.communicate()
  exitcode = tint2.returncode
  if exitcode != 0:
    print("tint2 crashed with exit code {0}!".format(exitcode))
    print("Output:")
    print("```")
    print(out)
    print("```")
    return
  min_fps, med_fps = compute_min_med_fps(out)
  leaks = find_asan_leaks(out)
  sys.stderr.write("\n")
  mem_status = ok if mem < 20 else warning if mem < 40 else error
  print("Memory usage: %.1f %s %s" % (mem, "MB", mem_status))
  leak_status = ok if not leaks else error
  print("Memory leak count:", len(leaks), leak_status)
  for leak in leaks:
    print("Memory leak:")
    for line in leak:
      print(line)
  fps_status = ok if min_fps > 60 else warning if min_fps > 40 else error
  print("FPS:", "min:", min_fps, "median:", med_fps, fps_status)
  if mem_status != ok or leak_status != ok or fps_status != ok:
    print("Output:")
    print("```")
    print(out)
    print("```")
  stop_xvfb()


def main():
  utc_datetime = datetime.datetime.utcnow()
  print("Last updated:", utc_datetime.strftime("%Y-%m-%d %H:%M UTC"))
  out, _ = run("git show -s '--format=%H %s'", True).communicate()
  print("Last commit:", out.strip())
  diff, _ = run("git diff", True).communicate()
  diff = diff.strip()
  diff_staged, _ = run("git diff --staged", True).communicate()
  diff_staged = diff_staged.strip()
  if diff or diff_staged:
    print("Repository not clean", warning)
    if diff:
      print("Diff:")
      print("```")
      print(diff)
      print("```")
    if diff_staged:
      print("Diff staged:")
      print("```")
      print(diff_staged)
      print("```")
  print("")
  print("# Compilation")
  cmake_flags = "-DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON"
  print("Flags:", cmake_flags)
  start = time.time()
  c = run("rm -rf build; mkdir build; cd build; cmake {0} ../../ ; make -j7".format(cmake_flags), True)
  out, _ = c.communicate()
  duration = time.time() - start
  if c.returncode != 0:
    print("Status: Failed!", error)
    print("Output:")
    print("```")
    print(out)
    print("```")
    return
  if "warning:" in out:
    print("Status: Succeeded with warnings!", warning)
    print("Warnings:")
    print("```")
    for line in out.split("\n"):
      if "warning:" in line:
        print(line)
    print("```")
  else:
    print("Status: Succeeded in %.1f seconds" % (duration,), ok)
  configs = []
  configs += ["./configs/tint2/" +s for s in os.listdir("./configs/tint2") ]
  configs += ["../themes/" + s for s in os.listdir("../themes")]
  index = 0
  for config in configs:
    index += 1
    print("")
    print("# Test", index)
    print("Config: [{0}]({1})".format(config.split("/")[-1].replace(".tint2rc", ""), "https://gitlab.com/o9000/tint2/blob/master/test/" + config))
    test("./build/tint2", config)

if __name__ == "__main__":
  main()
