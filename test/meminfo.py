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


def rss_extractor(m):
  if "Rss" in m and is_int(m["Rss"]):
    return int(m["Rss"])
  return 0


def private_extractor(m):
  result = 0
  if "Private_Clean" in m and is_int(m["Private_Clean"]):
    result += int(m["Private_Clean"])
  if "Private_Dirty" in m and is_int(m["Private_Dirty"]):
    result += int(m["Private_Dirty"])
  return result


def shared_extractor(m):
  result = 0
  if "Shared_Clean" in m and is_int(m["Shared_Clean"]):
    result += int(m["Shared_Clean"])
  if "Shared_Dirty" in m and is_int(m["Shared_Dirty"]):
    result += int(m["Shared_Dirty"])
  return result


def extract_mappings(maps, select_func, group_func, val_func=rss_extractor):
  parts = {}
  for m in maps:
    if select_func(m) and val_func(m):
      group = group_func(m)
      if group not in parts:
        parts[group] = 0
      parts[group] += val_func(m)
  parts = parts.items()
  parts.sort(key=lambda p: -p[1])
  total = sum([p[1] for p in parts])
  return total, parts


def extract_total(maps, args):
  return extract_mappings(maps,
                          lambda m: True,
                          lambda m: "Total",
                          rss_extractor)


def extract_total_shared(maps, args):
  return extract_mappings(maps,
                          lambda m: True,
                          lambda m: "Shared",
                          shared_extractor)


def extract_total_private(maps, args):
  return extract_mappings(maps,
                          lambda m: True,
                          lambda m: "Private",
                          private_extractor)


def extract_code(maps, args):
  return extract_mappings(maps,
                          lambda m: "ex" in m["VmFlags"],
                          lambda m: m["path"],
                          private_extractor if args.private else rss_extractor)


def extract_heap(maps, args):
  return extract_mappings(maps,
                          lambda m: m["path"] == "[heap]",
                          lambda m: m["path"],
                          private_extractor if args.private else rss_extractor)


def extract_stack(maps, args):
  return extract_mappings(maps,
                          lambda m: m["path"] == "[stack]",
                          lambda m: m["path"],
                          private_extractor if args.private else rss_extractor)


def extract_mapped_files(maps, args):
  return extract_mappings(maps,
                          lambda m: "/" in m["path"] and "ex" not in m["VmFlags"],
                          lambda m: m["path"],
                          private_extractor if args.private else rss_extractor)


def extract_other(maps, args):
  return extract_mappings(maps,
                          lambda m: "/" not in m["path"] and m["path"] != "[stack]" and m["path"] != "[heap]" and "ex" not in m["VmFlags"],
                          lambda m: m["path"],
                          private_extractor if args.private else rss_extractor)


def print_histogram(name, maps, extractor_func, args):
  total, histogram = extractor_func(maps, args)
  print "{}: {:,} B".format(name, total)
  if not args.detailed:
    return
  max_size_str_len = 0
  for item, size in histogram:
    size_str = "{:,}".format(size)
    padding = max(0, max_size_str_len - len(size_str))
    max_size_str_len = max(max_size_str_len, len(size_str))
    print "  * {}{} B: {}".format(" " * padding, size_str, item)


def show_mem_info(args):
  maps = get_mappings(args.pid)
  print_histogram("Total", maps, extract_total, args)
  print_histogram("Shared", maps, extract_total_shared, args)
  print_histogram("Private", maps, extract_total_private, args)
  print "Breakdown:"
  print_histogram("* Heap", maps, extract_heap, args)
  print_histogram("* Stack", maps, extract_stack, args)
  print_histogram("* Memory mapped files", maps, extract_mapped_files, args)
  print_histogram("* Executable code", maps, extract_code, args)
  print_histogram("* Other", maps, extract_other, args)


def main():
  parser = argparse.ArgumentParser(description="Prints the memory allocation information of a process.")
  parser.add_argument("--detailed", action="store_true", help="Show a breakdown for each category.")
  parser.add_argument("--private", action="store_true", help="Do not take int account memory shared with other processes.")
  parser.add_argument("pid", help="PID of the process whose memory is to be analyzed.", type=int)
  parser.set_defaults(detailed=False)
  parser.set_defaults(private=False)
  args = parser.parse_args()
  show_mem_info(args)


if __name__ == "__main__":
  main()
