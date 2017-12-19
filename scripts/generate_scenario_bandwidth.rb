require 'yaml'

require_relative './util-generation.rb'

args = ["a", "b"]

n_elems = ARGV[0].to_i
machine = ARGV[1] || "brunch"



case machine
when "idchire"
  nodes = (0..23)
when "brunch"
  nodes = (0..4)
when "brunch-cod"
  nodes = (0..8)
when "idkat"
  nodes = (0..4)
else
  raise "Unrecognized machine"
end

base_filename = "bandwidth_#{n_elems*8}"
puts "Filename: #{base_filename}"

papi = ["PAPI_L3_TCM", "PAPI_L3_DCR", "PAPI_L3_DCW"]

nodes.each do |src|
  current_scenario = { "scenarii" => { "params" => {}, "data" => {}, "actions" => [] } }
  scenario = current_scenario["scenarii"]
  create_data(scenario, "a", "double*")
  create_data(scenario, "b", "double*")
  create_data(scenario, "n_elems", "int", n_elems)
  create_data(scenario, "random", "bool", false)
  current_scenario["scenarii"]["name"] = "#{base_filename}_#{src}"
  current_scenario["scenarii"]["watchers"] = { "time" => ["toto"], "size" => ["n_elems"] }
  create_action(scenario, "init_array", get_first_core_from_node(machine, src), 1, false, false, "a", "n_elems", "random")
  core_used = nodes.map { |n| get_first_core_from_node(machine, n) }
  core_used << get_first_core_from_node(machine, src)+1
  nodes.each do |dest|
    core = if src == dest
             get_first_core_from_node(machine, dest) + 1
           else
             get_first_core_from_node(machine, dest)
           end
    create_action(scenario, "init_array", core, 1, false, false, "b", "n_elems", "random")
    create_action(scenario, "copy", core, 50, true, true, "a", "b", "n_elems")
    (core_used-[core]).each do |c|
      create_action(scenario, "dummy", c, 1, true, false)
    end
  end
  File.open(current_scenario["scenarii"]["name"].tr(' ', '_') + ".yml", 'w') do |file|
    file.write(current_scenario.to_yaml)
  end
end
