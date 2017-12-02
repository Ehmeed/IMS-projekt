/*
 * File model.h
*/


/*
 * Returns current time of the day in seconds
 */
int getTod();
/*
 * Returns time left of weekned (to Monday 0:00) in seconds. Returns zero on workdays
 */
int weekendTimeLeft();
/*
 * Parses and stores program arguments
 */
void parseArgs();

/*
 * Ticket class represents a support ticket written by customer
 */
class Ticket;
/*
 * Represents support worker. His job is to solve tickets send by customers.
 */
class SupportWorker;
/*
 * Represents backend developer. His job is to solve tickets that require some server maintenance.
 */
class BackendWorker;
/*
 * Represents an issue that customer needs help with. Issus is either solved in this class by livechat or delegated.
 */
class CustomerRequirement;
/*
 * Generates major breakdowns that influence multiple servers.
 */
class BreakdownGenerator;
/*
 * Generates common customer requirements.
 */
class Generator;

