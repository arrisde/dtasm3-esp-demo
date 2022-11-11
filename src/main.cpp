#include "dtasm3.h"

#include "esp_spiffs.h"
#include <sys/stat.h>

#include <fstream>
#include <iterator>
#include <sstream>
#include <iostream>
#include <vector>
#include <algorithm>


using namespace dtasm3;


void print_status(const DtasmStatus status, const std::string call) {
    switch (status){
        case DtasmOK:
            std::cout << call << " returned status: OK" << std::endl;
            break;
        case DtasmDiscard:
            std::cout << call << " returned status: Discard" << std::endl;
            break;
        case DtasmWarning:
            std::cout << call << " returned status: Warning" << std::endl;
            break;
        case DtasmError:
            std::cout << call << " returned status: Error" << std::endl;
            break;
        case DtasmFatal:
            std::cout << call << " returned status: Fatal" << std::endl;
            break;
    }
}


void print_var_names(const std::vector<std::string> &out_var_names) {
    std::cout << "t";
    for (auto it = out_var_names.begin(); it != out_var_names.end(); ++it) {
        std::cout << "," << *it;
    }
    std::cout << std::endl;
}


void print_var_values(double t, 
    const std::vector<int32_t> &var_ids, 
    const std::vector<DtasmVariableType> &var_types,
    const DtasmVarValues &var_values) {

    std::cout << t;
    typedef std::vector<int32_t>::const_iterator int_iter;
    typedef std::vector<DtasmVariableType>::const_iterator type_iter;
    for (std::pair<int_iter, type_iter> i(var_ids.begin(), var_types.begin());
        i.first != var_ids.end();
        ++i.first, ++i.second) {

        switch (*i.second) {
            case DtasmReal: 
                std::cout << "," << var_values.real_values.at(*i.first);
                break;
            case DtasmInt:
                std::cout << "," << var_values.int_values.at(*i.first);
                break;
            case DtasmBool:
                std::cout << "," << var_values.bool_values.at(*i.first);
                break;
            case DtasmString:
                std::cout << "," << var_values.string_values.at(*i.first);
                break;
        }
    }
    std::cout << std::endl;
}


void check_status_ok(const DtasmStatus status, const std::string call) {
    if (status != DtasmOK) {
        std::cerr << "Non-ok status returned from doStep(): " << status << std::endl;
        exit(1);
    }
}


extern "C" void app_main() {

    double tmin = 0.0;
    double tmax = 10.0;
    int n_steps = 100;

    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = false
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            std::cout << "Failed to mount or format filesystem" << std::endl;
        } else if (ret == ESP_ERR_NOT_FOUND) {
            std::cout << "Failed to find SPIFFS partition";
        } else {
            std::cout << "Failed to initialize SPIFFS (" <<  esp_err_to_name(ret) << ")" << std::endl;
        }
        return;
    }


    struct stat st;
    if (stat("/spiffs/dpend_cpp.wasm", &st) != 0) {
        std::cout << "File dpend_cpp.wasm not found" << std::endl;
        return;
    }
    else {
        std::cout << "Reading file dpend_cpp.wasm...";
    }

    uint8_t* buffer;
    size_t buflen;
    FILE *fileptr;

    fileptr = fopen("/spiffs/dpend_cpp.wasm", "rb");
    fseek(fileptr, 0, SEEK_END);
    buflen = ftell(fileptr);
    rewind(fileptr);

    buffer = (uint8_t *)malloc(buflen * sizeof(uint8_t));
    fread(buffer, buflen, 1, fileptr);
    fclose(fileptr);

    std::cout << " read " << buflen << " bytes" << std::endl;

    Environment env((size_t)(64 * 1024));
    Module mod = env.load_module(buffer, buflen);
    Runtime rt = env.create_runtime(mod);
    auto model_desc = rt.get_model_description();
    DtasmModelInfo mi = model_desc.model;
    std::cout << "ID: " << mi.id << std::endl 
        << "Name: " << mi.name << std::endl 
        << "Description: " << mi.description << std::endl 
        << "Generating Tool: " << mi.generation_tool << std::endl;

    DtasmCapabilities cap = mi.capabilities;
    std::cout << " can_handle_variable_step_size: " << cap.can_handle_variable_step_size << std::endl
        << " can_interpolate_inputs: " << cap.can_interpolate_inputs << std::endl
        << " can_reset_step: " << cap.can_interpolate_inputs << std::endl;

    DtasmVarValues initial_vals;
    for (auto it = model_desc.variables.begin(); it != model_desc.variables.end(); ++it) {
        if (it->has_default) {
            switch (it->value_type) {
                case DtasmReal:
                    initial_vals.real_values[it->id] = it->default_.real_val;
                    break;
                case DtasmInt:
                    initial_vals.int_values[it->id] = it->default_.int_val;
                    break;
                case DtasmBool:
                    initial_vals.bool_values[it->id] = it->default_.bool_val;
                    break;
                case DtasmString:
                    initial_vals.string_values[it->id] = it->default_.string_val;
                    break;
            }
        }
    }

    auto init_status = rt.initialize(initial_vals, tmin, true, tmax, false, 0, DtasmLogInfo, false);
    print_status(init_status, "Init");

    std::vector<int32_t> out_var_ids;
    std::vector<std::string> out_var_names;
    std::vector<DtasmVariableType> out_var_types;
    for (auto it = model_desc.variables.begin(); it != model_desc.variables.end(); ++it) {
        if (it->causality == DtasmCausalityType::Output || it->causality == DtasmCausalityType::Local) {
            out_var_ids.push_back(it->id);
            out_var_names.push_back(it->name);
            out_var_types.push_back(it->value_type);
        }
    }

    DtasmVarValues set_vals_default;
    for (auto it = model_desc.variables.begin(); it != model_desc.variables.end(); ++it) {
        if (it->causality == DtasmCausalityType::Input && it->has_default) {
            switch (it->value_type) {
                case DtasmReal:
                    set_vals_default.real_values[it->id] = it->default_.real_val;
                    break;
                case DtasmInt:
                    set_vals_default.int_values[it->id] = it->default_.int_val;
                    break;
                case DtasmBool:
                    set_vals_default.bool_values[it->id] = it->default_.bool_val;
                    break;
                case DtasmString:
                    set_vals_default.string_values[it->id] = it->default_.string_val;
                    break;
            }
        }
    }

    DtasmGetValuesResponse res;
    auto get_values_status = rt.get_values(out_var_ids, res);
    check_status_ok(get_values_status, "GetValues");

    print_var_names(out_var_names);
    print_var_values(res.current_time, out_var_ids, out_var_types, res.values);

    double t = res.current_time;
    DtasmDoStepResponse do_step_res;
    DtasmStatus set_values_status;

    double dt = (tmax-t)/n_steps;

    for (int i=0; i<n_steps; ++i) {
        do_step_res = rt.do_step(t, dt);
        check_status_ok(do_step_res.status, "DoStep");
        get_values_status = rt.get_values(out_var_ids, res);
        check_status_ok(get_values_status, "GetValues");
        print_var_values(res.current_time, out_var_ids, out_var_types, res.values);
        set_values_status = rt.set_values(set_vals_default);
        check_status_ok(set_values_status, "SetValues");
        t = res.current_time;
    }
}