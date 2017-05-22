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

n = ARGV[0].to_i || 1
toto = ARGV[2].to_i || 1
base_filename = ARGV[1] || "scenario"

NTHREADS = 192

(0..toto).each do |i|
    current_scenario = { "scenarii" => { "params" => {}, "data" => {}, "actions" => [] } }
    current_scenario["name"] = "_#{i}"
    scenario = current_scenario["scenarii"]
    (0..i).each do |sid|
        create_data(scenario, "bs", "int", 512)
        init_core = sid * 8
        ["a#{sid}", "b#{sid}", "c#{sid}"].each do |name|
            create_data(scenario, name, "double*")
            create_action(scenario, "init_blas_bloc", init_core, 1, false, name, "bs")
        end

        create_action(scenario, "dgemm", init_core, 20, true, "a#{sid}", "b#{sid}", "c#{sid}", "bs")
        #if init_core > i
            #create_action(scenario, "dummy", init_core, 1, true)
        #end
    end
    File.open(base_filename + current_scenario.delete("name") + ".yml", 'w') do |file|
        file.write(current_scenario.to_yaml)
    end
end

