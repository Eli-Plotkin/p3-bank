#include <iostream>
#include <algorithm>
#include <fstream> 
#include <getopt.h>
#include <unordered_set>
#include <queue>

using namespace std;

// Project Identifier: 292F24D17A4455C1B5133EDD8C7CEAA0C9570A98


class Bank {
    public:
    Bank(): fileName(""), isV(false), cID(0), currTS(0), prevTS(0){}
    void getMode(int argc, char * argv[])   {
            // These are used with getopt_long()
            opterr = false; // Let us handle all error output for command line options
            int choice;
            int index = 0;
            option long_options[] = {
                {"help", no_argument, nullptr, 'h'},
                {"file", required_argument, nullptr, 'f'},
                {"verbose", no_argument, nullptr, 'v'},
                { nullptr, 0, nullptr, '\0' },
            };  // long_options[]

            while ((choice = getopt_long(argc, argv, "hvf:", long_options, &index)) != -1) {
                switch(choice)  {
                    case 'h':
                        printHelp();
                        break;
                    case 'v':
                        isV = true;
                        break;
                    case 'f':
                        fileName = optarg;
                        break;
                        //TODO Error handle if file not provided or won't open
                    default:
                        cerr << "Error: invalid option\n";
                        exit(1);
                }
            }
            handleInput();
        }

        void talkToBank()   {
            string type;
            bool q = false;
            while (!q && cin >> type) {
                if (type[0] == '#') {
                    string trash;
                    getline(cin, trash);
                    continue;
                }
                else if (type[0] == 'l') {
                    string uID;
                    uint32_t pin;
                    string IP;
                    cin >> uID >> pin >> IP;
                    login(uID, pin, IP);
                }
                else if (type[0] == 'o') {
                    string uID; 
                    string IP;
                    cin >> uID >> IP;
                    logout(uID, IP);
                }
                else if (type[0] == 'b')    {
                    string uID;
                    string IP;
                    cin >> uID >> IP;
                    balance(uID, IP);
                }
                else if (type[0] == 'p')    {
                    string ts;
                    string IP;
                    string sr;
                    string rr;
                    string amt;
                    string ed;
                    string os;
                    cin >> ts >> IP >> sr >> rr >> amt >> ed >> os;
                    place(ts, IP, sr, rr, amt, ed, os[0]);
                }
                else if (type[0] == '$')    {
                    q = true;
                    while(!todoTrans.empty())   {
                        transaction temp = todoTrans.top();
                        todoTrans.pop();
                        executeTrans(temp);
                    }
                } 
            }
            while (cin >> type) {
                if (type[0] == 'l')    {
                    string start;
                    string end;
                    cin >> start >> end;
                    listTransactions(convertTS(start), convertTS(end));
                }
                else if (type[0] == 'r')    {
                    string start;
                    string end;
                    cin >> start >> end;
                    bankRev(convertTS(start), convertTS(end));
                }
                else if (type[0] == 'h')    {
                    string uID;
                    cin >> uID;
                    customerHistory(uID);
                }
                else if (type[0] == 's')    {
                    string ts;
                    cin >> ts;
                    summarizeDay(convertTS(ts));
                }
            }
        }

    private:
        string fileName;
        bool isV;
        uint32_t cID;
        uint64_t currTS;
        uint64_t prevTS;
        

        struct transaction  {
            transaction(): id(0), timestamp(0), IP(""), sender(""), receiver(""), amt(0), execDate(0), os('\0'), fee(0){}
            transaction(uint32_t id, uint64_t t, string i, string s, string r, uint32_t a, uint64_t e, char os,
            uint32_t f): id(id), timestamp(t), IP(i), sender(s), receiver(r), amt(a), execDate(e), os(os), fee(f){}
            uint32_t id;
            uint64_t timestamp;
            string IP;
            string sender;
            string receiver;
            uint32_t amt;
            uint64_t execDate;
            char os;
            uint32_t fee;
        };

        struct transComp    {
            bool operator()(transaction& a, transaction& b) {
                if (a.execDate == b.execDate){
                    return a.id > b.id;
                }
                return a.execDate > b.execDate;
            }
        };

        struct tsComp    {
            bool operator()(const transaction& t, const uint64_t& ed) {
                return t.execDate < ed;
            }
        };

        std::priority_queue<transaction, vector<transaction>, transComp> todoTrans;
        vector<transaction> doneTrans;

        struct userReg  {
            userReg(): REG_TIMESTAMP(0), USER_ID(""), PIN(0), STARTING_BALANCE(0){}
            userReg(uint64_t r, string u, uint32_t p, uint32_t sb): REG_TIMESTAMP(r), 
            USER_ID(u), PIN(p), STARTING_BALANCE(sb){}

            uint64_t REG_TIMESTAMP;
            string USER_ID;
            uint32_t PIN;
            uint32_t STARTING_BALANCE;

            unordered_set<string> IPList;
            vector<transaction> inTrans;
            vector<transaction> outTrans;
        };


        uint64_t convertTS(string ts) {
            ts.erase(std::remove(ts.begin(), ts.end(), ':'), ts.end());
            return stoul(ts);

        }


        const string pluralize(const uint32_t& num, const string& single, const string& plural)  {
            return (num == 1) ? single : plural;
        }
        const string pluralize(const uint64_t& num, const string& single, const string& plural)  {
            return (num == 1) ? single : plural;
        }

        void handleInput()  {
            ifstream i;
            i.open(fileName);

            if (!i.is_open()) {
                std::cerr << "Registration file failed to open.\n";
                exit(1); 
            }

            string ts, uID, p, sb;
            while (getline(i, ts, '|') 
                && getline(i, uID, '|')
                && getline(i, p, '|')
                && getline(i, sb)) {

                if (uID.find(' ') != string::npos)  continue;

                uint32_t pInt = static_cast<uint32_t>(stoi(p));
                uint32_t sbInt = static_cast<uint32_t>(stoi(sb));
                accounts.emplace(uID, userReg(convertTS(ts), uID, pInt, sbInt));
            }
        }

        unordered_map<string, userReg> accounts;

        void printHelp()    {
            cout << "How can I help?\n";
            exit(0);
        }

        void login(const string& uID, const uint32_t& PN, const string& IP)    {
            if (accounts.find(uID) == accounts.end())   {
                if (isV)    {
                    cout << "Login failed for " << uID << ".\n";
                }
                return;
            }
            userReg& acc = accounts[uID];
            if (acc.PIN != PN)   {
                if (isV)    {
                    cout << "Login failed for " << uID << ".\n";
                }
                return;
            }
            if (isV)    {
                cout << "User "<< uID << " logged in.\n";
            }
            acc.IPList.insert(IP);
        }
        
        void balance(const string& uID, const string& IP)  {
            if (accounts.find(uID) == accounts.end())   {
                if (isV)    {
                    cout << "Account "<< uID << " does not exist.\n";
                }
                return;
            }
            userReg& acc = accounts[uID];

            if (acc.IPList.empty())  {
                if (isV)    {
                    cout << "Account " << uID << " is not logged in.\n";
                }
                return;
            }
            if (acc.IPList.find(IP) == acc.IPList.end())    {
                if (isV)    {
                    cout << "Fraudulent transaction detected, aborting request.\n";
                }
                return;
            }

            uint64_t timeToUse = (currTS == 0) ? acc.REG_TIMESTAMP : currTS;

            cout << "As of " << timeToUse 
            << ", " << uID << " has a balance of $" << acc.STARTING_BALANCE << ".\n";
        }

        void logout(const string& uID, const string& IP)   {
            if (accounts.find(uID) == accounts.end()
            || accounts[uID].IPList.find(IP) == accounts[uID].IPList.end())   {
                if (isV)    {
                    cout << "Logout failed for " << uID <<  ".\n";
                }
                return;
            }

            accounts[uID].IPList.erase(IP);
            if (isV)    {
                cout << "User " << uID << " logged out.\n";
            }
        }

        void place(const string& TIMESTAMP, const string& IP, const string& SENDER, 
            const string& RECIPIENT, const string& AMOUNT,
            const string& EXEC_DATE, const char& os)    {

            uint64_t ts = convertTS(TIMESTAMP);
            uint64_t ed = convertTS(EXEC_DATE);

            currTS = ts;


            if (ts < prevTS)    {
                cerr << "Invalid decreasing timestamp in 'place' command.\n";
                exit(1);
            }

            if (ed < ts)    {
                cerr << "You cannot have an execution date before the current timestamp.\n";
                exit(1);
            }

            uint32_t amt = static_cast<uint32_t>(stoul(AMOUNT));
            uint32_t fee = (amt / 100 < 10)  ? 10 : (amt / 100 > 450) ? 450 : amt / 100;
            if (ed - accounts[SENDER].REG_TIMESTAMP > 50000000000) fee = (fee * 3) / 4;

            transaction t(cID, ts, IP, SENDER, RECIPIENT, amt, ed, os, fee);

            string check = "";
            checkTransaction(transaction(cID, ts, IP, SENDER, RECIPIENT, amt, ed, os, fee), check);
            if (check != "")    {
                if (isV)    {
                    cout << check << "\n";
                }
                return;
            }
            
            else {
                while (!todoTrans.empty() && ts > todoTrans.top().execDate)   {
                    transaction temp = todoTrans.top();
                    todoTrans.pop();
                    executeTrans(temp);
                }
                if (isV)    {
                    cout << "Transaction " << t.id << " placed at " 
                    << t.timestamp << ": $" << t.amt << " from " << t.sender << " to " 
                    << t.receiver << " at " << t.execDate << ".\n";
                }
            }
            ++cID;
            todoTrans.push(t);
            prevTS = ts;
        }
        
        void checkTransaction(const transaction& t, string& check)    {
            if (t.sender == t.receiver) {
                check = "Self transactions are not allowed.";
                return;
            }
            if (t.execDate - t.timestamp > 3000000)    {
                check = "Select a time up to three days in the future.";
                return;
            }
            if (accounts.find(t.sender) == accounts.end())  {
                check = "Sender " + t.sender + " does not exist.";
                return;
            }
            userReg& sender = accounts[t.sender];

            if (accounts.find(t.receiver) == accounts.end())  {
                check = "Recipient " + t.receiver + " does not exist.";
                return;
            }

            if (sender.REG_TIMESTAMP > t.execDate ||
            accounts[t.receiver].REG_TIMESTAMP > t.execDate)    {
                check = "At the time of execution, sender and/or recipient have not registered.";
                return;
            }
            if (sender.IPList.empty()) {
                check = "Sender " + t.sender + " is not logged in.";
                return;
            }
            if (sender.IPList.find(t.IP) == sender.IPList.end()){
                check = "Fraudulent transaction detected, aborting request.";
                return;
            }
        }
        
        void executeTrans(const transaction& t) {
            userReg& sender = accounts[t.sender];
            userReg& receiver = accounts[t.receiver];

            if (t.os == 'o')    {
                if (sender.STARTING_BALANCE < t.amt + t.fee)  {
                    if (isV)    {
                        cout << "Insufficient funds to process transaction " << t.id << ".\n";
                    }
                    return;
                }
                else    {
                    sender.STARTING_BALANCE -= (t.amt + t.fee);
                    receiver.STARTING_BALANCE += t.amt;
                }
            }
            else {
                uint32_t rFee = t.fee / 2;
                if (sender.STARTING_BALANCE < t.amt + (t.fee - rFee) || 
                receiver.STARTING_BALANCE < rFee)  {
                    if (isV)    {
                        cout << "Insufficient funds to process transaction " << t.id << ".\n";
                    }
                    return;
                }
                else    {
                    sender.STARTING_BALANCE -= (t.amt + (t.fee - rFee));
                    receiver.STARTING_BALANCE += t.amt;
                    receiver.STARTING_BALANCE -= rFee;
                }
            }

            if (isV)    {
                cout << "Transaction " << t.id << " executed at " 
                << t.execDate << ": $" << t.amt << " from " << t.sender << " to "
                << t.receiver << ".\n";
            }

            sender.outTrans.push_back(t);
            receiver.inTrans.push_back(t);
            doneTrans.push_back(t);
        }


        void listTransactions(const uint64_t& x, const uint64_t& y)   {
            if (y <= x) {
                cout << "List Transactions requires a non-empty time interval.\n";
                return;
            }
            uint32_t counter = 0;
            auto it = lower_bound(doneTrans.begin(), doneTrans.end(), x, tsComp());
            
            while (it != doneTrans.end() && it->execDate < y)  {
                cout << it->id << ": " << it->sender << " sent " << it->amt <<
                " " << pluralize(it->amt, "dollar", "dollars") << " to " << it->receiver << " at " 
                << it->execDate << ".\n";
                ++it;
                ++counter;
            }
            cout << "There " << pluralize(counter, "was", "were") << " " << counter 
            << " " << pluralize(counter, "transaction", "transactions") << " that " 
            << pluralize(counter, "was", "were") << " placed between time " << x << " to " << y 
            << ".\n";
        }


        void bankRev(const uint64_t& x, const uint64_t& y)  {
            if (y <= x) {
                cout << "Bank Revenue requires a non-empty time interval.\n";
                return;
            }

            uint32_t total = 0;
            uint64_t interval = y - x;
            auto it = lower_bound(doneTrans.begin(), doneTrans.end(), x, tsComp());

            while (it != doneTrans.end() && it->execDate < y)  {
                total += it->fee;
                ++it;
            }

            uint64_t timeFormat[6]  {
                (interval / 10000000000), // years
                (interval / 100000000) % 100, //Months
                (interval / 1000000) % 100, //days
                (interval / 10000) % 100, //hours
                (interval / 100) % 100, //minutes
                (interval % 100) //seconds
            };

            cout << "281Bank has collected " << total << " dollars in fees over ";
            
            for (size_t i = 0; i < 6; ++i)  {
                if (timeFormat[i] == 0) {
                    continue;
                }
                if (i == 0)    {
                    cout << timeFormat[i] << pluralize(timeFormat[i], " year", " years");
                }
                else if (i == 1) {
                    if (timeFormat[0] > 0)  {
                        cout << " ";
                    }
                    cout << timeFormat[i] << pluralize(timeFormat[i], " month", " months");
                }
                else if (i == 2) {
                    if (timeFormat[0] > 0 || timeFormat[1] > 0) {
                        cout << " ";
                    }
                    cout << timeFormat[i] << pluralize(timeFormat[i], " day", " days");
                }
                else if (i == 3) {
                    if (timeFormat[0] > 0 || timeFormat[1] > 0 || timeFormat[2] > 0) {
                        cout << " ";
                    }
                    cout << timeFormat[i] << pluralize(timeFormat[i], " hour", " hours");
                }
                else if (i == 4) {
                    if (timeFormat[0] > 0 || timeFormat[1] > 0 || timeFormat[2] > 0
                    || timeFormat[3] > 0) {
                        cout << " ";
                    }
                    cout << timeFormat[i] << pluralize(timeFormat[i], " minute", " minutes");
                }
                else if (i == 5) {
                    if (timeFormat[0] > 0 || timeFormat[1] > 0 || timeFormat[2] > 0
                    || timeFormat[3] > 0 || timeFormat[4] > 0) {
                        cout << " ";
                    }
                    cout << timeFormat[i] << pluralize(timeFormat[i], " second", " seconds");
                }
            }
            cout << ".\n";
        }


        void customerHistory(const string& uID)    {
            if (accounts.find(uID) == accounts.end())   {
                cout << "User " << uID << " does not exist.\n";
                return;
            }
            userReg& acc = accounts[uID];

            cout << "Customer " << uID << " account summary:\n" 
            << "Balance: $" << acc.STARTING_BALANCE << "\n"
            << "Total # of transactions: " << acc.inTrans.size() + 
            acc.outTrans.size() << "\n"
            << "Incoming " << acc.inTrans.size() << ":\n";
            
            size_t i = acc.inTrans.size() > 10 ? acc.inTrans.size() - 10 : 0;

            for (; i < acc.inTrans.size(); ++i)   {
                transaction& in = acc.inTrans[i];

                cout << in.id << ": " << in.sender
                << " sent " << in.amt << " " 
                << pluralize(in.amt, "dollar", "dollars") 
                << " to " << in.receiver
                << " at " << in.execDate << ".\n";
            }

            cout << "Outgoing " << acc.outTrans.size() << ":\n";

            size_t j = acc.outTrans.size() > 10 ? acc.outTrans.size() - 10 : 0;

            for (; j < acc.outTrans.size(); ++j)   {
                transaction& out = acc.outTrans[j];
                
                cout << out.id << ": " << out.sender
                << " sent " << out.amt << " " 
                << pluralize(out.amt, "dollar", "dollars") 
                << " to " << out.receiver
                << " at " << out.execDate << ".\n";
            }
        }

        void summarizeDay(uint64_t ts)  {
            uint32_t counter = 0;
            uint32_t tFees = 0;
            ts -= (ts % 1000000);

            auto it = lower_bound(doneTrans.begin(), doneTrans.end(), ts, tsComp());
            
            cout  << "Summary of [" << ts << ", " << ts + 1000000 << "):\n";


            while (it != doneTrans.end() && it->execDate < (ts + 1000000))  {
                cout << it->id << ": " << it->sender << " sent " << it->amt << " dollars to "
                << it->receiver << " at " << it->execDate << ".\n";
                tFees += it->fee;
                ++it;
                ++counter;
            }
            cout << "There " << pluralize(counter, "was", "were") << " a total of " << counter 
            << pluralize(counter, " transaction", " transactions") << ", 281Bank has collected " 
            << tFees << " dollars in fees.\n";
        }
};



int main(int argc, char * argv[]) {
    Bank b;

    b.getMode(argc, argv);
    b.talkToBank();


    return 0;
}