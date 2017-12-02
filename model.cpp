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
// global objects:
Histogram Table("Customer Requirements",0,25,20);
Histogram TicketQueueTable("Ticket Queue Table", 0, 1500, 20);
Store liveChat("LiveChat", 5);
Queue waitTickets("Waiting Tickets");
Queue waitTicketsBackend("Waiting Tickets for technician");

/*
 * Returns current time of time day in seconds
 */
int getTod(){
	return (int)(Time)%(86400); 
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
				if(tod < (11*60*60)){
					Wait((11*60*60) - tod + Uniform(0, 60*60));
				}else if(tod < (16*60*60)){
					Wait((16*60*60) - tod + Uniform(0,60*60));
				}else if(tod < (21*60*60)){
					Wait((21*60*60) - tod + Uniform(0,30*60));
				}else {
					Wait(86400 - tod);
					Wait((11*60*60) + Uniform(0, 60*60));
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
					Wait(Exponential(2*60));
				}else{
					Wait(Exponential(30*60));
				}
				waitTicketsBackend.GetFirst()->Activate();
			}else{
				int tod = getTod();
				if(tod < (8*60*60)){
					Wait((8*60*60) - tod + Uniform(0,60*60));
				}else if(tod > (22*60*60)){
					Wait((86400-tod));
					Wait((8*60*60) + Uniform(0,60*60));
				}else {
					Wait(Uniform(0, 2*60*60));
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
		if(tod > (12*60*60) && tod < (18*60*60) && Random() < 0.85 && !liveChat.Full() && weekendTimeLeft() == 0){
			// goes to live chat
			Enter(liveChat);
			if(Random() < 0.5){
				Wait(Exponential(120));
			}else{
				Wait(Exponential(15*60));
			}
			Leave(liveChat);
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
		Wait(Exponential(30*24*60*60));
		double repairTime = Exponential(60*30);
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
		if(tod < (12*60*60)){
			timeToNextRequest = 2057;
		}else if(tod < (18*60*60)){
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
	//TODO REMOVE REMOVE REMOVE
	simulationTime = 86400;  
	simulationTime = WEEK;
	return;

	try {
		simulationTime = std::stoi(argv[1]);
	} catch(std::exception const &e){
		std::cout << "Error parsing arguments \n";
		exit(EXIT_FAILURE);
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
	//Table.Output();
	TicketQueueTable.Output();
	waitTickets.Output();
	waitTicketsBackend.Output();
	Print("Simulation run for %d seconds\n", simulationTime);
	Print("Total number of breakdowns: %d\n", breakdowns);
	Print("Requirements during breakdown: %d\n", requirementsGeneratedDuringBreakdown);
	return 0;
}

