require 'yaml'

require_relative './util-generation.rb'

kernel = ARGV[0]
args = []

case kernel
when "naive_dgemm", "dgemm"
  args = ["a", "b", "c"]
when "dtrsm", "dsyrk"
  args = ["a", "b"]
when "dpotrf"
  args = ["a"]
else
  raise "Unrecognized kernel"
end




blocksize = ARGV[1].to_i
remote_access = ARGV[2] || "false"
repeat = ARGV[3] || 200
machine = ARGV[4] || "brunch"

max_cores = 8
case machine
when "idchire"
  max_cores = 192
when "brunch"
  max_cores = 96
when "idkat"
  max_cores = 48
else
  raise "Unrecognized kernel"
end

remote_access = remote_access.to_s == "true"
base_filename = "#{kernel}_#{blocksize}_#{remote_access ? "remote" : "local"}"
puts "Filename: #{base_filename}"

papi = ["PAPI_L3_TCM", "PAPI_L3_DCR", "PAPI_L3_DCW"]

(1..max_cores).each do |total_cores|
  last_core = total_cores - 1
  cores = (0..last_core)
  init_name = if kernel == "dpotrf"
                "init_symmetric"
              else
                "init_blas_bloc"
              end
  scenario = generate_scenario(machine, kernel, init_name, args, blocksize, cores, remote_access, repeat)
  scenario["scenarii"]["name"] = "#{base_filename}_#{total_cores}"
  flops_watcher = if kernel == "naive_dgemm"
                    "flops_dgemm"
                  else
                    "flops_#{kernel}"
                  end
  scenario["scenarii"]["watchers"] = { flops_watcher => ["bs"], "time" => ["toto"] }
  unless remote_access
    scenario["scenarii"]["watchers"]["papi"] = papi
  end
  #scenario["scenarii"]["watchers"] = { "flops_dgemm" => ["bs"], "papi" => ["PAPI_TOT_CYC"] }
  #scenario["scenarii"]["watchers"] = { "flops_dgemm" => ["bs"] }
  File.open(scenario["scenarii"]["name"].tr(' ', '_') + ".yml", 'w') do |file|
      file.write(scenario.to_yaml)
  end
end
