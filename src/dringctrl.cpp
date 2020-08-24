#include <iostream>
#include <ostream>
#include <string>
#include <utility>
#include <QObject>
#include <qmap.h>
#include <qobject.h>

#include <api/account.h>
#include <api/contact.h>
#include <api/conversationmodel.h>
#include <api/newaccountmodel.h>
#include <api/newcallmodel.h>
#include <api/profile.h>
#include <namedirectory.h>

#include "api/call.h"
#include "dringctrl.h"
#include "tabulate.hpp"

typedef struct AddedAccountInfo_
{
    std::string alias;
    std::string username;
    std::string password;
} AddedAccountInfo;

AddedAccountInfo addedAccountInfo;

Dringctrl::Dringctrl(const char* prompt)
    : accountInfo_(nullptr)
{
    prompt_ = prompt;
    try {
        lrc_ = std::make_unique<lrc::api::Lrc>();
    } catch (const char* e) {
        std::cout << e << std::endl;
        exit(0);
    }
}

Dringctrl::~Dringctrl()
{
    QObject::disconnect(newAccountConnection_);
    QObject::disconnect(rmAccountConnection_);
    QObject::disconnect(nameRegistrationEnded_);
    QObject::disconnect(registeredNameFound_);
    QObject::disconnect(updateInteractionConnection_);
    QObject::disconnect(callChangedConnection_);
    QObject::disconnect(callStartedConnection_);
    QObject::disconnect(callEndedConnection_);
    QObject::disconnect(newIncomingCallConnection_);
}

void
Dringctrl::init()
{
    newAccountConnection_  = QObject::connect(&lrc_->getAccountModel(),
                                             &lrc::api::NewAccountModel::accountAdded,
                                             [this](const QString& id) {
                                                 slotAccountAddedFromLrc(id.toStdString());
                                             });
    rmAccountConnection_   = QObject::connect(&lrc_->getAccountModel(),
                                            &lrc::api::NewAccountModel::accountRemoved,
                                            [this](const QString& id) {
                                                slotAccountRemovedFromLrc(id.toStdString());
                                            });
    nameRegistrationEnded_ = QObject::connect(
        &lrc_->getAccountModel(),
        &lrc::api::NewAccountModel::nameRegistrationEnded,
        [=](const QString&, lrc::api::account::RegisterNameStatus status, const QString& name) {
            if (name == "")
                return;

            switch (status) {
            case lrc::api::account::RegisterNameStatus::SUCCESS: {
                std::cout << "\nName \"" << name.toStdString() << "\" registered successfully"
                          << std::endl
                          << prompt_ << std::flush;
                break;
            }
            case lrc::api::account::RegisterNameStatus::INVALID_NAME:
                std::cout << "\nUnable to register name \"" << name.toStdString()
                          << "\" (Invalid name). Your username should contains "
                             "between 3 and 32 alphanumerics characters (or underscore)."
                          << std::endl
                          << prompt_ << std::flush;
                break;
            case lrc::api::account::RegisterNameStatus::WRONG_PASSWORD:
                std::cout << "\nUnable to register name \"" << name.toStdString()
                          << "\" (Wrong password)." << std::endl
                          << prompt_ << std::flush;
                break;
            case lrc::api::account::RegisterNameStatus::ALREADY_TAKEN:
                std::cout << "\nUnable to register name \"" << name.toStdString()
                          << "\" (Username already taken)" << std::endl
                          << prompt_ << std::flush;
                break;
            case lrc::api::account::RegisterNameStatus::NETWORK_ERROR:
                std::cout << "\nUnable to register name \"" << name.toStdString()
                          << "\" (Network error) - check your connection." << std::endl
                          << prompt_ << std::flush;
                break;
            case lrc::api::account::RegisterNameStatus::INVALID:
                break;
            }
        });
}

void
Dringctrl::createAccount(lrc::api::profile::Type type,
                         const char* display_name,
                         const char* username,
                         const char* password,
                         const char* pin,
                         const char* archivePath)
{
    addedAccountInfo.alias    = display_name ? display_name : "";
    addedAccountInfo.username = username ? username : "";
    addedAccountInfo.password = password ? password : "";

    auto accountId = lrc::api::NewAccountModel::createNewAccount(type,
                                                                 addedAccountInfo.alias.c_str(),
                                                                 archivePath ? archivePath : "",
                                                                 addedAccountInfo.password.c_str(),
                                                                 pin ? pin : "");
}

void
Dringctrl::createRingAccount(std::string display_name, std::string username, std::string password)
{
    createAccount(lrc::api::profile::Type::RING,
                  display_name.c_str(),
                  username.c_str(),
                  password.c_str(),
                  NULL,
                  NULL);
}

void
Dringctrl::printAccounts(bool istable)
{
    tabulate::Table table;

    auto accounts = lrc_->getAccountModel().getAccountList();
    if (accounts.size() == 0)
        std::cout << "No accounts" << std::endl;

    if (istable) {
        table.add_row({"index", "accountId", "hash", "alias", "username"});
        for (auto& cell : table[0]) {
            cell.format()
                .font_style({tabulate::FontStyle::underline})
                .font_align(tabulate::FontAlign::center);
        }
    }
    int i = 0;
    for (auto account : accounts) {
        const lrc::api::account::Info& accountInfo = lrc_->getAccountModel().getAccountInfo(account);

        std::string indicator;
        if (accountInfo_)
            indicator = (!accountInfo_->id.compare(accountInfo.id)) ? "*" : " ";

        table.add_row({std::to_string(i++) + indicator,
                       accountInfo.id.toStdString(),
                       accountInfo.profileInfo.uri.toStdString(),
                       accountInfo.profileInfo.alias.toStdString(),
                       accountInfo.registeredName.toStdString()});
    }

    if (!istable)
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

int
Dringctrl::totalAccounts()
{
    return lrc_->getAccountModel().getAccountList().size();
}

std::string
Dringctrl::log(int index)
{
    if (totalAccounts() < index || index < 0) {
        std::cout << "No such index account" << std::endl;
        return "";
    }

    if (accountInfo_) {
        QObject::disconnect(updateInteractionConnection_);
        QObject::disconnect(newIncomingCallConnection_);
        QObject::disconnect(callStartedConnection_);
        QObject::disconnect(callEndedConnection_);
        QObject::disconnect(callChangedConnection_);
    }

    std::string id = lrc_->getAccountModel().getAccountList().at(index).toStdString();
    accountInfo_   = &lrc_->getAccountModel().getAccountInfo(id.c_str());

    if (!accountInfo_)
        return "";

    callChangedConnection_ = QObject::connect(&*accountInfo_->callModel,
                                              &lrc::api::NewCallModel::callStatusChanged,
                                              [this](const QString& callId) {
                                                  slotCallStatusChanged(callId.toStdString());
                                              });

    callStartedConnection_ = QObject::connect(&*accountInfo_->callModel,
                                              &lrc::api::NewCallModel::callStarted,
                                              [this](const QString& callId) {
                                                  slotCallStarted(callId.toStdString());
                                              });

    callEndedConnection_ = QObject::connect(&*accountInfo_->callModel,
                                            &lrc::api::NewCallModel::callEnded,
                                            [this](const QString& callId) {
                                                slotCallEnded(callId.toStdString());
                                            });

    updateInteractionConnection_ = QObject::connect(
        &*accountInfo_->conversationModel,
        &lrc::api::ConversationModel::interactionStatusUpdated,
        [=](const QString&, uint64_t, lrc::api::interaction::Info msg) {
            switch (msg.status) {
            case lrc::api::interaction::Status::SUCCESS:
                std::cout << "\nDelivery status: sent\n" << prompt_ << std::flush;
                break;
            case lrc::api::interaction::Status::FAILURE:
            case lrc::api::interaction::Status::TRANSFER_ERROR:
                std::cout << "\nDelivery status: failure\n" << prompt_ << std::flush;
                break;
            case lrc::api::interaction::Status::TRANSFER_UNJOINABLE_PEER:
                std::cout << "\nDelivery status: unjoinable peer\n" << prompt_ << std::flush;
                break;
            case lrc::api::interaction::Status::SENDING:
                std::cout << "\nDelivery status: sending\n" << prompt_ << std::flush;
                break;
            case lrc::api::interaction::Status::TRANSFER_CREATED:
                std::cout << "\nDelivery status: connecting\n" << prompt_ << std::flush;
                break;
            case lrc::api::interaction::Status::TRANSFER_ACCEPTED:
                std::cout << "\nDelivery status: accepted\n" << prompt_ << std::flush;
                break;
            case lrc::api::interaction::Status::TRANSFER_CANCELED:
                std::cout << "\nDelivery status: canceled\n" << prompt_ << std::flush;
                break;
            case lrc::api::interaction::Status::TRANSFER_ONGOING:
                std::cout << "\nDelivery status: ongoing\n" << prompt_ << std::flush;
                break;
            case lrc::api::interaction::Status::TRANSFER_AWAITING_PEER:
                std::cout << "\nDelivery status: awaiting peer\n" << prompt_ << std::flush;
                break;
            case lrc::api::interaction::Status::TRANSFER_AWAITING_HOST:
                std::cout << "\nDelivery status: awaiting host\n" << prompt_ << std::flush;
                break;
            case lrc::api::interaction::Status::TRANSFER_TIMEOUT_EXPIRED:
                std::cout << "\nDelivery status: awaiting peer timeout\n" << prompt_ << std::flush;
                break;
            case lrc::api::interaction::Status::TRANSFER_FINISHED:
                std::cout << "\nDelivery status: finished\n" << prompt_ << std::flush;
                break;
            case lrc::api::interaction::Status::INVALID:
            case lrc::api::interaction::Status::UNKNOWN:
            case lrc::api::interaction::Status::DISPLAYED:
            case lrc::api::interaction::Status::COUNT__:
            default:
                break;
            }
        });

    newIncomingCallConnection_ = QObject::connect(&*accountInfo_->callModel,
                                                  &lrc::api::NewCallModel::newIncomingCall,
                                                  [this](const QString&, const QString& callId) {
                                                      slotNewIncomingCall(callId.toStdString());
                                                  });

    std::string username = accountInfo_->registeredName.toStdString();
    if (username.empty())
        return accountInfo_->profileInfo.uri.toStdString();

    return username;
}

std::string
Dringctrl::removeAccount(int index)
{
    QString id          = lrc_->getAccountModel().getAccountList().at(index);
    auto removedAccount = &lrc_->getAccountModel().getAccountInfo(id);

    if (accountInfo_ != nullptr && accountInfo_->id == id)
        std::cout << "Removing current account" << std::endl;

    lrc_->getAccountModel().removeAccount(id);

    std::string username = removedAccount->registeredName.toStdString();
    if (username.empty())
        return removedAccount->profileInfo.uri.toStdString();

    return username;
}

void
Dringctrl::call(std::string contact, bool audioOnly)
{
    if (accountInfo_ == nullptr)
        return;

    std::string uri = "ring:" + contact;
    accountInfo_->callModel->createCall(static_cast<QString>(uri.c_str()), audioOnly);
}

void
Dringctrl::getAllContacts(bool istable)
{
    tabulate::Table table;

    if (istable) {
        table.add_row({"username", "hash"});
        for (auto& cell : table[0]) {
            cell.format()
                .font_style({tabulate::FontStyle::underline})
                .font_align(tabulate::FontAlign::center);
        }
    }

    auto contacts = accountInfo_->contactModel->getAllContacts();

    if (contacts.size() == 0) {
        std::cout << "no contacts" << std::endl;
        return;
    }

    for (auto contact : contacts) {
        auto contactInfo = accountInfo_->contactModel->getContact(contact.profileInfo.uri);

        if (!contactInfo.profileInfo.uri.isEmpty())
            table.add_row({contactInfo.registeredName.toStdString(),
                           contactInfo.profileInfo.uri.toStdString()});
    }

    if (!istable)
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

void
Dringctrl::slotNewIncomingCall(const std::string& callId)
{
    if (!accountInfo_)
        return;

    try {
        auto call          = accountInfo_->callModel->getCall(callId.c_str());
        auto peer          = call.peerUri.remove("ring:");
        auto& contactModel = accountInfo_->contactModel;
        QString name = "", uri = "";
        std::string notifId = "";
        try {
            auto contactInfo = contactModel->getContact(peer);
            uri              = contactInfo.profileInfo.uri;
            name             = contactInfo.profileInfo.alias;
            if (name.isEmpty()) {
                name = contactInfo.registeredName;
                if (name.isEmpty()) {
                    name = contactInfo.profileInfo.uri;
                }
            }
            notifId = accountInfo_->id.toStdString() + ":call:" + callId;
        } catch (...) {
            std::cerr << "Can't get contact for account " << accountInfo_->id.toStdString()
                      << ". Don't show notification";
            return;
        }

        name.remove('\r');
        calls.emplace(callId, name.toStdString());
        incomingCallId = callId;

        std::string body = "\n" + name.toStdString() + " is calling you!\n";
        std::cout << body << prompt_ << std::flush;
    } catch (const std::exception& e) {
        std::cerr << "Can't get call" << callId << "for this account.";
    }
}

void
Dringctrl::slotAccountAddedFromLrc(const std::string& id)
{
    auto& accountModel      = lrc_->getAccountModel();
    const auto& accountInfo = accountModel.getAccountInfo(id.c_str());

    accountModel.setAlias(id.c_str(), addedAccountInfo.alias.c_str());

    if (!addedAccountInfo.username.empty()) {
        auto prop = accountInfo.accountModel->getAccountConfig(id.c_str());
        accountModel.registerName(id.c_str(),
                                  addedAccountInfo.password.c_str(),
                                  addedAccountInfo.username.c_str());
    }

    std::cout << "\nAccount added: " << id << std::endl << prompt_ << std::flush;
}

void
Dringctrl::slotAccountRemovedFromLrc(const std::string& id)
{
    if (totalAccounts() == 0)
        std::cout << "\nDeleted last account!";
    else if (accountInfo_ != nullptr && accountInfo_->id.toStdString() == id)
        std::cout << "\nDeleted selected account\n"
                  << "Logging to: " << log(0);

    std::cout << "\nSuccessfully removed account " << id << std::endl << prompt_ << std::flush;

    try {
        lrc_->getAccountModel().flagFreeable(id.c_str());
    } catch (std::exception& e) {
        std::cerr << "Error while flagging " << id << " for removal: '" << e.what() << "'"
                  << std::endl;
    } catch (...) {
        std::cerr << "Unexpected failure while flagging " << id << " for removal." << std::endl;
    }
}

void
Dringctrl::printConversations(bool istable)
{
    if (!accountInfo_) {
        std::cout << "No account selected" << std::endl;
        return;
    }

    auto conversations = accountInfo_->conversationModel->allFilteredConversations();
    if (conversations.size() == 0)
        std::cout << "No conversations" << std::endl;

    tabulate::Table table;

    if (istable) {
        table.add_row({"uid", "hash", "username", "alias", "lastInteraction"});
        for (auto& cell : table[0]) {
            cell.format()
                .font_style({tabulate::FontStyle::underline})
                .font_align(tabulate::FontAlign::center);
        }
    }

    for (auto conversation : conversations) {
        auto contactUri  = conversation.participants.front().toStdString();
        auto contactInfo = accountInfo_->contactModel->getContact(contactUri.c_str());
        auto lastMessage = conversation.interactions.empty()
                               ? ""
                               : conversation.interactions.at(conversation.lastMessageUid)
                                     .body.toStdString();

        if (lastMessage.size() > 40) {
            lastMessage.resize(40);
            lastMessage.append("...");
        }

        table.add_row({conversation.uid.toStdString(),
                       contactUri,
                       contactInfo.registeredName.toStdString(),
                       contactInfo.profileInfo.alias.toStdString(),
                       lastMessage});
    }

    if (!istable)
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

void
Dringctrl::printCalls(bool istable)
{
    if (!accountInfo_) {
        std::cout << "No account currently selected" << std::endl;
        return;
    }

    if (calls.empty()) {
        std::cout << "\nNo current calls\n" << prompt_ << std::flush;
        return;
    }

    tabulate::Table table;

    if (istable) {
        table.add_row({"index", "callId", "contact"});
        for (auto& cell : table[0]) {
            cell.format()
                .font_style({tabulate::FontStyle::underline})
                .font_align(tabulate::FontAlign::center);
        }
    }

    int i = 0;
    for (auto call : calls) {
        table.add_row({std::to_string(i), call.first, call.second});
    }

    if (!istable)
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

void
Dringctrl::acceptCall()
{
    if (!accountInfo_)
        std::cout << "\nNo account currently selected" << std::endl;

    accountInfo_->callModel->accept(incomingCallId.c_str());
}

bool
Dringctrl::sendMessage(std::string uid, std::string message)
{
    auto conversation = accountInfo_->conversationModel->getConversationForUID(uid.c_str());

    if (conversation.uid.isEmpty())
        return false;

    accountInfo_->conversationModel->sendMessage(uid.c_str(), message.c_str());
    return true;
}

void
Dringctrl::slotCallStarted(const std::string& callId)
{
    if (!calls.count(callId))
        return;

    std::cout << "\nCall with " << calls.at(callId) << " started\n" << prompt_ << std::flush;
}

void
Dringctrl::slotCallEnded(const std::string& callId)
{
    if (incomingCallId == callId)
        incomingCallId = "";

    if (!calls.count(callId))
        return;

    std::cout << "\nCall with " << calls.at(callId) << " ended\n" << prompt_ << std::flush;
    calls.erase(callId);
}

void
Dringctrl::slotCallStatusChanged(const std::string& callId)
{
    if (!accountInfo_) {
        return;
    }

    try {
        auto call = accountInfo_->callModel->getCall(callId.c_str());
        auto peer = call.peerUri.remove("ring:");

        if (call.status == lrc::api::call::Status::CONNECTING
            || call.status == lrc::api::call::Status::SEARCHING
            || call.status == lrc::api::call::Status::OUTGOING_RINGING
            || call.status == lrc::api::call::Status::TERMINATING)
            std::cout << "\n"
                      << "Call with " << peer.toStdString()
                      << " status: " << lrc::api::call::to_string(call.status).toStdString() << "\n"
                      << prompt_ << std::flush;

    } catch (const std::exception& e) {
        std::cerr << "Can't get call " << callId.c_str() << " for this account." << std::endl;
    }
}
