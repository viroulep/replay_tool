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

NTHREADS = 96
base_filename = ARGV[0] || "scenario"
remote_access = ARGV[1] || "false"
remote_access = remote_access.to_s == "true"

(0..23).each do |i|
    current_scenario = { "scenarii" => { "params" => {}, "data" => {}, "actions" => [] } }
    #i+1 is the number of threads per node used
    scenario = current_scenario["scenarii"]
    scenario["name"] = "#{base_filename}_spread #{i+1}"
    create_data(scenario, "bs", "int", 256)
    last = (i+1)*4-1

    init_core_used = []
    (0..last).each do |sid|
        init_core = remote_access ? (sid+1)%96 : sid
        init_core_used << init_core
        ["a#{sid}", "b#{sid}", "c#{sid}"].each do |name|
            create_data(scenario, name, "double*")
            create_action(scenario, "init_blas_bloc", init_core, 1, false, name, "bs")
        end
    end
    init_core_used.uniq!

    compute_core_used = []

    (0..last).each do |sid|
        compute_core_used << sid
        create_action(scenario, "dgemm", sid, 50, true, "a#{sid}", "b#{sid}", "c#{sid}", "bs")
    end
    compute_core_used.uniq!
    (init_core_used-compute_core_used).each do |c|
        create_action(scenario, "dummy", c, 1, true)
    end
    File.open(scenario["name"].tr(' ', '_') + ".yml", 'w') do |file|
        file.write(current_scenario.to_yaml)
    end
end

