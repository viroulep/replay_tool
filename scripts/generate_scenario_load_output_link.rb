require 'yaml'

require_relative './util-generation.rb'

args = ["a", "b"]

n_elems = (ARGV[0] || 25000000).to_i
machine = "idchire"

# 4 levels
threads = ((8..23).to_a + (80..87).to_a).flatten

base_filename = "bandwidth_output_link_#{n_elems*8}"
puts "Filename: #{base_filename}"

papi = ["PAPI_L3_TCM", "PAPI_L3_DCR", "PAPI_L3_DCW"]

threads.each do |t_max|
  current_scenario = { "scenarii" => { "params" => {}, "data" => {}, "actions" => [] } }
  scenario = current_scenario["scenarii"]
  create_data(scenario, "n_elems", "int", n_elems)
  create_data(scenario, "random", "bool", false)
  threads_used = threads.select { |t| t <= t_max }
  current_scenario["scenarii"]["name"] = "#{base_filename}_0_#{threads_used.size}"
  current_scenario["scenarii"]["watchers"] = { "time" => ["toto"], "size" => ["n_elems"] }
  used_cores = []
  used_cores << 0
  threads_used.each do |core|
    used_cores << core
    create_data(scenario, "a#{core}", "double*")
    create_data(scenario, "b#{core}", "double*")
    create_action(scenario, "init_array", 0, 1, false, false, "a#{core}", "n_elems", "random")
    create_action(scenario, "init_array", core, 1, false, false, "b#{core}", "n_elems", "random")
  end
  used_cores.uniq!
  used_cores.each do |c|
    create_action(scenario, "dummy", c, 1, true, false)
  end
  threads_used.each do |core|
    create_action(scenario, "copy", core, 10, true, false, "a#{core}", "b#{core}", "n_elems")
  end

  (used_cores-threads_used).each do |c|
    create_action(scenario, "dummy", c, 1, true, false)
  end

  File.open(current_scenario["scenarii"]["name"].tr(' ', '_') + ".yml", 'w') do |file|
    file.write(current_scenario.to_yaml)
  end
end
