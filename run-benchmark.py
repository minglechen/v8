import os
import sys
import argparse
import subprocess

PMC_HEADER = ["INST_RETIRED", "CPU_CYCLES"]
PMC_ARCH_SET = PMC_HEADER + ["LD_SPEC", "ST_SPEC", "EXC_RETURN", "BR_RETURN_SPEC"]
PMC_DCACHE_SET = PMC_HEADER + ["L1D_CACHE", "L1D_CACHE_REFILL", "L2D_CACHE", "L2D_CACHE_REFILL"]
PMC_INSTR_SET = PMC_HEADER + ["L1I_CACHE", "L1I_CACHE_REFILL", "BR_MIS_PRED", "BR_PRED"]

def list_test_filenames(suite_name):
  if suite_name == "octane":
    return [
      "octane/box2d",
      "octane/code-load",
      "octane/crypto",
      "octane/deltablue",
      "octane/earley-boyer",
      "octane/gbemu-part1",
      "octane/mandreel",
      "octane/navier-stokes",
      "octane/pdfjs",
      "octane/raytrace",
      "octane/regexp",
      "octane/richards",
      "octane/splay",
      "octane/typescript",
      "octane/zlib",
    ]
  elif suite_name == "sunspider":
    return [
      "sunspider/3d-cube",
      "sunspider/3d-morph",
      "sunspider/3d-raytrace",
      "sunspider/access-binary-trees",
      "sunspider/access-fannkuch",
      "sunspider/access-nbody",
      "sunspider/access-nsieve",
      "sunspider/bitops-3bit-bits-in-byte",
      "sunspider/bitops-bits-in-byte",
      "sunspider/bitops-bitwise-and",
      "sunspider/bitops-nsieve-bits",
      "sunspider/controlflow-recursive",
      "sunspider/crypto-aes",
      "sunspider/crypto-md5",
      "sunspider/crypto-sha1",
      "sunspider/date-format-tofte",
      "sunspider/date-format-xparb",
      "sunspider/math-cordic",
      "sunspider/math-partial-sums",
      "sunspider/math-spectral-norm",
      "sunspider/regexp-dna",
      "sunspider/string-base64",
      "sunspider/string-fasta",
      "sunspider/string-tagcloud",
      "sunspider/string-unpack-code",
      "sunspider/string-validate-input",
    ]
  elif suite_name == "kraken":
    return [
      "kraken/ai-astar",
      "kraken/audio-beat-detection",
      "kraken/audio-dft",
      "kraken/audio-fft",
      "kraken/audio-oscillator",
      "kraken/imaging-darkroom",
      "kraken/imaging-desaturate",
      "kraken/imaging-gaussian-blur",
      "kraken/json-parse-financial",
      "kraken/json-stringify-tinderbox",
      "kraken/stanford-crypto-aes",
      "kraken/stanford-crypto-ccm",
      "kraken/stanford-crypto-pbkdf2",
      "kraken/stanford-crypto-sha256-iterative",
    ]

def get_files_for_test(testroot, path):
  files = []
  if path.startswith("kraken"):
    files.append(os.path.join(testroot, "%s-data.js" % path))
    files.append(os.path.join(testroot, "%s.js" % path))
  elif path.startswith("octane"):
    files.append(os.path.join(testroot, "octane/base.js"))
    files.append(os.path.join(testroot, "%s.js" % path))
    if path.startswith("octane/gbemu"):
      files.append(os.path.join(testroot, "octane/gbemu-part2.js"))
    elif path.startswith("octane/typescript"):
      files.append(os.path.join(testroot,
                                "octane/typescript-compiler.js"))
      files.append(os.path.join(testroot, "octane/typescript-input.js"))
    elif path.startswith("octane/zlib"):
      files.append(os.path.join(testroot, "octane/zlib-data.js"))
    files += ["-e", r"BenchmarkSuite.RunSuites\({}\);"]
  elif path.startswith("sunspider"):
    files.append(os.path.join(testroot, "%s.js" % path))
  return files



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
  parser.add_argument("-c", "--pmc-cumulative", action="store_true", dest="pmc_cumulative",
        help="Collect cumulative PMC data.")
  parser.add_argument("-i", "--run-individual", action="store_true", dest="run_individual",
      help="Run individual tests in benchmark.")
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
  if args.pmc_cumulative:
    prefix_cmd = "pmcstat -C"
  else:
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
  if args.run_individual:
    test_files = list_test_filenames(suite)
    testroot = benchmark_path
    for test in test_files:
      files = get_files_for_test(testroot, test)
      #print(files)
      for i in range(runs):
        run_cmd = "%s %s %s" % (d8_path, extra_args, " ".join(files))
        if args.pmc_set:
          pmc_dir = os.path.join(output_runs_path, "pmc", args.pmc_set)
          if not os.path.exists(pmc_dir):
            os.makedirs(pmc_dir)
          pmc_file = os.path.join(pmc_dir, "tests", test.split("/")[-1], "run%d.pmc" % i)
          pmc_path = "-o %s" % pmc_file
          # make sure the directory exists
          if not os.path.exists(os.path.dirname(pmc_file)):
            os.makedirs(os.path.dirname(pmc_file), exist_ok=True)
          cmdline = "%s %s %s" % (prefix_cmd, pmc_path, run_cmd)
        else:
          cmdline = run_cmd
        if args.verbose:
          print("Running %s." % cmdline)
        subprocess.call(cmdline, shell=True, cwd=suite_path)
    sys.exit(0)


  for i in range(runs):
    file_path = os.path.join(output_runs_path, "run%d" % i)
    if args.trace_gc:
      # Trace GC on last run.
      if i == runs - 1:
        inner_command = "%s --expose-gc --enable-tracing --trace-gc-heap-layout --gc-global %s %s" \
          % (d8_path, extra_args, cmd)
      file_path = os.path.join(output_path, suite + "_gc_trace")

    run_cmd = inner_command
    
    if args.pmc_set:
      pmc_dir = os.path.join(output_runs_path, "pmc", args.pmc_set)
      if not os.path.exists(pmc_dir):
        os.makedirs(pmc_dir)
      pmc_path = "-o %s" % os.path.join(pmc_dir, "run%d.pmc" % i)
      cmdline = "%s %s %s" % (prefix_cmd, pmc_path, inner_command)
    else:    
      cmdline = "%s > %s" % (run_cmd, file_path)
    if args.verbose:
      print("Running %s." % cmdline)
    subprocess.call(cmdline, shell=True, cwd=suite_path)
