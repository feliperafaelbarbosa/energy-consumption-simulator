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

    /*
     * Declaration of the top-level WRENCH simulation object
     */
    auto simulation = wrench::Simulation::createSimulation();

    /*
     * Initialization of the simulation, which may entail extracting WRENCH-specific and
     * Simgrid-specific command-line arguments that can modify general simulation behavior.
     * Two special command-line arguments are --help-wrench and --help-simgrid, which print
     * details about available command-line arguments.
     */
    simulation->init(&argc, argv);

    /*
     * Parsing of the command-line arguments for this WRENCH simulation
     */
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <xml platform file> <workflow file> [--log=simple_wms.threshold=info]" << std::endl;
        exit(1);
    }

    /* The first argument is the platform description file, written in XML following the SimGrid-defined DTD */
    char *platform_file = argv[1];
    /* The second argument is the workflow description file, written in JSON using WfCommons's WfFormat format */
    char *workflow_file = argv[2];

    /* Reading and parsing the workflow description file to create a wrench::Workflow object */
    // std::cerr << "Loading workflow..." << std::endl;
    std::shared_ptr<wrench::Workflow> workflow;
    workflow = wrench::WfCommonsWorkflowParser::createWorkflowFromJSON(workflow_file, "100Gf");
    // std::cerr << "The workflow has " << workflow->getNumberOfTasks() << " tasks " << std::endl;
    std::cerr.flush();

    /* Reading and parsing the platform description file to instantiate a simulated platform */
    // std::cerr << "Instantiating SimGrid platform..." << std::endl;
    simulation->instantiatePlatform(platform_file);

    /* Get a vector of all the hosts in the simulated platform */
    std::vector<std::string> hostname_list = wrench::Simulation::getHostnameList();

    /* Create a list of storage services that will be used by the WMS */
    std::set<std::shared_ptr<wrench::StorageService>> storage_services;

    /* Instantiate a storage service, to be started on some host in the simulated platform,
     * and adding it to the simulation.  A wrench::StorageService is an abstraction of a service on
     * which files can be written and read.  This particular storage service, which is an instance
     * of wrench::SimpleStorageService, is started on WMSHost in the
     * platform (platform/batch_platform.xml), which has an attached disk called large_disk. The SimpleStorageService
     * is a bare bone storage service implementation provided by WRENCH.
     * Throughout the simulation execution, input/output files of workflow tasks will be located
     * in this storage service.
     */
    // std::cerr << "Instantiating a SimpleStorageService on WMSHost " << std::endl;
    auto storage_service = simulation->add(wrench::SimpleStorageService::createSimpleStorageService({"WMSHost"}, {"/"}));
    storage_services.insert(storage_service);

    /* Create a list of compute services that will be used by the WMS */
    std::set<std::shared_ptr<wrench::ComputeService>> compute_services;

    /* Instantiate and add to the simulation a batch_standard_and_pilot_jobs service, to be started on some host in the simulation platform.
     * A batch_standard_and_pilot_jobs service is an abstraction of a compute service that corresponds to
     * batch_standard_and_pilot_jobs-scheduled platforms in which jobs are submitted to a queue and dispatched
     * to compute nodes according to various scheduling algorithms.
     * In this example, this particular batch_standard_and_pilot_jobs service has no scratch storage space (mount point = "").
     * The next argument to the constructor
     * shows how to configure particular simulated behaviors of the compute service via a property
     * list. In this case, we use the conservative_bf_core_level scheduling algorithm which implements
     * conservative backfilling at the core level (i.e., two jobs can share a compute node by using different cores on it).
     * The last argument to the constructor makes it possible to specify various control message sizes.
     * In this example, one specifies that the message that will be sent to the service to
     * terminate it will be 2048 bytes. See the documentation to find out all available
     * configurable properties for each kind of service.
     */
    std::shared_ptr<wrench::BatchComputeService> batch_compute_service;
#ifndef ENABLE_BATSCHED
    std::string scheduling_algorithm = "conservative_bf_core_level";
#else
    std::string scheduling_algorithm = "conservative_bf";
#endif
    try
    {
        batch_compute_service = simulation->add(new wrench::BatchComputeService(
            {"BatchHeadNode"}, {{"BatchNode1"}, {"BatchNode2"}, {"BatchNode3"}}, "",
            {{wrench::BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM, scheduling_algorithm}},
            {{wrench::BatchComputeServiceMessagePayload::STOP_DAEMON_MESSAGE_PAYLOAD, 2048}}));
    }
    catch (std::invalid_argument &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        std::exit(1);
    }

    /* Instantiate and add to the simulation a cloud service, to be started on some host in the simulation platform.
     * A cloud service is an abstraction of a compute service that corresponds to a
     * Cloud platform that provides access to virtualized compute resources.
     * In this example, this particular cloud service has no scratch storage space (mount point = "").
     * The last argument to the constructor
     * shows how to configure particular simulated behaviors of the compute service via a property
     * list. In this example, one specified that the message that will be sent to the service to
     * terminate it will by 1024 bytes. See the documentation to find out all available
     * configurable properties for each kind of service.
     */
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

    /* Instantiate a WMS (which is an ExecutionController really), to be started on some host (wms_host), which is responsible
     * for executing the workflow.
     *
     * The WMS implementation is in SimpleWMS.[cpp|h].
     */
    // std::cerr << "Instantiating a WMS on WMSHost..." << std::endl;
    auto wms = simulation->add(
        new wrench::SimpleWMS(workflow, batch_compute_service,
                              cloud_compute_service, storage_service, {"WMSHost"}));

    /* Instantiate a file registry service to be started on some host. This service is
     * essentially a replica catalog that stores <file , storage service> pairs so that
     * any service, in particular a WMS, can discover where workflow files are stored.
     */
    std::string file_registry_service_host = hostname_list[(hostname_list.size() > 2) ? 1 : 0];
    // std::cerr << "Instantiating a FileRegistryService on " << file_registry_service_host << "..." << std::endl;
    auto file_registry_service =
        simulation->add(new wrench::FileRegistryService(file_registry_service_host));

    /* It is necessary to store, or "stage", input files for the first task(s) of the workflow on some storage
     * service, so that workflow execution can be initiated. The getInputFiles() method of the Workflow class
     * returns the set of all workflow files that are not generated by workflow tasks, and thus are only input files.
     * These files are then staged on the storage service.
     */
    // std::cerr << "Staging input files..." << std::endl;
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
    // std::cerr << "Launching the Simulation..." << std::endl;
    try
    {
        simulation->launch();
    }
    catch (std::runtime_error &e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 0;
    }
    // std::cerr << "Simulation done!" << std::endl;
    // std::cerr << "Workflow completed at time: " << workflow->getCompletionDate() << std::endl;

    simulation->getOutput().dumpWorkflowGraphJSON(workflow, "/tmp/workflow.json", true);

    /* Simulation results can be examined via simulation->getOutput(), which provides access to traces
     * of events. In the code below, go through some time-stamps and compute some statistics.
     */
    std::vector<wrench::SimulationTimestamp<wrench::SimulationTimestampTaskCompletion> *> trace;
    trace = simulation->getOutput().getTrace<wrench::SimulationTimestampTaskCompletion>();

    unsigned long num_failed_tasks = 0;
    double computation_communication_ratio_average = 0.0;
    double io_time_input = 0.0;
    double io_time_output = 0.0;
    double compute_time = 0.0;
    for (const auto &item : trace)
    {
        auto task = item->getContent()->getTask();
        if (task->getExecutionHistory().size() > 1)
        {
            num_failed_tasks++;
        }
        io_time_input = task->getExecutionHistory().top().read_input_end - task->getExecutionHistory().top().read_input_start;
        io_time_output = task->getExecutionHistory().top().write_output_end - task->getExecutionHistory().top().write_output_start;
        compute_time = task->getExecutionHistory().top().computation_end - task->getExecutionHistory().top().computation_start;
        computation_communication_ratio_average += compute_time / (io_time_input + io_time_output);
    }

    computation_communication_ratio_average /= (double)(trace.size());

    std::ofstream csvFile;

    csvFile.open("/home/wrench/datas/execution_output.csv", std::ios::app);

    if (!csvFile.is_open())
    {
        std::cerr << "Erro ao abrir o arquivo CSV!" << std::endl;
        std::cerr << "Erro do sistema: " << strerror(errno) << std::endl;
    }

    // Adiciona o cabeçalho apenas na primeira vez que abrir o arquivo
    if (csvFile.tellp() == 0)
    {
        csvFile << "runid,host_name,num_cores,num_tasks,trace_size,failed_tasks,compute_time,IO_time_input,IO_time_output,Comm/Comp_Ratio,power,completion_date\n";
    }

    csvFile << std::fixed << std::setprecision(2);

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
        double watts = energy_consumed / conclusion_time;

        csvFile << runId << ","
                << host_name << ","
                << num_cores << ","
                << num_tasks << ","
                << trace.size() << ","
                << num_failed_tasks << ","
                << compute_time << ","
                << io_time_input << ","
                << io_time_output << ","
                << computation_communication_ratio_average << ","
                << watts << ","
                << conclusion_time << "\n";
    }
    csvFile.close();

    return 0;
}
