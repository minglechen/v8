import os
import sys
import argparse
import subprocess

PMC_HEADER = ["INST_RETIRED", "CPU_CYCLES"]
PMC_ARCH_SET = PMC_HEADER + ["LD_SPEC", "ST_SPEC", "EXC_RETURN", "BR_RETURN_SPEC"]
PMC_DCACHE_SET = PMC_HEADER + ["L1D_CACHE", "L1D_CACHE_REFILL", "L2D_CACHE", "L2D_CACHE_REFILL"]
PMC_INSTR_SET = PMC_HEADER + ["L1I_CACHE", "L1I_CACHE_REFILL", "BR_MIS_PRED", "BR_PRED"]

if __name__ == "__main__":
  parser = argparse.ArgumentParser(description="Run a benchmark suite.")
  parser.add_argument("-r", "--runs", dest="runs",
      help="Override the default number of runs for the benchmark.")
  parser.add_argument("-x", "--extra-arguments", dest="extra_args",
      help="Pass these extra arguments to d8.")
  parser.add_argument("-t", "--trace-gc", action="store_true", dest="trace_gc",
        help="Trace GC events.")
  parser.add_argument("-p", "--pmc-set", dest="pmc_set",
        help="Collect PMC data. Options are: arch, dcache, instr.")
  parser.add_argument("-v", "--verbose", action="store_true", dest="verbose",
      help="See more output about what magic csuite is doing.")
  parser.add_argument("suite_name", help="The benchmark suite to run.")
  parser.add_argument("d8_path", help="The path to the d8 executable.")
  parser.add_argument("csuite_path", help="The path to the csuite directory.")

  args = parser.parse_args()
  suite = args.suite_name
  # Set up paths.
  d8_path = os.path.abspath(args.d8_path)
  if not os.path.exists(d8_path):
    print(d8_path + " is not valid.")
    sys.exit(1)

  csuite_path = os.path.abspath(args.csuite_path)
  if not os.path.exists(csuite_path):
    print("The csuite directory is invalid.")
    sys.exit(1)

  benchmark_path = os.path.abspath(os.path.join(csuite_path, "../data"))
  if not os.path.exists(benchmark_path):
    print("I can't find the benchmark data directory. Aborting.")
    sys.exit(1)

  # Gather the remaining arguments into a string of extra args for d8.
  extra_args = ""
  if args.extra_args:
    extra_args = args.extra_args

  if suite == "octane":
    runs = 10
    suite_path = os.path.join(benchmark_path, "octane")
    cmd = os.path.join(suite_path, "run.js")
  elif suite == "kraken":
    runs = 80
    suite_path = os.path.join(benchmark_path, "kraken")
    cmd = os.path.join(csuite_path, "run-kraken.js")
  else:
    runs = 100
    suite_path = os.path.join(benchmark_path, "sunspider")
    cmd = os.path.join(csuite_path, "sunspider-standalone-driver.js")

  if args.runs:
    if (float(args.runs) / runs) < 0.6:
      print("Normally, %s requires %d runs to get stable results." \
          % (suite, runs))
    runs = int(args.runs)

  if args.trace_gc:
    runs = 1
  if args.verbose:
    print("Running and averaging %s %d times." % (suite, runs))

  # Ensure output directory is setup
  output_path_base = os.path.abspath(os.getcwd())
  output_path = os.path.join(output_path_base, "_benchmark_results")
  output_runs_path = os.path.join(output_path, suite)

  if not os.path.exists(output_path):
    if args.verbose:
      print("Creating directory %s." % output_path)
    os.mkdir(output_path)

  if not os.path.exists(output_runs_path):
    if args.verbose:
      print("Creating directory %s." % output_runs_path)
    os.mkdir(output_runs_path)
  if args.verbose:
    print("Working directory for runs is %s." % suite_path)

  inner_command = "%s --expose-gc %s %s" \
      % (d8_path, extra_args, cmd)
  prefix_cmd = "pmcstat"
  if args.pmc_set:
    if not subprocess.check_output("kldstat | grep hwpmc", shell=True):
      print("PMC support is not enabled in the kernel.")
      sys.exit(1)
    if args.pmc_set == "arch":
      prefix_cmd += " -p " + " -p ".join(PMC_ARCH_SET)
    elif args.pmc_set == "dcache":
      prefix_cmd += " -p " + " -p ".join(PMC_DCACHE_SET)
    elif args.pmc_set == "instr":
      prefix_cmd += " -p " + " -p ".join(PMC_INSTR_SET)
    else:
      print("Invalid PMC set. Options are: arch, dcache, instr.")
      sys.exit(1)

  
  # Run the benchmark.
  for i in range(runs):
    file_path = os.path.join(output_runs_path, "run%d" % i)
    if args.trace_gc:
      # Trace GC on last run.
      if i == runs - 1:
        inner_command = "%s --expose-gc --enable-tracing --trace-gc-heap-layout %s %s" \
          % (d8_path, extra_args, cmd)
      file_path = os.path.join(output_path, suite + "_gc_trace")

    run_cmd = inner_command
    
    if args.pmc_set:
      pmc_dir = os.path.join(output_runs_path, "pmc", args.pmc_set)
      if not os.path.exists(pmc_dir):
        os.mkdir(pmc_dir)
      pmc_path = "-o %s" % os.path.join(pmc_dir, "run%d.pmc" % i)
      cmdline = "%s %s %s" % (prefix_cmd, pmc_path, inner_command)
    else:    
      cmdline = "%s > %s" % (run_cmd, file_path)
    if args.verbose:
      print("Running %s." % cmdline)
    subprocess.call(cmdline, shell=True, cwd=suite_path)
