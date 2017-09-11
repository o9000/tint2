#!/usr/bin/env python2

import argparse
import sys


# Returns true if `value` is an integer represented as a string.
def is_int(value):
  # type: (str) -> bool
  try:
    value = int(value)
  except ValueError:
    return False
  return True


def get_mappings(pid):
  mappings = []
  with open("/proc/{0}/smaps".format(pid)) as f:
    mapping = None
    for line in f:
      if ": " not in line:
        # Header
        # 00400000-00447000 r-xp 00000000 fd:00 11667056     /usr/bin/tint2
        if mapping:
          mappings.append(mapping)
        mapping = {}
        mapping["path"] = line.strip().split("  ", 1)[-1].strip()
      elif mapping:
        # Entry
        # Size:                  4 kB
        # VmFlags: rd mr mw me dw ac sd
        key, val = [s.strip() for s in line.split(":", 1)]
        if val.endswith(" kB") and is_int(val.split(" kB", 1)[0]):
          val = int(val.split(" kB", 1)[0]) * 1024
        mapping[key] = val
      else:
        print "Cannot handle line:", line
        raise RuntimeException("Parsing error")
  if mapping:
    mappings.append(mapping)
  return mappings


def extract_mappings(maps, select_func, group_func):
  parts = {}
  for m in maps:
    if "Rss" in m and is_int(m["Rss"]) and m["Rss"] > 0 and select_func(m):
      group = group_func(m)
      if group not in parts:
        parts[group] = 0
      parts[group] += m["Rss"]
  parts = parts.items()
  parts.sort(key=lambda p: -p[1])
  total = sum([p[1] for p in parts])
  return total, parts


def print_histogram(name, maps, extractor_func, detailed):
  total, histogram = extractor_func(maps)
  print "{}: {:,} B".format(name, total)
  if not detailed:
    return
  max_size_str_len = 0
  for item, size in histogram:
    size_str = "{:,}".format(size)
    padding = max(0, max_size_str_len - len(size_str))
    max_size_str_len = max(max_size_str_len, len(size_str))
    print "  * {}{} B: {}".format(" " * padding, size_str, item)


def extract_total(maps):
  return extract_mappings(maps,
                          lambda m: True,
                          lambda m: "Total")


def extract_code_size(maps):
  return extract_mappings(maps,
                          lambda m: "ex" in m["VmFlags"],
                          lambda m: m["path"])


def extract_heap_size(maps):
  return extract_mappings(maps,
                          lambda m: m["path"] == "[heap]",
                          lambda m: m["path"])


def extract_stack_size(maps):
  return extract_mappings(maps,
                          lambda m: m["path"] == "[stack]",
                          lambda m: m["path"])


def extract_mapped_files(maps):
  return extract_mappings(maps,
                          lambda m: "/" in m["path"] and "ex" not in m["VmFlags"],
                          lambda m: m["path"])


def extract_other(maps):
  return extract_mappings(maps,
                          lambda m: "/" not in m["path"] and m["path"] != "[stack]" and m["path"] != "[heap]" and "ex" not in m["VmFlags"],
                          lambda m: m["path"])


def show_mem_info(pid, detailed):
  maps = get_mappings(pid)
  print_histogram("Total", maps, extract_total, detailed)
  print_histogram("* Heap", maps, extract_heap_size, detailed)
  print_histogram("* Stack", maps, extract_stack_size, detailed)
  print_histogram("* Memory mapped files", maps, extract_mapped_files, detailed)
  print_histogram("* Executable code", maps, extract_code_size, detailed)
  print_histogram("* Other", maps, extract_other, detailed)


def main():
  parser = argparse.ArgumentParser(description="Prints the memory allocation information of a process.")
  parser.add_argument("--detailed", action="store_true", help="Show a breakdown for each category.")
  parser.add_argument("pid", help="PID of the process whose memory is to be analyzed.", type=int)
  parser.set_defaults(detailed=False)
  args = parser.parse_args()
  show_mem_info(args.pid, args.detailed)


if __name__ == "__main__":
  main()
