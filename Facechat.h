#ifndef FACECHAT_H
#define FACECHAT_H

#include "stdinclude.hpp"

typedef long long int UserID;
typedef long long int ThreadID;
typedef long long int UniversalID;

class Facechat
{

public:

    struct Thread
    {
        std::vector<UserID> pastParticipant;
        std::vector<UserID> participants;

        std::string name;

        int messageCount;

        long long int threadID;
    };

    struct Group
    {
        int groupParticipantCount;
        std::vector<UserID> groupParticipantIDs;
        std::vector<std::string> groupParticipantNames;
        std::string groupName;
    };

    struct Attachment
    {
        long long int stickerID;

        std::string url;
        std::string previewUrl;

        enum Type {STICKER, PHOTO, VIDEO, UNKNOW} type;
        static Type stringToAttachmentType(const std::string& string)
        {
            if(string == "sticker")
                return STICKER;
            else if(string == "photo")
                return PHOTO;
            else if(string == "video")
                return VIDEO;

            return UNKNOW;
        }
    };

    struct Message
    {
        UserID from;
        UserID to = 0; //is not set when group
        std::string body;
        std::string senderName;
        std::string messageID;

        long long int conversationID = 0; //most of the time only set for group

        bool isGroup = false;
        Group group;

        bool asAttachment = false;
        Attachment attachment;
    };

    struct TypingStatus
    {
        UserID from;
        bool isTyping;
        bool fromMobile;
    };

    struct OnlineStatus
    {
        enum Status {ACTIVE, IDLE, INVISIBLE};
        static Status stringToStatus(const std::string& string)
        {
            if(string == "idle")
                return IDLE;
            else if(string == "active")
                return ACTIVE;

            return INVISIBLE;
        }

        time_t timestamp;

        Status status;
        Status onFacechatApplication;
        Status onMessenger;
        Status onWeb;
        Status onOther;

        UserID from;
    };

    struct ReadStatus
    {
        UserID from;
    };

    struct MessagingEvent
    {
        MessagingEvent() : message() {};

        union
        {
            Facechat::Message message;
            Facechat::TypingStatus typingStatus;
            Facechat::OnlineStatus onlineStatus;
            Facechat::ReadStatus readStatus;
        };

        enum Type {MESSAGE, TYPING_STATUS, ONLINE_STATUS, READ_STATUS} type = MESSAGE;

        MessagingEvent& operator=(const MessagingEvent& other)
        {
            if(this->type == MESSAGE && other.type != MESSAGE)
                (&this->message)->~Message();
            else if(this->type != MESSAGE && other.type == MESSAGE)
                new (&this->message) Message();

            if(other.type == MESSAGE)
                this->message = other.message;
            else if(other.type == TYPING_STATUS)
                this->typingStatus = other.typingStatus;
            else if(other.type == ONLINE_STATUS)
                this->onlineStatus = other.onlineStatus;
            else if(other.type == READ_STATUS)
                this->readStatus = other.readStatus;

            this->type = other.type;

            return *this;
        }

        MessagingEvent(const MessagingEvent& other) : message()
        {
            *this = other;
        }

        ~MessagingEvent()
        {
            if(type == MESSAGE)
                (&this->message)->~Message();
        }
    };

    struct UserInfo
    {
        std::string completeName;
        std::string firstName;
        std::string vanity;

        int gender; //1 = woman 2 = man
        bool isFriend;

        UserID id;
        std::string profilePicture;
        std::string profileUrl;
    };

    struct UserSearchReturn
    {
        std::string name;
        UserID id;
        std::string profilePicture;
        std::string profileUrl;

        int index;
        double score;
    };

    Facechat();
    virtual ~Facechat();

    int login(std::string email, std::string password);
    void logout();

    std::string sendMessage(std::string message, UniversalID sendTo, bool isGroup = false, std::vector<cpr::Pair> datas = std::vector<cpr::Pair>());
    std::string sendAttachement(std::string message, std::string filePath, UniversalID sendTo, bool isGroup = false);
    std::string sendUrl(std::string message, std::string url, UniversalID sendTo, bool isGroup = false);
    std::string sendSticker(std::string stickerID, UniversalID sendTo, bool isGroup = false);
    std::string createGroup(std::string message, std::vector<UserID> otherUsers);

    void deleteMessage(std::string messageID);
    void markAsRead(UniversalID threadID);
    void setTypingStatus(UniversalID threadID, bool typing, bool isGroup = false);

    void deleteThread(UniversalID id);
    Facechat::Thread getThread(UniversalID id);
    std::vector<Facechat::Message> readThread(UniversalID id, int offset = 0, int limit = 20, bool isGroup = false);
    std::vector<Facechat::Thread> findThread(std::string name, int offset = 0, int limit = 20);
    std::vector<Facechat::Thread> listThread(int offset = 0, int limit = 20);

    UserInfo getUserInfo(UserID id);
    std::vector<UserSearchReturn> findUser(std::string name);
    std::vector<UserID> getFriendList(UserID id);
    std::vector<std::pair<UserID, time_t>> getOnlineFriend(bool includeMobile = false);

    void setGroupTitle(ThreadID id, std::string title);
    void addUserToGroup(UserID userID, ThreadID group);
    void removeUserFromGroup(UserID userID, ThreadID group);

    bool pullMessage(MessagingEvent& event);

    std::string getUserID();

protected:
private:
    std::string send(std::vector<cpr::Pair>& data);

    std::string getFacechatURL(std::string url);
    json uploadFile(std::string filePath);

    void update();

    void defaultPayload(std::vector<cpr::Pair>& payloadsPairs);

    static std::string generateOfflineThreadingID();

    static json responseToJson(cpr::Response& response);

    static Thread parseThread(json& j);
    static Message parseMessage(json& j);

    std::deque<MessagingEvent> mEvents;
    std::mutex mEventsMutex;

    std::thread mUpdateThread;
    std::atomic<bool> mStopThread;

    cpr::Session mPostSession;
    cpr::Session mUpdateSession;

    cpr::Cookies mDefaultCookie;
    std::vector<cpr::Pair> mDefaultPayloads;

    std::string mUserID;
    std::string mDTSG;
    std::string mSticky;
    std::string mPool;

    std::string seq = "0";

    const std::string user_agent = "Mozilla/5.0 (Wihttps://www.facebook.com/ajax/mercury/threadlist_info.phpndows NT 5.1; rv:31.0) Gecko/20100101 Firefox/31.0";

    const std::string login_url = "https://www.facebook.com/login.php";
    const std::string logout_url = "https://www.facebook.com/logout.php";
    const std::string get_friends_list_part_1 = "https://www.facebook.com/profile.php?sk=friends&id=";
    const std::string get_friends_list_part_2 = "https://www.facebook.com/ajax/pagelet/generic.php/AllFriendsAppCollectionPagelet";
    const std::string get_online_friends_url = "https://www.facebook.com/ajax/chat/buddy_list.php";
    const std::string find_user_url = "https://www.facebook.com/ajax/typeahead/search.php";
    const std::string get_user_info = "https://www.facebook.com/chat/user_info/";
    const std::string access_token_url = "https://developers.facebook.com/tools/explorer/{}/permissions?version=v2.1&__user={}&__a=1&__dyn=5U463-i3S2e4oK4pomXWo5O12wAxu&__req=2&__rev=1470714";
    const std::string thread_info_url = "https://www.facebook.com/ajax/mercury/thread_info.php";
    const std::string find_thread_url = "https://www.facebook.com/ajax/mercury/search_threads.php";
    const std::string list_thread_url = "https://www.facebook.com/ajax/mercury/threadlist_info.php";
    const std::string delete_thread_url = "https://www.facebook.com/ajax/mercury/delete_thread.php";
    const std::string delete_message_url = "https://www.facebook.com/ajax/mercury/delete_messages.php";
    const std::string send_message_url = "https://www.facebook.com/ajax/mercury/send_messages.php";
    const std::string set_read_status_url = "https://www.facebook.com/ajax/mercury/change_read_status.php";
    const std::string set_typing_status_url = "https://www.facebook.com/ajax/messaging/typ.php";
    const std::string remove_user_from_group_url = "https://www.facebook.com/chat/remove_participants";
    const std::string pull_message_url = "https://0-edge-chat.facebook.com/pull?channel=p_$USER_ID&partition=-2&clientid=3396bf29&cb=gr6l&idle=0&cap=8&msgs_recv=0&uid=$USER_ID&viewer_uid=$USER_ID&state=active&seq=$SEQ&sticky_token=$STICKY&sticky_pool=$POOL";
    const std::string sticky_url = "https://0-edge-chat.facebook.com/pull?channel=p_$USER_ID&partition=-2&clientid=3396bf29&cb=gr6l&idle=0&cap=8&msgs_recv=0&uid=$USER_ID&viewer_uid=$USER_ID&state=active&seq=0";
};

#endif // FACECHAT_H
