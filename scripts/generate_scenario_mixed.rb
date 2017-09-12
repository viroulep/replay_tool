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

base_filename = ARGV[0] || "scenario"

NTHREADS = 96
CORES_PER_NODE = 24

    current_scenario = { "scenarii" => { "params" => {}, "data" => {}, "actions" => [] } }
    scenario = current_scenario["scenarii"]
    scenario["name"] = "#{base_filename} 96"
    create_data(scenario, "bs", "int", 512)
    init_core_used = []
(0..95).each do |i|
    actual_core = (i*4)%NTHREADS + i/24
    init_core = actual_core
    init_core_used << init_core
    data = ["a#{actual_core}", "b#{actual_core}"]
    if true || i%2 == 0
        data << "c#{actual_core}"
    end
    data.each do |name|
        create_data(scenario, name, "double*")
        create_action(scenario, "init_blas_bloc", init_core, 1, false, name, "bs")
    end
    init_core_used.uniq!

    compute_core_used = []

    compute_core_used << actual_core

    #if i%2 == 0
        #create_action(scenario, "dgemm", actual_core, 50, true, "a#{actual_core}", "b#{actual_core}", "c#{actual_core}", "bs")
    #else
        #create_action(scenario, "dtrsm", actual_core, 50, true, "a#{actual_core}", "b#{actual_core}", "bs")
    #end
    create_action(scenario, "dtrsm", actual_core, 50, true, "a#{actual_core}", "b#{actual_core}", "bs")
    create_action(scenario, "dgemm", actual_core, 50, true, "a#{actual_core}", "b#{actual_core}", "c#{actual_core}", "bs")
end
File.open(scenario["name"].tr(' ', '_') + ".yml", 'w') do |file|
    file.write(current_scenario.to_yaml)
end
