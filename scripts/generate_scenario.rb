require 'yaml'

def create_data(scenario, name, type, value=0)
    scenario["data"][name] = { "type" => type , "value" => value }
end

def create_action(scenario, name, core, repeat, sync, *params)
    scenario["actions"] << {
        "kernel" => name,
        "core" => core,
        "repeat" => repeat,
        "sync" => sync,
        "params" => [*params],
    }
end

def sync(scenario)
    scenario["actions"] << "sync"
end

first = ARGV[0].to_i || 0
last = ARGV[1].to_i || 0
base_filename = ARGV[2] || "scenario"
remote_access = ARGV[3] || "false"
remote_access = remote_access.to_s == "true"
puts "Remote access: #{remote_access}"
if last - first < 0 || first < 0
    raise "Can't generate less than 1 scenario"
end

NTHREADS = 96
CORES_PER_NODE = 24

(first..last).each do |i|
    current_scenario = { "scenarii" => { "params" => {}, "data" => {}, "actions" => [] } }
    scenario = current_scenario["scenarii"]
    scenario["name"] = "#{base_filename} #{i+1}"
    create_data(scenario, "bs", "int", 256)
    init_core_used = []
    (0..i).each do |sid|
        actual_core = (sid*4)%NTHREADS + sid/24
        init_core = remote_access ? (actual_core+1)%NTHREADS : actual_core
        init_core_used << init_core
        ["a#{actual_core}", "b#{actual_core}", "c#{actual_core}"].each do |name|
            create_data(scenario, name, "double*")
            create_action(scenario, "init_blas_bloc", init_core, 1, false, name, "bs")
        end
    end
    init_core_used.uniq!

    compute_core_used = []

    # If we're doing init by shifting cores, we need to spawn all compute kernels afterwards
    (0..i).each do |sid|
        actual_core = (sid*4)%NTHREADS + sid/24
        compute_core_used << actual_core
        #create_action(scenario, "check_affinity", sid, 1, true)
        create_action(scenario, "dgemm", actual_core, 50, true, "a#{actual_core}", "b#{actual_core}", "c#{actual_core}", "bs")
    end
    compute_core_used.uniq!
    (init_core_used-compute_core_used).each do |c|
        create_action(scenario, "dummy", c, 1, true)
    end
    File.open(scenario["name"].tr(' ', '_') + ".yml", 'w') do |file|
        file.write(current_scenario.to_yaml)
    end
end

