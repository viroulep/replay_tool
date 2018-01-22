require 'yaml'

require_relative './util-generation.rb'

args = ["a", "b"]

n_elems = (ARGV[0] || 25000000).to_i
machine = "brunch"

# 2 levels
sources = [0, 1]

# load a single node
dest = (0..95).step(4).to_a

base_filename = "bandwidth_#{n_elems*8}"
puts "Filename: #{base_filename}"


sources.each do |src|
  dest.each do |d|
    current_scenario = { "scenarii" => { "params" => {}, "data" => {}, "actions" => [] } }
    scenario = current_scenario["scenarii"]
    create_data(scenario, "n_elems", "int", n_elems)
    create_data(scenario, "random", "bool", false)
    total_cores = (d/4)+1
    current_scenario["scenarii"]["name"] = "#{base_filename}_#{src}_#{total_cores}"
    current_scenario["scenarii"]["watchers"] = { "time" => ["toto"], "size" => ["n_elems"] }
    used_cores = []
    used_cores << src
    all_cores = (0..d).step(4).to_a
    all_cores.each do |core|
      used_cores << core
      create_data(scenario, "a#{core}", "double*")
      create_data(scenario, "b#{core}", "double*")
      create_action(scenario, "init_array", src, 1, false, false, "a#{core}", "n_elems", "random")
      create_action(scenario, "init_array", core, 1, false, false, "b#{core}", "n_elems", "random")
    end
    used_cores.uniq!
    used_cores.each do |c|
      create_action(scenario, "dummy", c, 1, true, false)
    end
    all_cores.each do |core|
      create_action(scenario, "copy", core, 10, true, false, "a#{core}", "b#{core}", "n_elems")
    end

    (used_cores-all_cores).each do |c|
      create_action(scenario, "dummy", c, 1, true, false)
    end

    File.open(current_scenario["scenarii"]["name"].tr(' ', '_') + ".yml", 'w') do |file|
      file.write(current_scenario.to_yaml)
    end
  end
end
