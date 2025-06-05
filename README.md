# Workflow Energy Consumption Simulator

![Static Badge](https://img.shields.io/badge/2.3-gray?style=for-the-badge&label=WRENCH&labelColor=orange)
![Static Badge](https://img.shields.io/badge/1.5-gray?style=for-the-badge&label=WfCommons&labelColor=green)
![Static Badge](https://img.shields.io/badge/28.2.2-gray?style=for-the-badge&label=Docker&labelColor=blue)

This repository brings together scripts and tools to simulate energy consumption in scientific workflows using **WRENCH**. The goal is to help understand how different workflow configurations affect **energy consumption** and the use of **computational resources**.

## Project Objective

Provide a basis for simulating energy consumption in workflows, enabling studies and analyses that contribute to the development of more efficient and sustainable strategies.

## Functionality

- Simulation of scientific workflows in **HPC** systems.
- Generation of synthetic workflows with tools such as **Wfcommons**.
- Collection and export of energy consumption metrics in **CSV** format files.

## Used tools

- **WRENCH**: A simulation framework for workflow management systems.
- **Wfcommons**: Tool for modeling and generating synthetic workflows.
- **Python**: For automation and analysis of results.
- **C++**: For simulation development.
- **Docker**: For configuring reproducible environments.

## Repository structure

```bash
{path}/energy-consumption-simulator/
├── datas/                 # Data obtained through simulations
├── include/               # Code headers
├── notebooks/             # Jupyter Notebooks with Machine Learning results
├── platforms/             # Files and scripts related to different simulated platforms
├── src/                   # Main source code of the project
├── workflows/             # Workflows metrics used for simulations
   ├── blast/
   ├── epigenomics/
   ├── montage/
```

## Pre requisites

Before you begin, make sure you have:
- Docker

## How to Use

### 1. Environment Setup

Clone the repository and install the dependencies:

```bash
git clone https://github.com/feliperafaelbarbosa/energy-consumption-simulator.git
cd energy-consumption-simulator
```

### 2. Workflow Generation
Inside the Docker container, install WfCommons.
Create synthetic workflows using the `wfgenrecipe.py` script located in the `src` folder. THe files will be generated in the `workflows/` directory.

### 3. Project Compilation and Build

Create a folder named `build` in the root of the project, enter the folder, run `cmake ..` and then `make`

### 4. Starting the Simulation

Navigate to the root of the project and run the `start.sh` script. The simulation results will be generated in the `/data` directory.

## Contributing

Contributions are welcome! Feel free to:

- Open **issues** to report problems or suggest improvements.

- Submit **pull requests** with new features or fixes.

## License

This project is licensed under the [Licença MIT](LICENSE).
