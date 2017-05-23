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

(0..7).each do |i|
    current_scenario = { "scenarii" => { "params" => {}, "data" => {}, "actions" => [] } }
    #i+1 is the number of threads per node used
    current_scenario["name"] = "_#{i+1}"
    scenario = current_scenario["scenarii"]
    create_data(scenario, "bs", "int", 512)
    (0..23).each do |node_first_core|
        (0..i).each do |sid|
            init_core = node_first_core * 8 + sid
            ["a#{init_core}", "b#{init_core}", "c#{init_core}"].each do |name|
                create_data(scenario, name, "double*")
                create_action(scenario, "init_blas_bloc", init_core, 1, false, name, "bs")
            end

            create_action(scenario, "dgemm", init_core, 50, true, "a#{init_core}", "b#{init_core}", "c#{init_core}", "bs")
            #if init_core > i
                #create_action(scenario, "dummy", init_core, 1, true)
            #end
        end
    end
    File.open("local_spread" + current_scenario.delete("name") + ".yml", 'w') do |file|
        file.write(current_scenario.to_yaml)
    end
end

