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
remote_access = ARGV[3] || false
if last - first < 0 || first < 0
    raise "Can't generate less than 1 scenario"
end

NTHREADS = 192

(first..last).each do |i|
    current_scenario = { "scenarii" => { "params" => {}, "data" => {}, "actions" => [] } }
    current_scenario["name"] = "_#{i+1}"
    scenario = current_scenario["scenarii"]
    (0..i).each do |sid|
        create_data(scenario, "bs", "int", 512)
        init_core = remote_access ? (sid+8)%NTHREADS : sid
        ["a#{sid}", "b#{sid}", "c#{sid}"].each do |name|
            create_data(scenario, name, "double*")
            create_action(scenario, "init_blas_bloc", init_core, 1, false, name, "bs")
        end

        #create_action(scenario, "check_affinity", sid, 1, true)
        create_action(scenario, "dgemm", sid, 50, true, "a#{sid}", "b#{sid}", "c#{sid}", "bs")
        if init_core > i
            create_action(scenario, "dummy", init_core, 1, true)
        end
    end
    File.open(base_filename + current_scenario.delete("name") + ".yml", 'w') do |file|
        file.write(current_scenario.to_yaml)
    end
end

