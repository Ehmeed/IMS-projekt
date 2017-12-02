#include "simlib.h"
#include <iostream>
#include <sstream>
#include <string>
#include <cstring>

#include "model.h"
#include "constants.h"

using namespace  std;

// globals variables
int simulationTime = 0;
int breakdowns = 0;
bool breakdownActive = false;
int requirementsGeneratedDuringBreakdown = 0;
bool liveChatOn = true;

// global objects:
Histogram CustomerRequirementsTable("Customer Requirements",0, 1500, 20);
Histogram TicketQueueTable("Ticket Queue Table", 0, 1500, 20);
Store liveChat("LiveChat", 5);
Queue waitTickets("Waiting Tickets");
Queue waitTicketsBackend("Waiting Tickets for technician");

/*
 * Returns current time of time day in seconds
 */
int getTod(){
	return (int)(Time)%(DAY); 
}
int weekendTimeLeft(){
	if((int)(Time) % (WEEK) > 5*DAY){
		return (WEEK - ((int)(Time) %(WEEK)));
	}
	return 0;
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
	void Behavior(){
		while(true){
			if(!waitTickets.Empty()){
				Wait(Exponential(300));
				if(Random() < 0.05){
					//ticket requires backend technical worker
					waitTicketsBackend.Insert(waitTickets.GetFirst());
				}else{
					waitTickets.GetFirst()->Activate();
				}
				
			}else{
				int tod = getTod();
				if(tod < (11*HOUR)){
					Wait((11*HOUR) - tod + Uniform(0, HOUR));
				}else if(tod < (16*HOUR)){
					Wait((16*HOUR) - tod + Uniform(0,HOUR));
				}else if(tod < (21*HOUR)){
					Wait((21*HOUR) - tod + Uniform(0,HOUR));
				}else {
					Wait(DAY - tod);
					Wait((11*HOUR) + Uniform(0, HOUR));
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
			if(Random() < 0.5){
				Wait(Exponential(2*MINUTE));
			}else{
				Wait(Exponential(15*MINUTE));
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
			timeToNextRequest = 323;
		}else{
			timeToNextRequest = 232;
		}
		if(breakdownActive){
			timeToNextRequest *= 0.1;
			requirementsGeneratedDuringBreakdown++;
		}
		Activate(Time+Normal(timeToNextRequest, 0.02));
  	}
};
void parseArgs(int argc, char** argv){
	simulationTime = WEEK;

	try {
		simulationTime = std::stoi(argv[1]);
	} catch(std::exception const &e){
		std::cout << "Error parsing arguments \n";
		//exit(EXIT_FAILURE);
	}
	for(int i = 1; i < argc; i++){
		if(strcmp(argv[i], "--no-livechat") == 0){
			liveChatOn = false;
		}
	}
}
int main(int argc, char** argv) {                 
  	Print("Support model\n");
  	SetOutput("model.out");
  	parseArgs(argc, argv);
	RandomSeed(time(NULL));
	Init(0,simulationTime);

	(new Generator)->Activate();
	(new BreakdownGenerator)->Activate();
	(new SupportWorker)->Activate(); 
	(new BackendWorker)->Activate();
	Run();                  
  	liveChat.Output();
	CustomerRequirementsTable.Output();
	TicketQueueTable.Output();
	waitTickets.Output();
	waitTicketsBackend.Output();
	Print("Simulation run for %d seconds\n", simulationTime);
	Print("Total number of breakdowns: %d\n", breakdowns);
	Print("Requirements during breakdown: %d\n", requirementsGeneratedDuringBreakdown);
	return 0;
}

