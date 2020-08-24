#pragma once

#include <map>
#include <string>

#include <api/contactmodel.h>
#include <api/lrc.h>
#include <api/profile.h>
#include <qobjectdefs.h>

typedef const lrc::api::account::Info* AccountInfoPointer;

class Dringctrl
{
public:
    Dringctrl(const char* prompt = ">> ");
    ~Dringctrl();
    void init();
    void call(std::string contact, bool audioOnly);
    bool sendMessage(std::string uid, std::string message);
    void createRingAccount(std::string display_name, std::string username, std::string password);
    void getAllContacts(bool istable);
    std::string log(int index);
    void printAccounts(bool istable);
    void printConversations(bool istable);
    void printCalls(bool istable);
    void acceptCall();
    int totalAccounts();
    std::string removeAccount(int index);

    QMetaObject::Connection newAccountConnection_;
    QMetaObject::Connection rmAccountConnection_;
    QMetaObject::Connection nameRegistrationEnded_;
    QMetaObject::Connection registeredNameFound_;
    QMetaObject::Connection updateInteractionConnection_;
    QMetaObject::Connection newIncomingCallConnection_;
    QMetaObject::Connection callStartedConnection_;
    QMetaObject::Connection callEndedConnection_;
    QMetaObject::Connection callChangedConnection_;

private:
    void createAccount(lrc::api::profile::Type type,
                       const char* display_name,
                       const char* username,
                       const char* password,
                       const char* pin,
                       const char* archivePath);

    void slotAccountAddedFromLrc(const std::string& id);
    void slotAccountRemovedFromLrc(const std::string& id);
    void slotNewIncomingCall(const std::string& callId);
    void slotCallStarted(const std::string& callId);
    void slotCallEnded(const std::string& callId);
    void slotCallStatusChanged(const std::string& callId);

    std::map<std::string, std::string> calls;
    std::string incomingCallId;
    std::unique_ptr<lrc::api::Lrc> lrc_;
    AccountInfoPointer accountInfo_;
    const char* prompt_;
};
