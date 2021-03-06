#include "simlib.h"
#include <iostream>
#include <sstream>
#include <string>
#include <cstring>

#include "model.h"

using namespace  std;

// globals variables
int simulationTime = 0;
int breakdowns = 0;
bool breakdownActive = false;
int requirementsGeneratedDuringBreakdown = 0;
bool liveChatOn = true;

bool extraWorker = false;
int extraWorkerHours[3];
string outFile = "";

// global objects:
Histogram CustomerRequirementsTable("Customer Requirements",0, 1000, 20);
Histogram TicketQueueTable("Ticket Queue Table", 0, 1000, 20);
Store liveChat("LiveChat", 5);
Queue waitTickets("Waiting Tickets");
Queue waitTicketsBackend("Waiting Tickets for technician");


int getTod(){
	return (int)(Time)%(DAY); 
}
int weekendTimeLeft(){
	if((int)(Time) % (WEEK) > 5*DAY){
		return (WEEK - ((int)(Time) %(WEEK)));
	}
	return 0;
}
void printHelp(){
	cout << "\n";
	cout << "usage: model [-t time] [--no-livechat] [--extra-worker t1 t2 t3] [--out file]\n";
	cout << " -t             - sets the time of simulation to {time} seconds (default is 604800) \n";
	cout << " --no-livechat  - removes the livechat worker from simulation\n";
	cout << " --extra-worker - adds extra support worker with working hours t1, t2, t3\n";
	cout << " --out          - sets output file to {file} (default is stdout)\n";
	cout << "\n";
}
class Ticket : public Process {
	double Prichod;
	void Behavior(){
		Prichod = Time;
		Into(waitTickets);
		Passivate();
		TicketQueueTable(Time-Prichod);
		CustomerRequirementsTable(Time-Prichod);
	}
};
class SupportWorker : public Process {
	int operatingHours[3];
	public:
		SupportWorker(int oh0, int oh1, int oh2) : Process(){
			operatingHours[0] = oh0;
			operatingHours[1] = oh1;
			operatingHours[2] = oh2;
		}
		int timeSpentWorking = 0;

	void Behavior(){
		while(true){
			if(!waitTickets.Empty()){
				int timeToSolveTicket = Exponential(2*MINUTE);
				timeSpentWorking += timeToSolveTicket;
				Wait(timeToSolveTicket);
				if(!waitTickets.Empty()){
					// we have to check for empty queue here since there is a chance that
					// both support workers (if two present) will be working on the same ticket
					// and by the time the slower gets here, the other solves it already
					if(Random() < 0.05){
						//ticket requires backend technical worker
						waitTicketsBackend.Insert(waitTickets.GetFirst());
					}else{
						//ticket solved
						waitTickets.GetFirst()->Activate();
					}
				}			
			}else{
				int tod = getTod();
				if(tod < (operatingHours[0]*HOUR)){
					Wait((operatingHours[0]*HOUR) - tod + Uniform(0, HOUR));
				}else if(tod < (operatingHours[1]*HOUR)){
					Wait((operatingHours[1]*HOUR) - tod + Uniform(0,HOUR));
				}else if(tod < (operatingHours[2]*HOUR)){
					Wait((operatingHours[2]*HOUR) - tod + Uniform(0,HOUR));
				}else {
					Wait(DAY - tod);
					Wait((operatingHours[0]*HOUR) + Uniform(0, HOUR));
				}
			}			
		}
	}
};
class BackendWorker : public Process {
	void Behavior(){
		while(true){
			if(!waitTicketsBackend.Empty()){
				if(Random()<0.3){
					Wait(Exponential(2*MINUTE));
				}else{
					Wait(Exponential(30*MINUTE));
				}
				waitTicketsBackend.GetFirst()->Activate();
			}else{
				int tod = getTod();
				if(tod < (8*HOUR)){
					Wait((8*HOUR) - tod + Uniform(0,HOUR));
				}else if(tod > (22*HOUR)){
					Wait((DAY-tod));
					Wait((8*HOUR) + Uniform(0,HOUR));
				}else {
					Wait(Uniform(0, 2*HOUR));
				}
			}
		}
	}
};
class CustomerRequirement : public Process { 
	double Prichod;                 
	void  Behavior() {              
		Prichod = Time;   
		int tod = getTod();
		// IF live chat is on and not full 
		if(liveChatOn && tod > (12*HOUR) && tod < (18*HOUR) && Random() < 0.85 && !liveChat.Full() && weekendTimeLeft() == 0){
			// goes to live chat
			Enter(liveChat);
			if(Random() < 0.6){
				int shortConversation = Exponential(2*MINUTE);
				Wait(shortConversation);
			}else{
				int longConversation = Exponential(15*MINUTE);
				Wait(longConversation);
			}
			Leave(liveChat);
			CustomerRequirementsTable(Time - Prichod);
			// some problems can't be solved via livechat and need to write a ticket
			if(Random() < 0.1){
				(new Ticket)->Activate();				
			}
		}else {
			(new Ticket)->Activate();
		}            
	}
};
class BreakdownGenerator : public Process {
	void Behavior() {		
		Wait(Exponential(4*WEEK));
		double repairTime = Exponential(30*MINUTE);
		breakdownActive = true;
		breakdowns++;
		Wait(repairTime);
		breakdownActive = false;	
		Activate(Time);
	}
};
class Generator : public Event {  
	void Behavior() {              
		(new CustomerRequirement)->Activate();
		//calculate the time of the day in order to adjust activation time
		int tod = getTod();
		double timeToNextRequest = 0;
		if(tod < (12*HOUR)){
			timeToNextRequest = 2057;
		}else if(tod < (18*HOUR)){
			timeToNextRequest = 280;
		}else if(tod < (22*HOUR)){
			timeToNextRequest = 305;
		}else{
			timeToNextRequest = 500;
		}
		if(breakdownActive){
			timeToNextRequest *= 0.1;
			requirementsGeneratedDuringBreakdown++;
		}
		Activate(Time+Normal(timeToNextRequest, 0.1));
  	}
};
void parseArgs(int argc, char** argv){
	simulationTime = WEEK; //default simulation time
	//TODO error on wrong args
	for(int i = 1; i < argc; i++){
		if(strcmp(argv[i], "--help") == 0 || strcmp(argv[i],"-h") == 0){
			printHelp();
			exit(EXIT_SUCCESS);
		}
		if(strcmp(argv[i], "--no-livechat") == 0){
			liveChatOn = false;
		}
		if(strcmp(argv[i], "--extra-worker") == 0){
			try{
				extraWorkerHours[0] = std::stoi(argv[i+1]);
				extraWorkerHours[1] = std::stoi(argv[i+2]);
				extraWorkerHours[2] = std::stoi(argv[i+3]);
				extraWorker = true;
			}catch(std::exception const &e){
				std::cout << "Error parsing arguments \n";	
				exit(EXIT_FAILURE);	
			}
		}
		if(strcmp(argv[i], "-t") == 0){
			try {
				simulationTime = std::stoi(argv[i+1]);
			} catch(std::exception const &e){
				std::cout << "Error parsing arguments \n";
				exit(EXIT_FAILURE);
			}

		}
		if(strcmp(argv[i], "--out") == 0){
			try{
				outFile = string(argv[i+1]);
			}catch(std::exception const &e){
				std::cout << "Error parsing arguments \n";
				exit(EXIT_FAILURE);
			}
		}
	}
}
int main(int argc, char** argv) {                 
  	Print("Running simulation of technical support model\n");
  	parseArgs(argc, argv);
  	if(strcmp(outFile.c_str(), "") != 0){
		SetOutput(outFile.c_str());
	}
	// INITIALIZE SIMULATION
	RandomSeed(time(NULL));
	Init(0,simulationTime);
	(new Generator)->Activate();
	(new BreakdownGenerator)->Activate();
	(new BackendWorker)->Activate();
	SupportWorker* supportWorker1 = (new SupportWorker(11, 16, 20));
	supportWorker1->Activate(); 
	SupportWorker* supportWorker2;
	if(extraWorker){
		supportWorker2 = (new SupportWorker(extraWorkerHours[0], extraWorkerHours[1], extraWorkerHours[2]));
		supportWorker2->Activate(); 
	}

	Run(); 
	// LOG TO OUTPUT FILE                 
	TicketQueueTable.Output();
	waitTickets.Output();
	waitTicketsBackend.Output();
  	liveChat.Output();
	CustomerRequirementsTable.Output();
	Print("Simulation run for %d seconds\n", simulationTime);
	Print("Total number of breakdowns: %d\n", breakdowns);
	Print("Requirements during breakdown: %d\n", requirementsGeneratedDuringBreakdown);
	Print("Support worker 1 spent %d seconds working\n", supportWorker1->timeSpentWorking);
	if(extraWorker){
		Print("Support worker 2 spent %d seconds working\n", supportWorker2->timeSpentWorking);
	}
	return 0;
}

