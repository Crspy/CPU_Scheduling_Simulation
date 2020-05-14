#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <assert.h>

enum ExecutionPhase
{
    CPU1,
    IO,
    CPU2
};

enum ExecutionState
{
    NotArrived,
    READY,
    BLOCKED,
    DONE
};

struct Process
{
    int id;
    int timeNeeded;
    int IO_Time;
    int arrivalTime;
    int runTime = 0;
    int turnAroundTime = 0;
    int blockedTime = 0;
    ExecutionPhase phase = ExecutionPhase::CPU1;
    ExecutionState state = ExecutionState::NotArrived;
};

void PrintCycleInfo(std::ofstream &outFile, int SimTimer, const std::vector<Process> &processes,
                    int currentProcessIdx, std::vector<int> &readyVec);
std::vector<Process> LoadProcessesInfo(std::ifstream &file);
void RoundRobinPolicy(std::ifstream &file, int quantumTime);
void FCFSPolicy(std::ifstream &file);


enum SchedulingPolicy
{
    FCFS,
    RR
};

int main()
{
    while (true)
    {
        std::ifstream inFile;
        while(true)
        {

            std::cout << "Enter input file name: ";

            std::string inFileName;
            std::cin >> inFileName;

            inFile = std::ifstream(inFileName);

            if(inFile)
            {
                // file is accessible
                break;
            }

            // we keep looping until the user enters an accessible input file name
            std::cout << "File is not accessible or doesn't exist.\n";

        }


        bool IsError = false;
        while (true)
        {
            std::cout << "0.First Come First Served(FCFS).\n1.Round Robin(RR).\nChoose the scheduling policy: ";
            int chosenMehod = -1;
            std::cin >> chosenMehod;
            switch (chosenMehod)
            {
            case SchedulingPolicy::RR:
            {
                int quantumTime = -1;
                // we keep looping until the user enters a valid quantum time
                while (quantumTime <= 0)
                {
                    std::cout << "Enter quantum time: ";
                    std::cin >> quantumTime;
                    if (quantumTime <= 0)
                    {
                        std::cout << "Invalid quantum time.\n";
                    }
                }

                // execute our RR scheduling policy
                RoundRobinPolicy(inFile, quantumTime);
                std::cout << "outputRR.txt generated.\n\n";
                break;
            }
            case SchedulingPolicy::FCFS:
            {
                // execute our FCFS scheduling policy
                FCFSPolicy(inFile);
                std::cout << "outputFCFS.txt generated.\n\n";
                break;
            }
            default:
                std::cout << "Invalid choice.\n";
                IsError = true;
                break;
            }
            if (!IsError)
                break;
        }
    }
}




std::vector<Process> LoadProcessesInfo(std::ifstream &file)
{
    std::vector<Process> processes;

    Process proc;
    while (file >> proc.id >> proc.timeNeeded >> proc.IO_Time >> proc.arrivalTime)
    {
        processes.push_back(proc);
    }
    return processes;
}

void RoundRobinPolicy(std::ifstream &file, int quantumTime)
{
    std::vector<Process> processes = LoadProcessesInfo(file);
    if (processes.empty())
        return;

    std::ofstream outFile = std::ofstream("outputRR.txt");
    std::vector<int> readyVec; // ready queue
    std::vector<int> blockedVec; // blocked queue

    // Simulation cycles counter
    int SimTimer = 0;

    // a counter for Busy cycles
    int BusyTimer = 0;

    // a counter that signals the end of the simulation
    //when it reaches the same number of cycles read from the input file
    int processesCompleted = 0;

    while (true)
    {
        // used to check if the cpu was bysy during the current cycle
        // so that we can increment BusyTimer at the end of the cycle
        // based on its value
        bool IsBusy = false;

        // in every cycle we loop through all processes read from the input file
        // and check if any of them has reached its arrival time so that we can move it
        // into the readyVec
        for (int idx = 0; idx < processes.size(); ++idx)
        {
            Process &proc = processes[idx];

            if (SimTimer == proc.arrivalTime)
            {
                proc.state = ExecutionState::READY;
                readyVec.push_back(idx);
            }
        }


        // only execute if the readyVec isn't empty
        if (!readyVec.empty())
        {
            // the running process is always the front of the ready queue
            // which is being rotated after every quantum time or if the process finished its cpu time
            const int procIdx = *readyVec.begin();
            Process &proc = processes[procIdx];
            ++proc.runTime;
            IsBusy = true;


            PrintCycleInfo(outFile, SimTimer, processes, 0, readyVec);
            if (proc.runTime >= proc.timeNeeded)
            {
                assert(proc.runTime == proc.timeNeeded);
                assert(proc.phase != ExecutionPhase::IO);

                if (proc.phase == ExecutionPhase::CPU1)
                {
                    proc.phase = ExecutionPhase::IO;
                    proc.state = ExecutionState::BLOCKED;
                    readyVec.erase(readyVec.begin());
                    // move the process to blocked queue
                    blockedVec.emplace_back(procIdx);
                }
                else if (proc.phase == ExecutionPhase::CPU2)
                {
                    ++processesCompleted;
                    proc.turnAroundTime = SimTimer - proc.arrivalTime + 1;
                    proc.state = ExecutionState::DONE;
                    // print
                    readyVec.erase(readyVec.begin());
                }
            }

            // if current process finished its quantum time
            // or the process finished its CPU Time before quantum time is over
            if (!readyVec.empty() &&
                    ((proc.runTime % quantumTime) == 0 || proc.state != ExecutionState::READY))
            {

                // if current process was not removed from Ready Queue
                // then we rotate the readyVec left by 1 and use the first element as the new idx
                if (proc.state == ExecutionState::READY)
                    std::rotate(readyVec.begin(), readyVec.begin() + 1, readyVec.end());
            }

        }
        else
        {
            PrintCycleInfo(outFile, SimTimer, processes, 0, readyVec);
        }


        // in every cycle we loop through all blocked processes to check
        // if one of them has successfully finished their IO Time and
        // is ready for execution so that we can move it back to the ready queue
        for (auto it = blockedVec.begin(); it != blockedVec.end(); ++it)
        {
            const auto idx = *it;
            Process &blockedProc = processes[idx];
            if (blockedProc.blockedTime >= blockedProc.IO_Time)
            {
                assert(blockedProc.phase == ExecutionPhase::IO);
                blockedProc.phase = ExecutionPhase::CPU2;
                blockedProc.runTime = 0;
                blockedProc.state = ExecutionState::READY;
                blockedVec.erase(it);
                --it;
                // move the process to ready queue
                readyVec.emplace_back(idx);
            }
            ++blockedProc.blockedTime;
        }

        if (processesCompleted == processes.size())
        {
            break;
        }

        ++SimTimer;
        if (IsBusy)
            ++BusyTimer;
    }

    outFile << "Finishing time: " << SimTimer << '\n';
    outFile << "CPU Utilization: " << float(BusyTimer + 1) / (SimTimer + 1) << '\n';

    for (auto &proc : processes)
    {
        outFile << "Turnaround time of Process " << proc.id << ": "
                << proc.turnAroundTime << '\n';
    }
}

void FCFSPolicy(std::ifstream &file)
{
    std::vector<Process> processes = LoadProcessesInfo(file);
    if (processes.empty())
        return;

    std::ofstream outFile = std::ofstream("outputFCFS.txt");
    std::vector<int> readyVec; // ready queue
    std::vector<int> blockedVec; // blocked queue

    // Simulation cycles counter
    int SimTimer = 0;

    // a counter for Busy cycles
    int BusyTimer = 0;

    // a counter that signals the end of the simulation
    //when it reaches the same number of cycles read from the input file
    int processesCompleted = 0;

    // used to hold the ID of the Process that's about to be executed
    int currentProcessIdx = 0;



    while (true)
    {
        // used to check if the cpu was bysy during the current cycle
        // so that we can increment BusyTimer at the end of the cycle
        // based on its value
        bool IsBusy = false;

        // in every cycle we loop through all processes read from the input file
        // and check if any of them has reached its arrival time so that we can move it
        // into the readyVec
        for (int idx = 0; idx < processes.size(); ++idx)
        {
            Process &proc = processes[idx];

            if (SimTimer == proc.arrivalTime)
            {
                proc.state = ExecutionState::READY;
                readyVec.push_back(idx);
            }
        }

        // only execute if the readyVec isn't empty
        if (!readyVec.empty())
        {
            const int procIdx = readyVec[currentProcessIdx];
            Process &proc = processes[procIdx];
            ++proc.runTime;
            IsBusy = true;

            auto readyVec_it = readyVec.begin() + currentProcessIdx;
            PrintCycleInfo(outFile, SimTimer, processes, currentProcessIdx, readyVec);
            if (proc.runTime >= proc.timeNeeded)
            {
                assert(proc.runTime == proc.timeNeeded);
                assert(proc.phase != ExecutionPhase::IO);

                if (proc.phase == ExecutionPhase::CPU1)
                {
                    proc.phase = ExecutionPhase::IO;
                    proc.state = ExecutionState::BLOCKED;
                    readyVec.erase(readyVec_it);
                    // move the process to blocked queue
                    blockedVec.emplace_back(procIdx);
                }
                else if (proc.phase == ExecutionPhase::CPU2)
                {
                    ++processesCompleted;
                    proc.turnAroundTime = SimTimer - proc.arrivalTime + 1;
                    proc.state = ExecutionState::DONE;
                    // print
                    readyVec.erase(readyVec_it);
                }

                // here we switch to the next process to be executed from the ready queue
                // and we prioritize the one with the lowest ID
                currentProcessIdx = std::min_element(readyVec.begin(), readyVec.end()) - readyVec.begin();
            }
        }
        else
        {
            PrintCycleInfo(outFile, SimTimer, processes, currentProcessIdx, readyVec);
        }



        // in every cycle we loop through all blocked processes to check
        // if one of them has successfully finished their IO Time and
        // is ready for execution so that we can move it back to the ready queue
        for (auto it = blockedVec.begin(); it != blockedVec.end(); ++it)
        {
            const auto idx = *it;
            Process &blockedProc = processes[idx];
            if (blockedProc.blockedTime >= blockedProc.IO_Time)
            {
                assert(blockedProc.phase == ExecutionPhase::IO);
                blockedProc.phase = ExecutionPhase::CPU2;
                blockedProc.runTime = 0;
                blockedProc.state = ExecutionState::READY;
                blockedVec.erase(it);
                --it;
                // move the process to ready queue
                readyVec.emplace_back(idx);
            }
            ++blockedProc.blockedTime;
        }

        if (processesCompleted == processes.size())
        {
            break;
        }

        ++SimTimer;
        if (IsBusy)
            ++BusyTimer;
    }

    outFile << "Finishing time: " << SimTimer << '\n';
    outFile << "CPU Utilization: " << float(BusyTimer + 1) / (SimTimer + 1) << '\n';

    for (auto &proc : processes)
    {
        outFile << "Turnaround time of Process " << proc.id << ": "
                << proc.turnAroundTime << '\n';
    }
}

void PrintCycleInfo(std::ofstream &outFile, int SimTimer, const std::vector<Process> &processes,
                    int currentProcessIdx, std::vector<int> &readyVec)
{
    outFile << SimTimer;
    for (int idx = 0; idx < processes.size(); ++idx)
    {
        const auto &proc = processes[idx];
        if (proc.state == ExecutionState::DONE || proc.state == ExecutionState::NotArrived)
            continue;

        outFile << ' ' << proc.id << ':';
        if (!readyVec.empty() && idx == readyVec[currentProcessIdx])
        {
            outFile << "running";
        }
        else
        {
            switch (proc.state)
            {
            case ExecutionState::READY:
                outFile << "ready";
                break;
            case ExecutionState::BLOCKED:
                outFile << "blocked";
                break;
            }
        }
    }
    outFile << std::endl;
}
