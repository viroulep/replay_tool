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

NTHREADS = 192
base_filename = ARGV[0] || "scenario"
remote_access = ARGV[1] || "false"
remote_access = remote_access.to_s == "true"

(0..7).each do |i|
    current_scenario = { "scenarii" => { "params" => {}, "data" => {}, "actions" => [] } }
    #i+1 is the number of threads per node used
    scenario = current_scenario["scenarii"]
    scenario["name"] = "#{base_filename}_spread #{i+1}"
    create_data(scenario, "bs", "int", 512)
    (0..23).each do |node_first_core|
        (0..i).each do |sid|
            init_core = remote_access ? (((node_first_core+1)*8)%192 + sid) : (node_first_core * 8 + sid)
            ["a#{init_core}", "b#{init_core}", "c#{init_core}"].each do |name|
                create_data(scenario, name, "double*")
                create_action(scenario, "init_blas_bloc", init_core, 1, false, name, "bs")
            end
        end
    end

    (0..23).each do |node_first_core|
        (0..i).each do |sid|
            init_core = node_first_core * 8 + sid
            create_action(scenario, "dgemm", init_core, 50, true, "a#{init_core}", "b#{init_core}", "c#{init_core}", "bs")
            #if init_core > i
                #create_action(scenario, "dummy", init_core, 1, true)
            #end
        end
    end
    File.open(scenario["name"].tr(' ', '_') + ".yml", 'w') do |file|
        file.write(current_scenario.to_yaml)
    end
end

