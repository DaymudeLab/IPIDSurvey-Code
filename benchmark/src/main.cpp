#include <sched.h>

#include <chrono>
#include <cstdlib>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <boost/program_options.hpp>

#include "packet.h"
#include "globalipid.h"
#include "perconnipid.h"
#include "perdestipid.h"
#include "perbucketlipid.h"
#include "perbucketmipid.h"
#include "prngqueueipid.h"
#include "prngshuffleipid.h"
#include "perbucketshuffleipid.h"

namespace bpo = boost::program_options;

// Command line arguments defined here so we don't have to pass them around.

static std::vector<int> cpus;
static std::string pkt_fname, results_path, ipid_method;
static uint32_t method_arg, num_trials, trial_duration, warmup, max_cpus;

/* ================================ CSV I/O ================================= */

std::vector<Packet> load_packets(std::string pkt_fname, std::string src_addr) {
  std::vector<Packet> packets;

  // Open CSV and stop if it wasn't opened.
  std::ifstream input(pkt_fname);
  if (!input.is_open()) {
    std::cerr << "Couldn't open \'" << pkt_fname << "\'." << std::endl;
    std::exit(EXIT_FAILURE);
  }

  // Parse rows as packets. Assumes that the row format is: Protocol Number,
  // TCP Flags, IP Identifier, Source IP Address, Source Port, Destination IP
  // Address, Destination Port.
  bool first_row = true;
  for (std::string line; std::getline(input, line); ) {
    // Skip the header row.
    if (first_row) {
      first_row = false;
      continue;
    }

    // Tokenize the line by comma delimiter. Replace empty tokens with "0"; in
    // this dataset that usually means missing port numbers.
    std::vector<std::string> row;
    std::istringstream ss(std::move(line));
    for (std::string val; std::getline(ss, val, ','); ) {
      row.push_back((val != "") ? val : "0");
    }
    if (row.size() < 7) {  // Trailing comma, missing destination port.
      row.push_back("0");
    }

    // Turn the row data into a Packet if all values are specified.
    packets.push_back(Packet(src_addr, row[5], row[4], row[6], row[0]));
  }

  return packets;
}

// Write the IPID assignment count results for a given IPID method and number of
// CPUs to CSV, where rows are trials and columns are CPU counts.
void store_results(const std::vector<std::vector<uint64_t>>& results,
                   uint32_t num_cpus) {
  // Create up results/ directory if one does not already exist.
  std::filesystem::path rpath = results_path;
  std::filesystem::create_directory(rpath);

  // Get the results filename "<method><method_arg?>_<num_cpus>.csv".
  std::ostringstream oss;
  oss << ipid_method;
  std::set<std::string> need_args{"perdest", "perbucketl", "perbucketm",
                                  "prngqueue", "prngshuffle"};
  if (need_args.contains(ipid_method)) {
    oss << method_arg;
  }
  oss << "_" << num_cpus << ".csv";
  rpath /= oss.str();

  // Open CSV and stop if it wasn't opened.
  std::ofstream output(rpath);
  if (!output.is_open()) {
    std::cerr << "Couldn't open \'" << rpath << "\'." << std::endl;
    std::exit(EXIT_FAILURE);
  }

  // Write data to file.
  for (size_t i = 0; i < results.size(); i++) {  // Trial i.
    for (size_t j = 0; j < num_cpus; j++) {      // CPU j.
      if (j == 0) {
        output << results[i][j];
      } else {
        output << "," << results[i][j];
      }
    }
    output << std::endl;
  }

  // Close the file.
  output.close();
}

/* ============================= CPU ASSIGNMENT ============================= */

// Thread affinity works off of bitmasking. If a thread has a 1 in bit i, then
// this tells the operating system "this thread can be run on CPU i".

// Linux-specific function (requiring sched_getaffinity from sched.h) that gets
// the number of CPUs as reported by the OS. Modified from Travis Downs:
// https://github.com/travisdowns/concurrency-hierarchy-bench/blob/master/bench.cpp#L701
std::vector<int> get_cpus() {
  // Get current thread affinity.
  cpu_set_t cpuset;
  if (sched_getaffinity(0, sizeof(cpuset), &cpuset)) {
    std::cerr << "ERROR: Could not get thread affinity." << std::endl;
    std::exit(EXIT_FAILURE);
  }

  // Count CPUs.
  std::vector<int> cpus;
  std::cout << "Getting CPUs: [";
  for (int cpu = 0; cpu < CPU_SETSIZE; cpu++) {
    if (CPU_ISSET(cpu, &cpuset)) {
      cpus.push_back(cpu);
      std::cout << " " << cpu;
    }
  }
  std::cout << " ]" << std::endl;

  return cpus;
}

// Linux-specific function (requiring sched_setaffinity from sched.h) that pins
// the thread that calls it to the specified CPU. Sourced from Travis Downs:
// https://github.com/travisdowns/concurrency-hierarchy-bench/blob/master/bench.cpp#L342
void pin_to_cpu(int cpu) {
  // Clear the current thread affinity and then set it to our desired CPU.
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(cpu, &cpuset);

  // Set the new thread affinity and error if this fails.
  if (sched_setaffinity(0, sizeof(cpuset), &cpuset) == -1) {
    std::cerr << "ERROR: Could not pin thread to CPU " << cpu << std::endl;
    std::exit(EXIT_FAILURE);
  }
}

/* ========================= TRIAL/THREAD LAUNCHING ========================= */

template<typename T>
void trial_thread(uint32_t thread_id, T& ipid_method,
                  std::vector<Packet>& packets,
                  std::vector<uint64_t>& trial_results) {
  // Randomly sample a packet index in the list to start assigning IPIDs at.
  std::mt19937 rng_mt((std::random_device())());
  std::uniform_int_distribution<uint32_t> dist(0, packets.size() - 1);
  uint32_t pkt_idx = dist(rng_mt);

  // Assign as many IPIDs as possible in the time allotted. During the warmup
  // period, don't count the number of IPIDs assigned.
  auto t_start = std::chrono::steady_clock::now();
  do {
    ipid_method.get_ipid(packets[pkt_idx]);
    pkt_idx = (pkt_idx + 1) % packets.size();
  } while (std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now() - t_start)
           < std::chrono::milliseconds(warmup));

  // Now that we're warm, start counting.
  t_start = std::chrono::steady_clock::now();
  uint64_t ipids_assigned = 0;
  do {
    ipid_method.get_ipid(packets[pkt_idx]);
    pkt_idx = (pkt_idx + 1) % packets.size();
    ipids_assigned++;
  } while (std::chrono::duration_cast<std::chrono::seconds>(
               std::chrono::steady_clock::now() - t_start)
           < std::chrono::seconds(trial_duration));

  trial_results[thread_id] = ipids_assigned;
}

template<typename T>
std::vector<uint64_t> trial(uint32_t num_threads, T& ipid_method,
                            std::vector<Packet>& packets) {
  // Set up vectors of threads and results.
  std::vector<std::thread> threads(num_threads);
  std::vector<uint64_t> trial_results(num_threads);

  // Spawn one thread per CPU.
  for (uint32_t t = 0; t < num_threads; t++) {
    threads[t] = std::thread(&trial_thread<T>, t, std::ref(ipid_method),
                             std::ref(packets), std::ref(trial_results));
    pin_to_cpu(cpus[t]);
  }

  // Wait for all threads to finish.
  for (auto& t: threads) {
    t.join();
  }

  return trial_results;
}

void do_trials(uint32_t num_threads, std::vector<Packet>& packets) {
  std::cout << std::string(21, '=') << " #IPIDs assigned for " << num_threads
            << " CPUs " << std::string(21, '=') << std::endl;

  // Set up results vector of vectors (indexed by trial then thread).
  std::vector<std::vector<uint64_t>> results(
    num_trials, std::vector<uint64_t>(num_threads));

  // Construct the specified a IPID selection method and launch a single trial.
  for (uint32_t t = 0; t < num_trials; t++) {
    if (ipid_method == "global") {
      GlobalIPID global = GlobalIPID();
      results[t] = trial(num_threads, global, packets);
    } else if (ipid_method == "perconn") {
      PerConnIPID perconn = PerConnIPID();
      results[t] = trial(num_threads, perconn, packets);
    } else if (ipid_method == "perdest") {
      PerDestIPID perdest = PerDestIPID(method_arg);
      results[t] = trial(num_threads, perdest, packets);
    } else if (ipid_method == "perbucketl") {
      PerBucketLIPID perbucketl = PerBucketLIPID(method_arg);
      results[t] = trial(num_threads, perbucketl, packets);
    } else if (ipid_method == "perbucketm") {
      PerBucketMIPID perbucketm = PerBucketMIPID(method_arg);
      results[t] = trial(num_threads, perbucketm, packets);
    } else if (ipid_method == "prngqueue") {
      PRNGQueueIPID prngqueue = PRNGQueueIPID(method_arg);
      results[t] = trial(num_threads, prngqueue, packets);
    } else if (ipid_method == "prngshuffle") {
      PRNGShuffleIPID prngshuffle = PRNGShuffleIPID(method_arg);
      results[t] = trial(num_threads, prngshuffle, packets);
    } else if (ipid_method == "perbucketshuffle") {
      PerBucketShuffleIPID perbuckets = PerBucketShuffleIPID(method_arg);
      results[t] = trial(num_threads, perbuckets, packets);
    }

    // Print the results of this trial.
    std::cout << "[T" << t+1 << "]:";
    for (auto ipid_count : results[t]) {
      std::cout << "  " << ipid_count;
    }
    std::cout << std::endl;
  }

  std::cout << std::string(70, '=') << std::endl;

  // Write results to file.
  store_results(results, num_threads);
}

int main(int argc, char** argv) {
  // Parse command line arguments.
  bpo::options_description desc("optional arguments:");
  desc.add_options()
      ("help,h", "Show this help message and exit")
      ("pkt_fname,f", bpo::value<std::string>(&pkt_fname)->
        default_value("packets.csv"), "Filepath to packets CSV")
      ("results_path,r", bpo::value<std::string>(&results_path)->
        default_value("results"), "Write results to <results_path>/*.csv")
      ("ipid_method,m", bpo::value<std::string>(&ipid_method)->
        default_value("global"), "IPID selection method")
      ("method_arg,a", bpo::value<uint32_t>(&method_arg)->default_value(4096),
        "Max. # destinations for per-destination, # buckets for per-bucket, # reserved IPIDs for PRNG-based methods")
      ("num_trials,t", bpo::value<uint32_t>(&num_trials)->default_value(1),
        "Number of benchmark trials per # CPUs")
      ("trial_duration,d", bpo::value<uint32_t>(&trial_duration)->
        default_value(5), "Duration of a single trial in seconds")
      ("warmup,w", bpo::value<uint32_t>(&warmup)->
        default_value(100), "Duration of a trial warmup in milliseconds")
      ("max_cpus,c", bpo::value<uint32_t>(&max_cpus)->default_value(4),
        "Maximum # CPUs to benchmark on");

  bpo::variables_map vm;
  bpo::store(bpo::parse_command_line(argc, argv, desc), vm);
  bpo::notify(vm);

  // Print help message if requested and stop.
  if (vm.count("help")) {
      std::cout << desc << std::endl;
      return 1;
  }

  // Get available CPUs (variable stored statically).
  cpus = get_cpus();

  // Validate parsed arguments.
  bool valid = true;
  std::set<std::string> methods{"global", "perconn", "perdest", "perbucketl",
                                "perbucketm", "prngqueue", "prngshuffle",
                                "perbucketshuffle"};
  if (!methods.contains(ipid_method)) {
    std::cerr << "ERROR: IPID selection method must be one of:\n";
    for (auto method : methods) {
      std::cerr << "    - " << method << std::endl;
    }
    valid = false;
  }
  if (ipid_method == "perdest" &&
      (method_arg != pow(2, 12) && method_arg != pow(2, 15))) {
    std::cerr << "ERROR: Purge threshold must be in {2^12, 2^15}" << std::endl;
    valid = false;
  } else if ((ipid_method == "perbucketl" || ipid_method == "perbucketm") &&
             (method_arg < pow(2, 11) || method_arg > pow(2, 18))) {
    std::cerr << "ERROR: # buckets must be in [2^11, 2^18]" << std::endl;
    valid = false;
  } else if ((ipid_method == "prngqueue" || ipid_method == "prngshuffle") &&
             (method_arg < pow(2, 12) || method_arg > pow(2, 15))) {
    std::cerr << "ERROR: # reserved IPIDs must be in [2^12, 2^15]" << std::endl;
    valid = false;
  } else if (ipid_method == "perbucketshuffle" &&
             (method_arg < 2 || method_arg > 16)) {
    std::cerr << "ERROR: # buckets must be in [2, 16]" << std::endl;
    valid = false;
  }
  if (num_trials == 0) {
    std::cerr << "ERROR: # trials must be > 0" << std::endl;
    valid = false;
  }
  if (trial_duration == 0) {
    std::cerr << "ERROR: Trials must last > 0 seconds" << std::endl;
    valid = false;
  }
  if (warmup < 10 || warmup > (trial_duration * 1000) / 2) {
    std::cerr << "ERROR: Warmups must be in [10, "
              << (trial_duration * 1000) / 2 << "] ms" << std::endl;
    valid = false;
  }
  if (max_cpus < 1 || max_cpus > cpus.size()) {
    std::cerr << "ERROR: Maximum # CPUs must be in [1, " << cpus.size() << "]"
              << std::endl;
    valid = false;
  }

  // Error out and stop if there were invalid command line arguments.
  if (!valid) {
    return 1;
  }

  // Read packets' header information. Note that we fix the source IP address
  // because, ostensibly, all of these packets are supposed to be sent by the
  // same server that's assigning them IPIDs.
  std::cout << "Reading packets' header data..." << std::endl;
  std::vector<Packet> packets = load_packets(pkt_fname, "169.67.224.76");

  // Run trials for all numbers of CPUs.
  std::cout << "Starting " << ipid_method << " trials..." << std::endl;
  for (uint32_t num_cpus = 1; num_cpus <= max_cpus; num_cpus++) {
    do_trials(num_cpus, packets);
  }

  return 0;
}
