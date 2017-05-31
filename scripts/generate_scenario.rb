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
CORES_PER_NODE = 12

(first..last).each do |i|
    current_scenario = { "scenarii" => { "params" => {}, "data" => {}, "actions" => [] } }
    scenario = current_scenario["scenarii"]
    scenario["name"] = "#{base_filename} #{i+1}"
    (0..i).each do |sid|
        create_data(scenario, "bs", "int", 256)
        init_core = remote_access ? (sid+CORES_PER_NODE)%NTHREADS : sid
        ["a#{sid}", "b#{sid}", "c#{sid}"].each do |name|
            create_data(scenario, name, "double*")
            create_action(scenario, "init_blas_bloc", init_core, 1, false, name, "bs")
        end
    end

    # If we're doing init by shifting cores, we need to spawn all compute kernels afterwards
    (0..i).each do |sid|
        init_core = remote_access ? (sid+CORES_PER_NODE)%NTHREADS : sid
        #create_action(scenario, "check_affinity", sid, 1, true)
        create_action(scenario, "dgemm", sid, 50, true, "a#{sid}", "b#{sid}", "c#{sid}", "bs")
        if init_core > i
            create_action(scenario, "dummy", init_core, 1, true)
        end
    end
    File.open(scenario["name"].tr(' ', '_') + ".yml", 'w') do |file|
        file.write(current_scenario.to_yaml)
    end
end

