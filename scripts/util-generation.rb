require 'yaml'

require_relative './util.rb'

first = ARGV[0].to_i || 0
last = ARGV[1].to_i || 0
base_filename = ARGV[2] || "scenario"
remote_access = ARGV[3] || "false"
remote_access = remote_access.to_s == "true"
puts "Remote access: #{remote_access}"
if last - first < 0 || first < 0
    raise "Can't generate less than 1 scenario"
end

def get_phy_core_from_i(machine, i)
  case machine
  when "brunch"
    # On brunch adjacent cores are 0,4,8,...
    (i*4)%96 + i/24
  else
    i
  end
end

def get_phy_core_on_next_node_from_i(machine, i)
  case machine
  when "brunch"
    (i+1)%96
  else
    i
  end
end

def generate_scenario(machine, kernel_name, init_func, args, blocksize, cores, shift_init, repeat)
  current_scenario = { "scenarii" => { "params" => {}, "data" => {}, "actions" => [] } }
  scenario = current_scenario["scenarii"]
  create_data(scenario, "bs", "int", blocksize)
  init_core_used = []
  compute_core_used = []
  cores.each do |core|
    actual_core = get_phy_core_from_i(machine, core)
    init_core = shift_init ? get_phy_core_on_next_node_from_i(machine, core) : actual_core
    init_core_used << init_core
    arg_names = []
    args.each do |name|
      arg_name = "#{name}#{actual_core}"
      arg_names << arg_name
      create_data(scenario, arg_name, "double*")
      create_action(scenario, init_func, init_core, 1, false, arg_name, "bs")
    end


    # If we're doing init by shifting cores, we need to spawn all compute kernels afterwards
    actual_core = get_phy_core_from_i(machine, core)
    compute_core_used << actual_core
    create_action(scenario, kernel_name, actual_core, repeat, true, *arg_names, "bs")
  end
  init_core_used.uniq!
  compute_core_used.uniq!
  (init_core_used-compute_core_used).each do |c|
    create_action(scenario, "dummy", c, 1, true)
  end
  current_scenario
end
