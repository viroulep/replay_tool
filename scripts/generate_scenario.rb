require 'yaml'

require_relative './util-generation.rb'

kernel = ARGV[0]
args = []

case kernel
when "dgemm"
  args = ["a", "b", "c"]
when "dtrsm", "dsyrk"
  args = ["a", "b"]
else
  raise "Unrecognized kernel"
end


blocksize = ARGV[1].to_i
remote_access = ARGV[2] || "false"
repeat = ARGV[3] || 200
remote_access = remote_access.to_s == "true"
base_filename = "#{kernel}_#{blocksize}_#{remote_access ? "remote" : "local"}"
puts "Filename: #{base_filename}"

(1..96).each do |total_cores|
  last_core = total_cores - 1
  cores = (0..last_core)
  scenario = generate_scenario("brunch", kernel, "init_blas_bloc", args, blocksize, cores, remote_access, repeat)
  scenario["scenarii"]["name"] = "#{base_filename}_#{total_cores}"
  scenario["scenarii"]["watchers"] = { "flops_dgemm" => ["bs"] }
  File.open(scenario["scenarii"]["name"].tr(' ', '_') + ".yml", 'w') do |file|
      file.write(scenario.to_yaml)
  end
end
