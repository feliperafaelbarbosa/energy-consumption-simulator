/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <iostream>
#include <wrench.h>


#include <fstream>
#include <string>

#include "SimpleWMS.h"

///usr/local/include/wrench/tools/wfcommons/WfCommonsWorkflowParser.h
#include <wrench/tools/wfcommons/WfCommonsWorkflowParser.h>

/**
 * @brief An example that demonstrate how to run a simulation of a simple Workflow
 *        Management System (WMS) (implemented in SimpleWMS.[cpp|h]).
 *
 * @param argc: argument count
 * @param argv: argument array
 * @return 0 if the simulation has successfully completed
 */

int main(int argc, char **argv)
{

    auto simulation = wrench::Simulation::createSimulation();

    simulation->init(&argc, argv);

    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <xml platform file> <workflow file> [--log=simple_wms.threshold=info]" << std::endl;
        exit(1);
    }

    /* The first argument is the platform description file, written in XML following the SimGrid-defined DTD */
    char *platform_file = argv[1];
    /* The second argument is the workflow description file, written in JSON using WfCommons's WfFormat format */
    char *workflow_file = argv[2];

    std::cerr << "Loading workflow..." << std::endl;
    std::shared_ptr<wrench::Workflow> workflow;
    workflow = wrench::WfCommonsWorkflowParser::createWorkflowFromJSON(workflow_file, "100Gf");
    std::cerr.flush();

    /* Reading and parsing the platform description file to instantiate a simulated platform */
    std::cerr << "Instantiating SimGrid platform..." << std::endl;
    simulation->instantiatePlatform(platform_file);

    /* Get a vector of all the hosts in the simulated platform */
    std::vector<std::string> hostname_list = wrench::Simulation::getHostnameList();

    /* Create a list of storage services that will be used by the WMS */
    std::set<std::shared_ptr<wrench::StorageService>> storage_services;

    /* Instantiate a storage service */
    std::cerr << "Instantiating a SimpleStorageService on WMSHost " << std::endl;
    auto storage_service = simulation->add(wrench::SimpleStorageService::createSimpleStorageService({"WMSHost"}, {"/"}));
    storage_services.insert(storage_service);

    /* Create a list of compute services that will be used by the WMS */
    std::set<std::shared_ptr<wrench::ComputeService>> compute_services;

    /* Instantiate and add to the simulation a batch_standard_and_pilot_jobs service */
    std::shared_ptr<wrench::BatchComputeService> batch_compute_service;
#ifndef ENABLE_BATSCHED
    std::string scheduling_algorithm = "conservative_bf_core_level";
#else
    std::string scheduling_algorithm = "conservative_bf";
#endif
    try
    {
        batch_compute_service = simulation->add(new wrench::BatchComputeService(
            {"BatchHeadNode"}, {{"Node1"}, {"Node2"}, {"Node3"}, {"Node4"}, {"Node5"}, {"Node6"}, {"Node7"}, {"Node8"}, {"Node9"}, {"Node10"}, {"Node11"}, {"Node12"}, {"Node13"}, {"Node14"}, {"Node15"}, {"Node16"}, {"Node17"}, {"Node18"}, {"Node19"}, {"Node20"}, {"Node21"}, {"Node22"}, {"Node23"}, {"Node24"}, {"Node25"}, {"Node26"}, {"Node27"}, {"Node28"}, {"Node29"}, {"Node30"}}, "",
            {{wrench::BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM, scheduling_algorithm}},
            {{wrench::BatchComputeServiceMessagePayload::STOP_DAEMON_MESSAGE_PAYLOAD, 2048}}));
    }
    catch (std::invalid_argument &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        std::exit(1);
    }

    /* Instantiate and add to the simulation a cloud service */
    std::shared_ptr<wrench::CloudComputeService> cloud_compute_service;
    try
    {
        cloud_compute_service = simulation->add(new wrench::CloudComputeService(
            {"CloudHeadNode"}, {{"CloudNode1"}, {"CloudNode2"}, {"CloudNode3"}}, "", {},
            {{wrench::CloudComputeServiceMessagePayload::STOP_DAEMON_MESSAGE_PAYLOAD, 1024}}));
    }
    catch (std::invalid_argument &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        std::exit(1);
    }

    std::cerr << "Instantiating a WMS on WMSHost..." << std::endl;
    auto wms = simulation->add(
        new wrench::SimpleWMS(workflow, batch_compute_service,
                              cloud_compute_service, storage_service, {"WMSHost"}));

    /* Instantiate a file registry service */
    std::string file_registry_service_host = hostname_list[(hostname_list.size() > 2) ? 1 : 0];
    std::cerr << "Instantiating a FileRegistryService on " << file_registry_service_host << "..." << std::endl;
    auto file_registry_service =
        simulation->add(new wrench::FileRegistryService(file_registry_service_host));

    /* It is necessary to store, or "stage", input files for the first task(s) of the workflow on some storage
     * service, so that workflow execution can be initiated. The getInputFiles() method of the Workflow class
     * returns the set of all workflow files that are not generated by workflow tasks, and thus are only input files.
     * These files are then staged on the storage service.
     */
    std::cerr << "Staging input files..." << std::endl;
    for (auto const &f : workflow->getInputFiles())
    {
        try
        {
            simulation->stageFile(f, storage_service);
        }
        catch (std::runtime_error &e)
        {
            std::cerr << "Exception: " << e.what() << std::endl;
            return 0;
        }
    }

    /* Enable some output time stamps */
    simulation->getOutput().enableWorkflowTaskTimestamps(true);
    simulation->getOutput().enableEnergyTimestamps(true);

    /* Launch the simulation. This call only returns when the simulation is complete. */
    std::cerr << "Launching the Simulation..." << std::endl;
    try
    {
        simulation->launch();
    }
    catch (std::runtime_error &e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 0;
    }

    simulation->getOutput().dumpWorkflowGraphJSON(workflow, "/tmp/workflow.json", true);

    std::vector<wrench::SimulationTimestamp<wrench::SimulationTimestampTaskCompletion> *> trace;
    trace = simulation->getOutput().getTrace<wrench::SimulationTimestampTaskCompletion>();

    unsigned long num_failed_tasks = 0;
    double computation_communication_ratio_average = 0.0;
    double io_time_input = 0.0;
    double io_time_output = 0.0;
    double compute_time = 0.0;
    unsigned long total_bytes_read = 0.0;
    unsigned long total_bytes_write = 0.0;
    // std::vector<double> flops_per_task;
    // std::vector<sg_size_t> memory_req_task;
    std::vector<unsigned long> cores_alloc_task;
    for (const auto &item : trace)
    {
        auto task = item->getContent()->getTask();
        if (task->getExecutionHistory().size() > 1)
        {
            num_failed_tasks++;
        }
        /* Seconds */
        io_time_input = task->getExecutionHistory().top().read_input_end - task->getExecutionHistory().top().read_input_start;
        io_time_output = task->getExecutionHistory().top().write_output_end - task->getExecutionHistory().top().write_output_start;
        compute_time = task->getExecutionHistory().top().computation_end - task->getExecutionHistory().top().computation_start;
        computation_communication_ratio_average += compute_time / (io_time_input + io_time_output);
        /* Bytes */
        total_bytes_read += task->getBytesRead();
        total_bytes_write += task->getBytesWritten();
        // memory_req_task.push_back(task->getMemoryRequirement());
        
        cores_alloc_task.push_back(task->getNumCoresAllocated());
        // flops_per_task.push_back(task->getFlops());
    }

    computation_communication_ratio_average /= (double)(trace.size());

    std::ofstream csvFile;

    csvFile.open("/home/wrench/datas/execution_output.csv", std::ios::app);

    if (!csvFile.is_open())
    {
        std::cerr << "Erro ao abrir o arquivo CSV!" << std::endl;
        std::cerr << "Erro do sistema: " << strerror(errno) << std::endl;
    }

    /* Add the header only the first time you open the file */
    if (csvFile.tellp() == 0)
    {
        csvFile << "run_id,host_name,num_of_cores,cores_allocated_task,num_of_tasks,avg_task_execution,tasks_failed,compute_time,io_input_time,io_output_time,comm_comp_ratio,total_bytes_read,total_bytes_write,completion_date,power\n";
    }

    //csvFile << std::fixed << std::setprecision(2);

    // std::ostringstream flops_stream;
    // for (size_t i = 0; i < flops_per_task.size(); ++i) {
    //     flops_stream << flops_per_task[i];
    //     if (i != flops_per_task.size() - 1) {
    //         flops_stream << ";";  // separador entre os valores
    //     }
    // }

    // std::ostringstream memory_stream;
    // for (size_t i = 0; i < memory_req_task.size(); ++i) {
    //     memory_stream << memory_req_task[i];
    //     if (i != memory_req_task.size() - 1) {
    //         memory_stream << ";";
    //     }
    // }

    std::ostringstream cores_stream;
    for (size_t i = 0; i < cores_alloc_task.size(); ++i) {
        cores_stream << cores_alloc_task[i];
        if (i != cores_alloc_task.size() - 1) {
            cores_stream << ";";
        }
    }

    int lista_de_nos = simulation->getHostnameList().size();
    for (int index = 0; index < lista_de_nos; index++)
    {
        std::string host_name = simulation->getHostnameList()[index];
        int num_tasks = workflow->getNumberOfTasks();
        int num_cores = simulation->getHostNumCores(host_name);
        std::string runId = "extk-" + std::to_string(num_tasks);
        /* return current energy consumption in joules */
        double energy_consumed = simulation->getEnergyConsumed(host_name);
        /* return a date in seconds */
        double conclusion_time = workflow->getCompletionDate();

        /* Calculate the power, joule / second */
        double power = energy_consumed / conclusion_time;

        /* Average time per task in seconds */
        double avg_task_duration = compute_time / (num_tasks - num_failed_tasks);

        csvFile << runId << ","
                << host_name << ","
                << num_cores << ","
                << cores_stream.str() << ","
                << num_tasks << ","
                << avg_task_duration << ","
                //<< memory_stream.str() << ","
                //<< trace.size() << ","
                << num_failed_tasks << ","
                << compute_time << ","
                << io_time_input << ","
                << io_time_output << ","
                << computation_communication_ratio_average << ","
                << total_bytes_read << ","
                << total_bytes_write << ","
                << conclusion_time << ","
                // << flops_stream.str() << ","
                << power << "\n";
    }
    csvFile.close();

    return 0;
}
