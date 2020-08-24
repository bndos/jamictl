#include <getopt.h>
#include <iostream>
#include <string>

#include <QtCore/QCoreApplication>
#include <QtCore>
#include <qdir.h>
#include <qobject.h>

#include "jamictl.h"

static void
print_info()
{
    std::cout << "Jami-cli, a simple dring command line interface." << std::endl;
}

static void
print_version()
{
    std::cout << "Jami-cli version " << 1.0 << std::endl;
    print_info();
}

static void
print_usage()
{
    std::cout << "Usage: jami-cli [-h] [-v]" << std::endl << std::endl;
    print_info();
}

#define no_argument       0
#define required_argument 1
#define optional_argument 2

static const constexpr struct option long_options[] = {{"help", no_argument, nullptr, 'h'},
                                                       {"version", no_argument, nullptr, 'v'},
                                                       {nullptr, 0, nullptr, 0}};
struct dht_params
{
    bool help {false};
    bool version {false};
    bool generate_identity {false};
    bool daemonize {false};
    bool service {false};
    bool peer_discovery {false};
    bool log {false};
    bool syslog {false};
    std::string logfile {};
    std::string bootstrap {};
    std::string proxyclient {};
    std::string pushserver {};
    std::string devicekey {};
    std::string persist_path {};
    std::string privkey_pwd {};
    std::string proxy_privkey_pwd {};
    std::string save_identity {};
    bool no_rate_limit {false};
    bool public_stable {false};
};

static dht_params
parseArgs(int argc, char** argv)
{
    dht_params params;
    int opt;
    std::string privkey;
    std::string proxy_privkey;
    while ((opt = getopt_long(argc, argv, "hidsvVDUPp:n:b:f:l:", long_options, nullptr)) != -1) {
        switch (opt) {
        case 'h':
            params.help = true;
            break;
        case 'v':
            params.version = true;
            break;
        default:
            break;
        }
    }
    if (params.save_identity.empty())
        params.privkey_pwd.clear();
    return params;
}

static QString
getAppPath()
{
    QDir dataDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation));
    dataDir.cdUp();
    return dataDir.absolutePath() + "/jami/";
}

static std::string
mkLogPath()
{
    QString dataDir = getAppPath();
    QDir appPath(dataDir);
    appPath.mkpath(dataDir);

    return dataDir.toStdString() + "jami-cli.log";
}

int
main(int argc, char* argv[])
{
    auto params = parseArgs(argc, argv);
    if (params.help) {
        print_usage();
        return 0;
    }
    if (params.version) {
        print_version();
        return 0;
    }

    QCoreApplication qapp(argc, argv);

    if(!freopen(mkLogPath().c_str(), "w", stderr))
        std::cout << "Could not redirect stderr" << std::endl;

    Jamictl jamictl;

    QObject::connect(&jamictl, SIGNAL(finished()), &qapp, SLOT(quit()));

    jamictl.run();

    return qapp.exec();
}
