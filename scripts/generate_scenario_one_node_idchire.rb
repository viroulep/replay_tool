require 'yaml'

require_relative './util-generation.rb'

kernel = ARGV[0]
args = []

case kernel
when "dgemm"
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
repeat = ARGV[3] || 50
machine = ARGV[4] || "idchire"

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

["full", "half", "none"].each do |fill|
  cores_local, cores_remote = if fill == "full"
                                [(0..7), []]
                              elsif fill == "half"
                                [(0..3), (4..7)]
                              else
                                [[], (0..7)]
                              end
  init_name = "init_blas_bloc"
  current_scenario = { "scenarii" => { "params" => {}, "data" => {}, "actions" => [] } }
  scenario = current_scenario["scenarii"]
  create_data(scenario, "bs", "int", blocksize)
  init_core_used = []
  compute_core_used = []
  cores_local.each do |core|
    actual_core = get_phy_core_from_i(machine, core)
    init_core = actual_core
    init_core_used << init_core
    args.each do |name|
      arg_name = "#{name}#{actual_core}"
      create_data(scenario, arg_name, "double*")
      create_action(scenario, init_name, init_core, 1, false, false, arg_name, "bs")
    end
  end

  cores_local.each do |core|
    # If we're doing init by shifting cores, we need to spawn all compute kernels afterwards
    actual_core = get_phy_core_from_i(machine, core)
    compute_core_used << actual_core
    arg_names = args.map { |name| "#{name}#{actual_core}" }
    create_action(scenario, kernel, actual_core, repeat, true, false, *arg_names, "bs")
  end

  cores_remote.each do |core|
    actual_core = get_phy_core_from_i(machine, core)
    init_core = get_phy_core_on_next_node_from_i(machine, core)
    init_core_used << init_core
    args.each do |name|
      arg_name = "#{name}#{actual_core}"
      create_data(scenario, arg_name, "double*")
      create_action(scenario, init_name, init_core, 1, false, false, arg_name, "bs")
    end
  end

  cores_remote.each do |core|
    # If we're doing init by shifting cores, we need to spawn all compute kernels afterwards
    actual_core = get_phy_core_from_i(machine, core)
    compute_core_used << actual_core
    arg_names = args.map { |name| "#{name}#{actual_core}" }
    create_action(scenario, kernel, actual_core, repeat, true, false, *arg_names, "bs")
  end

  init_core_used.uniq!
  compute_core_used.uniq!
  (init_core_used-compute_core_used).each do |c|
    create_action(scenario, "dummy", c, 1, true, false)
  end

  #scenario = generate_scenario(machine, kernel, init_name, args, blocksize, cores, remote_access, repeat)
  current_scenario["scenarii"]["name"] = "#{base_filename}_#{fill}_8"
  current_scenario["scenarii"]["watchers"] = { "flops_#{kernel}" => ["bs"], "time" => ["toto"] }
  #unless remote_access
    #scenario["scenarii"]["watchers"]["papi"] = papi
  #end
  #scenario["scenarii"]["watchers"] = { "flops_dgemm" => ["bs"], "papi" => ["PAPI_TOT_CYC"] }
  #scenario["scenarii"]["watchers"] = { "flops_dgemm" => ["bs"] }
  File.open(current_scenario["scenarii"]["name"].tr(' ', '_') + ".yml", 'w') do |file|
      file.write(current_scenario.to_yaml)
  end
end
