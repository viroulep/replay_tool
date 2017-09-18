require 'yaml'

require_relative './util.rb'

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
    (get_phy_core_from_i(machine, i)+1)%96
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
  end


  cores.each do |core|
    # If we're doing init by shifting cores, we need to spawn all compute kernels afterwards
    actual_core = get_phy_core_from_i(machine, core)
    compute_core_used << actual_core
    arg_names = args.map { |name| "#{name}#{actual_core}" }
    create_action(scenario, kernel_name, actual_core, repeat, true, *arg_names, "bs")
  end
  init_core_used.uniq!
  compute_core_used.uniq!
  (init_core_used-compute_core_used).each do |c|
    create_action(scenario, "dummy", c, 1, true)
  end
  current_scenario
end
