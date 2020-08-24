#include "jamictl.h"
#include "api/lrc.h"
#include "api/newaccountmodel.h"
#include "api/profile.h"
#include "dringctrl.h"
#include "tabulate.hpp"

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <math.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <set>
#include <sstream>
#include <QObject>
#include <qcoreapplication.h>
#include <qobjectdefs.h>

static const constexpr char* PROMPT = "\x1B[34m>> \033[0m";

Jamictl::Jamictl(QObject* parent)
    : QObject(parent)
    , dringctrl(PROMPT)
{
    dringctrl.init();
}

Jamictl::~Jamictl() {}

static void
print_help(bool logged)
{
    tabulate::Table table;

    std::cout << "\x1B[33mJami command line interface (CLI)\033[0m" << std::endl;
    std::cout << "\x1B[36mPossible commands:\033[0m" << std::endl
              << " h,  help   Print this help message." << std::endl
              << " q,  quit   Quit the program." << std::endl;

    std::string loggedtext = logged ? ""
                                    : " \x1B[31m(switch to an account to see more options)\033[0m";
    std::cout << std::endl << "\x1B[36mJami control" << loggedtext << ":\033[0m" << std::endl;

    table.add_row({"la", "", "Lists all local accounts."});
    table.add_row({"lat", "", "Lists all local accounts in a table format."});
    table.add_row({"na", "", "Create a new local ring account interactively"});
    table.add_row({"rma", "", "Remove a local ring account"});
    table.add_row({"log",
                   "[index(optional)]",
                   "Switch to the indexed account or interactively (if no argument provided)"});

    if (logged) {
        table.add_row({"vc", "[hash/username]", "Video call someone from its hash."});
        table.add_row({"c", "[hash/username]", "Audio call someone from its hash."});
        table.add_row({"lc", "", "Lists all contacts."});
        table.add_row({"lct", "", "Lists all contacts in a table format."});
        table.add_row({"lco", "", "Lists all conversations."});
        table.add_row({"lcot", "", "Lists all conversations in a table format."});
        table.add_row(
            {"sms", "[conversation uid] [message]", "Send a conversation on the conversation uid."});
        table.add_row({"ans", "", "Answer the current incoming call."});
    }

    table.format()
        .corner_top_left("")
        .corner_top_right("")
        .corner_bottom_left("")
        .corner_bottom_right("")
        .border_top("")
        .border_bottom("")
        .border_left("")
        .border_right("");

    std::cout << table << std::endl;
}

static std::string
readLine(const char* prefix = PROMPT)
{
    const char* line_read = readline(prefix);
    if (line_read && *line_read)
        add_history(line_read);

    return line_read ? std::string(line_read) : std::string("\0", 1);
}

static int
getPositiveInt(std::string line)
{
    std::istringstream iss(line);
    int choice;

    if (iss >> choice && choice >= 0)
        return choice;

    return -1;
}

int
Jamictl::chooseAccount()
{
    while (true) {
        std::cout << "Choose the account to connect to " << std::endl;
        dringctrl.printAccounts(false);

        std::string prompt = "[0-" + std::to_string(dringctrl.totalAccounts() - 1) + "]: ";
        std::string line   = readLine(prompt.c_str());

        int choice = getPositiveInt(line);
        if (choice >= 0 && choice < dringctrl.totalAccounts()) {
            return choice;
        }

        std::cout << "Invalid choice" << std::endl;
    }
}

void
Jamictl::createAccount()
{
    std::string alias    = readLine("display name: ");
    std::string username = readLine("username (leave empty for no username): ");
    std::string password = readLine("password (leave empty for no password): ");
    std::cout << "Create user: \"alias: " << alias << " \" \"username: " << username << "\""
              << std::endl;

    std::string answer;
    while (answer.compare("y") != 0 && answer.compare("n") != 0) {
        std::string line;
        line = readLine("(y/n)? ");
        std::istringstream iss(line);
        iss >> answer;
    }

    if (answer == "y")
        dringctrl.createRingAccount(alias, username, password);
    else
        std::cout << "Account creation cancelled" << std::endl;
}

void
Jamictl::mainLoop()
{
    std::cout << "(type 'h' or 'help' for a list of possible commands)" << std::endl;
    bool logged = false;

    while (true) {
        std::string line = readLine();

        if (!line.empty() && line[0] == '\0')
            break;

        std::istringstream iss(line);
        std::string op, idstr, value, acc, keystr, pushServer, deviceKey;
        iss >> op;

        if (op == "q" || op == "exit" || op == "quit") {
            break;
        } else if (op == "h" || op == "help") {
            print_help(logged);
            continue;
        } else if (op == "lr") {
            std::cout << "IPv4 routing table:" << std::endl;
            std::cout << "IPv6 routing table:" << std::endl;
            continue;
        } else if (op == "la") {
            dringctrl.printAccounts(false);
            continue;
        } else if (op == "lat") {
            dringctrl.printAccounts(true);
            continue;
        } else if (op == "log") {
            iss >> acc;
            if (acc.empty()) {
                std::string acc = dringctrl.log(chooseAccount());
                std::cout << "Logged to " << acc << std::endl;
                logged = true;
                continue;
            }

            int choice;
            choice = getPositiveInt(acc);
            if (choice >= 0 && choice < dringctrl.totalAccounts()) {
                dringctrl.log(choice);
                std::cout << "Switched to account " << dringctrl.log(choice) << std::endl;
                logged = true;
            } else {
                std::cout << "Invalid choice" << std::endl;
            }
            continue;
        } else if (op == "rma") {
            iss >> acc;
            if (acc.empty()) {
                dringctrl.removeAccount(chooseAccount());
                if (dringctrl.totalAccounts() == 0)
                    logged = false;
                continue;
            }

            int choice;
            choice = getPositiveInt(acc);
            if (choice >= 0 && choice < dringctrl.totalAccounts()) {
                dringctrl.removeAccount(choice);
                if (dringctrl.totalAccounts() == 0)
                    logged = false;
            } else {
                std::cout << "Invalid choice" << std::endl;
            }
            continue;
        } else if (op == "na") {
            createAccount();
            continue;
        }

        if (op.empty())
            continue;

        if (!logged) {
            std::cout << "Unknown command: " << op << std::endl;
            std::cout << " (type 'h' or 'help' for a list of possible commands)" << std::endl;
            continue;
        }

        static const std::set<std::string>
            VALID_OPS {"vc", "c", "lc", "lct", "lco", "lcot", "sms", "ans", "hg"};

        if (VALID_OPS.find(op) == VALID_OPS.cend()) {
            std::cout << "Unknown command: " << op << std::endl;
            std::cout << " (type 'h' or 'help' for a list of possible commands)" << std::endl;
            continue;
        }

        if (op == "lc") {
            dringctrl.getAllContacts(false);
        }

        if (op == "lct") {
            dringctrl.getAllContacts(true);
        }

        if (op == "lco") {
            dringctrl.printConversations(false);
        }

        if (op == "lcot") {
            dringctrl.printConversations(true);
        }

        if (op == "c") {
            iss >> idstr;
            if (idstr.empty()) {
                std::cout << "Syntax error: invalid hash/username." << std::endl;
                continue;
            }

            std::cout << "Calling " << idstr << std::endl;
            dringctrl.call(idstr, true);
        }

        if (op == "vc") {
            iss >> idstr;
            if (idstr.empty()) {
                std::cout << "Syntax error: invalid hash/username." << std::endl;
                continue;
            }

            std::cout << "Video calling " << idstr << std::endl;
            dringctrl.call(idstr, false);
        }

        if (op == "sms") {
            iss >> idstr >> std::quoted(value);
            if (idstr.empty()) {
                std::cout << "Syntax error: invalid conversation uid." << std::endl;
                continue;
            }

            if (value.empty()) {
                std::cout << "Syntax error: no message specified." << std::endl;
                continue;
            }

            if (!dringctrl.sendMessage(idstr, value))
                std::cout << "No such conversation" << std::endl;
            else
                std::cout << "Sending message to conversation " << idstr << std::endl;
        }

        if (op == "ans") {
            dringctrl.acceptCall();
        }
    }

    std::cout << "Stopping Jami..." << std::endl;
    thread.quit();
}

void
Jamictl::run()
{
    moveToThread(&thread);
    connect(&thread, SIGNAL(started()), this, SLOT(mainLoop()));
    connect(&thread, SIGNAL(finished()), this, SLOT(exit()));
    thread.start();
}

void
Jamictl::exit()
{
    emit finished();
}
