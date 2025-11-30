Process Scheduling simulator
==================================     
## Overview
The Process Scheduling Simulator is a comprehensive visual tool for simulating and analyzing various CPU scheduling algorithms. Built with GTK3, it provides an intuitive way to explore how different scheduling policies manage processes in an operating system.
## Features
- **Graphical User Interface (GUI):** Modern GTK3-based interface with intuitive algorithm selection and real-time Gantt chart visualization.
- **Multiple Scheduling Algorithms:** Supports simulation of various CPU scheduling algorithms.
- **Process Visualization:** Displays the order of process execution and relevant metrics for analysis.
- **Dynamic Configuration:** Customizable process generation with adjustable ranges
## Supported Scheduling Algorithms
- **First-Come, First-Served (FCFS):**  Non-preemptive FIFO scheduling.
- **Shortest Job First (SJF):**  Non-preemptive burst-time based scheduling.
- **Shortest Remaining Time(SRT):** Preemptive version of SJF.
- **Priority Scheduling:** Non-preemptive, priority-based.
- **Round Robin (RR):** Preemptive time-slice/quantum based.
- **Preemptive Priority Scheduling:** Priority-based with preemption.
- **Multilevel Queue Scheduling (MLQ):** Static queue hierarchy with Round Robin inside each level and strict preemption from higher levels.
- **Multilevel Feedback Queue (MLFQ):** Dynamic scheduling with process demotion and adaptive time-quantum behavior.
## Getting Started

### System Requirements
- Ubuntu 14.04+ (or any Linux distribution with GTK3 support)
- Git
- GCC & build essentials
### Required Libraries
 **Ubuntu / Debian**
 ```bash
 sudo apt-get install build-essential
 sudo apt-get install libgtk-3-dev
 sudo apt-get install libcjson-dev
 ```
## <u>Installation</u>

Clone the project repository:
```bash
git clone https://github.com/ibtihelhamdi01/process-scheduling.git
cd process-scheduling
``` 
Run the installation script:
```bash
./install.sh
```
## <u>Configuring & Running</u>
Generate the configuration file
```bash
scheduler g
```
This will create a file named generated_config.json.

Run the simulator
```bash
scheduler generated_config.json
```
## Usage
### Main Interface
When launching the program, you will see:

- **Left Side Panel(Scheduling algorithms section)**  Algorithm selection panel with clickable buttons for all scheduling algorithms
- **Center Main Area** Gantt chart visualization showing process execution timelines
- **Left Side Panel(controls section)** Control buttons for generating configs, viewing metrics, settings, and exporting
- **Bottom Status Bar**  Status information showing current algorithm, process count, and quantum settings

![Main Interface](https://i.ibb.co/tT02ngYf/main.png)

You can:
- Select any of the 8 scheduling algorithms

![algorithm](https://i.ibb.co/XZtcZ4KJ/shedulingalgorthms.png)

- Generate random processes

![randomprocess](https://i.ibb.co/93swzFRY/generateconfig.png)

- Save Gantt charts as PNG files

![savegantt](https://i.ibb.co/v6JSxb91/exportpng.png)

### Settings Interface
Open the Settings window to modify:
- Quantum Value
- Random Process Generation Intervals

<div align="center">
  <img src="https://i.ibb.co/DfhZrhHm/Settings.png" alt="Settings Window" />
</div>

### Metrics Interface
Open the Metrics window to view:
- Waiting time for each process
- Turnaround time for each process
- Average waiting/turnaround times

<div align="center">
  <img src="https://i.ibb.co/fzmxdXsj/metrics.png" alt="metrics Window" />
</div>

## License

This project is open source and available under the MIT License.
