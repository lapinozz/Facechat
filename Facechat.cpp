#include "Facechat.h"

Facechat::Facechat()
{
//    mDefaultCookie["_js_reg_fb_gate"] = login_url;
    mDefaultCookie["reg_fb_gate"] = login_url;
    mDefaultCookie["reg_fb_ref"] = login_url;
//    mDefaultCookie["datr"] = "noZkV5yIO_tft56tOn_rZlet";
//    mDefaultCookie["_js_session"] = "false";
//    mDefaultCookie["presence"] = "EDvF3EtimeF1466382654EuserFA21B00803925467A2EstateFDt2F_5bDiFA2user_3a1B03742384564A2ErF1C_5dElm2FA2user_3a1B03742384564A2Euct2F1466382291289EtrFnullEtwF1685193339EatF1466382654078G466382654617CEchFDp_5f1B00803925467F2CC";

    mPostSession.SetHeader(cpr::Header {{"User-Agent", user_agent}});
    mUpdateSession.SetHeader(cpr::Header {{"User-Agent", user_agent}});

    mPostSession.SetCookies(mDefaultCookie);
    mUpdateSession.SetCookies(mDefaultCookie);
}

Facechat::~Facechat()
{
    logout();
}

int Facechat::login(std::string email, std::string password)
{
    mPostSession.SetUrl(cpr::Url {login_url});
    mPostSession.SetPayload(cpr::Payload {{"email", email}, {"pass", password}});
    mUpdateSession.SetPayload(cpr::Payload {{"email", email}, {"pass", password}});
    cpr::Response r = mPostSession.Post();

    int pos = r.text.find("\"USER_ID\":\"");
    if(pos != std::string::npos)
        mUserID = r.text.substr(pos+11, r.text.find("\"", pos+11) - (pos+11));

    pos = r.text.find("name=\"fb_dtsg\" value=\"");
    if(pos != std::string::npos)
        mDTSG = r.text.substr(pos+22, r.text.find("\"", pos+22) - (pos+22));

    pos = r.text.find("revision\":");
    if(pos != std::string::npos)
        mRevision = r.text.substr(pos+10, r.text.find(",\"", pos+10) - (pos+10));

//    pos = 0;
//    while((pos = r.text.find("\"_js_", pos + 1)) != std::string::npos)
//    {
//        std::cout << r.text.substr(pos - 10, 100) << std::endl;
//        pos += 5;
//        auto end = r.text.find("\",\"", pos ) + 3;
//        std::cout << r.text.substr(end, r.text.find("\",", end) - end) << std::endl;
//        r.cookies[r.text.substr(pos, end - pos - 3)] = r.text.substr(end, r.text.find("\",", end) - end);
//    }
//
//     for(auto& pair : r.cookies.map_)
//    {
//        std::cout << pair.first << ": " << pair.second << std::endl;

//    }

//    r.cookies.map_.erase("s");
//    r.cookies["reg_fb_gate"] = login_url;
//    mPostSession.SetCookies(r.cookies);

    r.text = ""; //for debug cuz the text is too big and make gdb lag

    mPostSession.SetUrl(cpr::Url({replaceAll(sticky_url, "$USER_ID", mUserID)}));
    r = mPostSession.Get();

    pos = r.text.find("\"sticky\":\"");
    if(pos != std::string::npos)
        mSticky = r.text.substr(pos+10, r.text.find("\"", pos+10) - (pos+10));

    pos = r.text.find("\"pool\":\"");
    if(pos != std::string::npos)
        mPool = r.text.substr(pos+8, r.text.find("\"", pos+8) - (pos+8));

//    pos = 0;
//    while((pos = r.text.find("\"_js_", pos + 1)) != std::string::npos)
//    {
//        std::cout << r.text.substr(pos - 10, 100) << std::endl;
//        pos += 5;
//        auto end = r.text.find("\",\"", pos ) + 3;
//        std::cout << r.text.substr(end, r.text.find("\",", end) - end) << std::endl;
//        r.cookies[r.text.substr(pos, end - pos - 3)] = r.text.substr(end, r.text.find("\",", end) - end);
//    }

//    r.cookies["_js_reg_fb_gate"] = login_url;
//    r.cookies["reg_fb_gate"] = login_url;
//    r.cookies["datr"] = "noZkV5yIO_tft56tOn_rZlet";
//    r.cookies["p"] = "1";

//    int erase = 1;
//    for(auto& pair : r.cookies.map_)
//    {
//        std::cout << pair.first << ": " << pair.second << std::endl;
//        if(!erase--)
//        r.cookies.map_.erase(pair.first);
//    }

//    mPostSession.SetCookies(r.cookies);
//    mPostSession.SetCookies({});
//    mPostSession.SetCookies(mDefaultCookie);

//    mPostSession.SetUrl(cpr::Url {login_url});
//    r = mPostSession.Post();

    if(mUserID.size() && mDTSG.size() && mSticky.size() && mPool.size())
    {
        mStopThread = false;
        mUpdateThread = std::thread(std::bind(&Facechat::update, this));

//        mUpdateThread.detach();
        return 1;
    }
    else
        return 0;
}

void Facechat::logout()
{
    std::vector<cpr::Pair> payloadsPairs;
    defaultPayload(payloadsPairs);
    mPostSession.SetPayload(cpr::Payload {range_to_initializer_list(payloadsPairs.begin(), payloadsPairs.end())});
    mPostSession.SetUrl(logout_url);
    mPostSession.Post();

    mStopThread = true;
    mDeadMutex.lock();

    mUserID.clear();
    mDTSG.clear();
    mSticky.clear();
    mPool.clear();

    mPostSession.~Session();
    mUpdateSession.~Session();
    new (&mPostSession) cpr::Session();
    new (&mUpdateSession) cpr::Session();

    mDefaultPayloads.clear();
}

bool Facechat::pullMessage(MessagingEvent& event)
{
    std::lock_guard<std::mutex> lock(mEventsMutex);
    if(mEvents.empty())
        return 0;

    event = mEvents.back();
    mEvents.pop_back();

    return 1;
}

void Facechat::update()
{
    std::lock_guard<std::mutex> lock(mDeadMutex);

    mUpdateSession.SetUrl(cpr::Url{login_url});
    mUpdateSession.Post();

    while(!mStopThread)
    {
        std::string url = replaceAll(pull_message_url, "$USER_ID", mUserID);
        url = replaceAll(url, "$STICKY", mSticky);
        url = replaceAll(url, "$POOL", mPool);
        url = replaceAll(url, "$SEQ", seq);

        mUpdateSession.SetUrl(cpr::Url(url));
        cpr::Response r = mUpdateSession.Get();

        if(r.status_code)
        {
            json j = responseToJson(r);

            if(j["seq"].is_number())
                seq = std::to_string(j["seq"].get<int>());

            j = j["ms"];

            for(int x = 0; x < j.size(); x++)
            {
                MessagingEvent event;

                if(j[x]["type"] == "typ")
                {
                    event.type = MessagingEvent::TYPING_STATUS;
                    event.typingStatus.from = j[x]["from"];
                    event.typingStatus.fromMobile = (!j[x]["from_mobile"].is_null() ? j[x]["from_mobile"].get<bool>() : false);
                    event.typingStatus.isTyping = j[x]["st"] == "true";
                }
                else if(j[x]["type"] == "buddylist_overlay")
                {
                    event.type = MessagingEvent::ONLINE_STATUS;
                    json::iterator it = j[x]["overlay"].begin();
                    std::string from =  it.key();
                    event.onlineStatus.from = std::stoll(from);
                    event.onlineStatus.timestamp = it.value()["la"];
//                    event.onlineStatus.status                = OnlineStatus::stringToStatus(it.value()["p"]["status"]);
//                    event.onlineStatus.onFacechatApplication = OnlineStatus::stringToStatus(it.value()["p"]["fbAppStatus"]);
//                    event.onlineStatus.onMessenger           = OnlineStatus::stringToStatus(it.value()["p"]["messengerStatus"]);
//                    event.onlineStatus.onWeb                 = OnlineStatus::stringToStatus(it.value()["p"]["webStatus"]);
//                    event.onlineStatus.onOther               = OnlineStatus::stringToStatus(it.value()["p"]["otherStatus"]);
                }
                else if((j[x]["type"] == "messaging" && j[x]["event"] == "deliver"))
                {
                    event.type = MessagingEvent::MESSAGE;
                    event.message = parseMessage(j[x]["message"]);
                }
                else if((j[x]["type"] == "messaging" && j[x]["event"] == "m_read_receipt"))
                {
                    event.type = MessagingEvent::READ_STATUS;
                    ReadStatus readStatus;
                    readStatus.from = j[x]["reader"];
                    event.readStatus = readStatus;
                }
                else
                {
//                    std::cout << j[x].dump(4) << std::endl;
//                    continue;
                }

//                std::cout << j[x].dump(4) << std::endl;

                mEventsMutex.lock();
                mEvents.push_front(event);
                mEventsMutex.unlock();
            }
        }
    }
}

void Facechat::defaultPayload(std::vector<cpr::Pair>& payloadsPairs)
{
    if(mDefaultPayloads.empty())
    {
//        mDefaultPayloads.push_back(cpr::Pair("client", "web_messenger"));

        mDefaultPayloads.push_back(cpr::Pair("client", "mercury"));
        mDefaultPayloads.push_back(cpr::Pair("__user", mUserID));
        mDefaultPayloads.push_back(cpr::Pair("__a", 1));
        mDefaultPayloads.push_back(cpr::Pair("__req", 3));
        mDefaultPayloads.push_back(cpr::Pair("fb_dtsg", mDTSG));

        std::string ttstamp = "";
        for(const char c : mDTSG)
            ttstamp += std::to_string((int)c);
        ttstamp += '2';

        mDefaultPayloads.push_back(cpr::Pair("ttstamp", ttstamp));
        mDefaultPayloads.push_back(cpr::Pair("__rev", mRevision));
    }

//    payloadsPairs.push_back({"seq", seq});

    payloadsPairs.insert(payloadsPairs.begin(), mDefaultPayloads.begin(), mDefaultPayloads.end());
}

json Facechat::responseToJson(cpr::Response& response)
{
    static const std::string toRemove = "for (;;);";

    if(response.text.size() >= toRemove.size())
        return json::parse(response.text.substr(toRemove.size()));

    return json();
}

std::string Facechat::generateOfflineThreadingID()
{
    std::random_device rd;
    std::default_random_engine engine(rd());
    std::uniform_int_distribution<long int> uniform_dist(0, 4294967295);

    return binaryStringToDecimalString(toBase(time(NULL), 2) + toBase(uniform_dist(engine), 2));
}

std::string Facechat::send(const std::vector<cpr::Pair>& data)
{
    std::vector<cpr::Pair> payloadsPairs;
    defaultPayload(payloadsPairs);

    payloadsPairs.push_back(cpr::Pair("message_batch[0][action_type]", "ma-type:user-generated-message"));
    payloadsPairs.push_back(cpr::Pair("message_batch[0][author]", "fbid:"+mUserID));
    payloadsPairs.push_back(cpr::Pair("message_batch[0][source]", "source:chat:web"));
    payloadsPairs.push_back(cpr::Pair("message_batch[0][source_tags][0]", "source:chat"));
    payloadsPairs.push_back(cpr::Pair("message_batch[0][ui_push_phase]", "V3"));
    payloadsPairs.push_back(cpr::Pair("message_batch[0][status]", "0"));
//    std::string threadingID = Facechat::generateOfflineThreadingID();
//    std::string threadingID = "6149052398431498505";
    std::string threadingID = "6150007224881098906";
//    std::string threadingID = "";
//    payloadsPairs.push_back(cpr::Pair("message_batch[0][message_id]", threadingID));
//    payloadsPairs.push_back(cpr::Pair("message_batch[0][offline_threading_id]", threadingID));

//    payloadsPairs.push_back(cpr::Pair("message_batch[0]", ""));
    payloadsPairs.push_back(cpr::Pair("message_batch[0][has_attachment]", true));
//    payloadsPairs.push_back(cpr::Pair("message_batch[0][thread_id]", ""));
    payloadsPairs.push_back(cpr::Pair("message_batch[0][timestamp]", "999"));
//    payloadsPairs.push_back(cpr::Pair("message_batch[0][timestamp_absolute]", "Aujourdhui"));
//    payloadsPairs.push_back(cpr::Pair("message_batch[0][timestamp_relative]", "14:53"));
    payloadsPairs.push_back(cpr::Pair("message_batch[0][timestamp_time_passed]", 0));
    payloadsPairs.push_back(cpr::Pair("message_batch[0][is_unread]", false));
    payloadsPairs.push_back(cpr::Pair("message_batch[0][is_forward]", false));
    payloadsPairs.push_back(cpr::Pair("message_batch[0][is_filtered_content]", false));
    payloadsPairs.push_back(cpr::Pair("message_batch[0][is_filtered_content_bh]", false));
    payloadsPairs.push_back(cpr::Pair("message_batch[0][is_filtered_content_account]", false));
    payloadsPairs.push_back(cpr::Pair("message_batch[0][is_filtered_content_quasar]", false));
    payloadsPairs.push_back(cpr::Pair("message_batch[0][is_filtered_content_invalid_app]", false));
    payloadsPairs.push_back(cpr::Pair("message_batch[0][is_spoof_warning]", false));
//    payloadsPairs.push_back(cpr::Pair("message_batch[0][html_body]", false));

    payloadsPairs.insert(payloadsPairs.end(), data.begin(), data.end());

    mPostSession.SetPayload(cpr::Payload {range_to_initializer_list(payloadsPairs.begin(), payloadsPairs.end())});
    mPostSession.SetUrl(cpr::Url {send_message_url});
//    mPostSession.SetUrl(cpr::Url {"http://www.httpbin.org/post"});
    cpr::Response r = mPostSession.Post();
    std::cout << r.text << std::endl;
    if(responseToJson(r)["payload"]["actions"][0]["message_id"].is_string())
        return responseToJson(r)["payload"]["actions"][0]["message_id"];
    else
        return "";
}

std::string Facechat::sendMessage(std::string message, UniversalID sendTo, bool isGroup, std::vector<cpr::Pair> datas)
{
    datas.emplace_back("message_batch[0][body]", message);

    if(!isGroup)
    {
        datas.push_back(cpr::Pair("message_batch[0][specific_to_list][1]", "fbid:"+mUserID));
        datas.push_back(cpr::Pair("message_batch[0][specific_to_list][0]", "fbid:"+std::to_string(sendTo)));
        datas.push_back(cpr::Pair("message_batch[0][other_user_fbid]", std::to_string(sendTo)));
    }
    else
        datas.push_back(cpr::Pair("message_batch[0][thread_fbid]", std::to_string(sendTo)));

    return send(datas);
}

std::string Facechat::sendAttachement(std::string message, std::string filePath, UniversalID sendTo, bool isGroup)
{
    std::vector<cpr::Pair> payloadsPairs;

    json j = uploadFile(filePath);

    std::string key;
    for(json::iterator it = j.begin(); it != j.end(); it++)
    {
        if(it.key().size() > 4 && it.key().substr(it.key().size() - 3) == "_id")
        {
            key = it.key();
            break;
        }
    }

    payloadsPairs.push_back(cpr::Pair("message_batch[0][has_attachment]", true));
    payloadsPairs.push_back(cpr::Pair("message_batch[0][" + key + "s][0]", std::to_string(j[key].get<long long int>())));

    return sendMessage(message, sendTo, isGroup, payloadsPairs);
}

std::string Facechat::sendUrl(std::string message, std::string url, UniversalID sendTo, bool isGroup)
{
    std::vector<cpr::Pair> payloadsPairs;
    payloadsPairs.push_back(cpr::Pair("message_batch[0][has_attachment]", true));
    payloadsPairs.push_back(cpr::Pair("message_batch[0][shareable_attachment][share_type]", 100));

    json j = json::parse(getFacechatURL(url));
    std::string base = "message_batch[0][shareable_attachment][share_params]";

    std::function<void(json&, std::string, std::vector<cpr::Pair>&)> eval;
    eval = [&eval](json& j, std::string base, std::vector<cpr::Pair>& pairs)
    {
        if(j.is_array())
        {
            int x = 0;
            for(json element : j)
                eval(element, base + "[" + std::to_string(x++) + "]", pairs);
        }
        else if(j.is_object())
        {
            for(json::iterator it = j.begin(); it != j.end(); it++)
                eval(it.value(), base + "[" + it.key() + "]", pairs);
        }
        else if(j.is_string())
            pairs.push_back(cpr::Pair(base, (std::string)j.get<std::string>()));
        else if(j.is_boolean())
            pairs.push_back(cpr::Pair(base, bool(j.get<bool>() ? true : false)));
    };

    eval(j, base, payloadsPairs);

    return sendMessage(message, sendTo, isGroup, payloadsPairs);
}

std::string Facechat::sendSticker(std::string stickerID, UniversalID sendTo, bool isGroup)
{
    std::vector<cpr::Pair> payloadsPairs;
    payloadsPairs.push_back(cpr::Pair("message_batch[0][has_attachment]", true));
    payloadsPairs.push_back(cpr::Pair("message_batch[0][sticker_id]", stickerID));
    return sendMessage("", sendTo, isGroup, payloadsPairs);
}

std::string Facechat::createGroup(std::string message, std::vector<UserID> otherUsers)
{
    std::vector<cpr::Pair> payloadsPairs;

    if(otherUsers.size() < 2)
        return "";

    for(int x = 1; x < otherUsers.size(); x++)
        payloadsPairs.push_back(cpr::Pair("message_batch[0][specific_to_list][" + std::to_string(x + 1) + "]", "fbid:"+std::to_string(otherUsers[x])));

    return sendMessage(message, otherUsers[0], false, payloadsPairs);
}

std::string Facechat::getFacechatURL(std::string url)
{
    std::vector<cpr::Pair> payloadsPairs;
    defaultPayload(payloadsPairs);

    payloadsPairs.push_back(cpr::Pair("image_height", 960));
    payloadsPairs.push_back(cpr::Pair("image_width", 960));
    payloadsPairs.push_back(cpr::Pair("uri", url));

    mPostSession.SetPayload(cpr::Payload {range_to_initializer_list(payloadsPairs.begin(), payloadsPairs.end())});
    mPostSession.SetUrl(cpr::Url {"https://www.facebook.com/message_share_attachment/fromURI/"});
    cpr::Response r = mPostSession.Post();
    return responseToJson(r)["payload"]["share_data"]["share_params"].dump();
}

json Facechat::uploadFile(std::string filePath)
{
    std::vector<cpr::Part> parts;
    std::vector<cpr::Pair> payloads;
    defaultPayload(payloads);

    for(cpr::Pair& pair : payloads)
        parts.push_back({pair.key, pair.value});

    parts.push_back(cpr::Part {"upload_1024", cpr::File(filePath)});

    mPostSession.SetMultipart(cpr::Multipart {range_to_initializer_list(parts.begin(), parts.end())});
    mPostSession.SetUrl("https://upload.facebook.com/ajax/mercury/upload.php");
    cpr::Response r = mPostSession.Post();
    return responseToJson(r)["payload"]["metadata"][0];
}

void Facechat::deleteMessage(std::string messageID)
{
    std::vector<cpr::Pair> payloadsPairs;
    defaultPayload(payloadsPairs);

    payloadsPairs.push_back(cpr::Pair("client", "mercury"));
    payloadsPairs.push_back(cpr::Pair("message_ids[0]", messageID));

    mPostSession.SetPayload(cpr::Payload {range_to_initializer_list(payloadsPairs.begin(), payloadsPairs.end())});
    mPostSession.SetUrl(cpr::Url {delete_message_url});
    mPostSession.Post();
}

void Facechat::markAsRead(UniversalID threadID)
{
    std::vector<cpr::Pair> payloadsPairs;
    defaultPayload(payloadsPairs);
    mPostSession.SetPayload(cpr::Payload {range_to_initializer_list(payloadsPairs.begin(), payloadsPairs.end())});
    mPostSession.SetUrl(cpr::Url {set_read_status_url + "?ids[" + std::to_string(threadID) + "]=true"});
    mPostSession.Post();
}

void Facechat::setTypingStatus(UniversalID threadID, bool typing, bool isGroup)
{
    std::vector<cpr::Pair> payloadsPairs;
    defaultPayload(payloadsPairs);

    payloadsPairs.push_back(cpr::Pair("source", "mercury-chat"));
    payloadsPairs.push_back(cpr::Pair("thread", std::to_string(threadID)));
    payloadsPairs.push_back(cpr::Pair("typ", (int)typing));
    payloadsPairs.push_back(cpr::Pair("to", (!isGroup ? std::to_string(threadID) : "")));

    mPostSession.SetPayload(cpr::Payload {range_to_initializer_list(payloadsPairs.begin(), payloadsPairs.end())});
    mPostSession.SetUrl(cpr::Url {set_typing_status_url});
    mPostSession.Post();
}

void Facechat::setGroupTitle(UniversalID id, std::string title)
{
    std::vector<cpr::Pair> payloadsPairs;
    defaultPayload(payloadsPairs);

    payloadsPairs.push_back(cpr::Pair("client", "mercury"));
    payloadsPairs.push_back(cpr::Pair("message_batch[0][action_type]", "ma-type:log-message"));
    payloadsPairs.push_back(cpr::Pair("message_batch[0][author]", "fbid:"+mUserID));
    payloadsPairs.push_back(cpr::Pair("message_batch[0][thread_id]", ""));
    payloadsPairs.push_back(cpr::Pair("message_batch[0][source]", "source:chat:web"));
    payloadsPairs.push_back(cpr::Pair("message_batch[0][source_tags][0]", "source:chat"));
    payloadsPairs.push_back(cpr::Pair("message_batch[0][log_message_type]", "log:thread-name"));
    payloadsPairs.push_back(cpr::Pair("message_batch[0][status]", "0"));

    payloadsPairs.push_back(cpr::Pair("message_batch[0][thread_fbid]", std::to_string(id)));

    payloadsPairs.push_back(cpr::Pair("message_batch[0][log_message_data][name]", title));

    payloadsPairs.push_back(cpr::Pair("message_batch[0][is_unread]", false));
    payloadsPairs.push_back(cpr::Pair("message_batch[0][is_cleared]", false));
    payloadsPairs.push_back(cpr::Pair("message_batch[0][is_forward]", false));
    payloadsPairs.push_back(cpr::Pair("message_batch[0][is_filtered_content]", false));
    payloadsPairs.push_back(cpr::Pair("message_batch[0][is_filtered_content_bh]", false));
    payloadsPairs.push_back(cpr::Pair("message_batch[0][is_filtered_content_account]", false));
    payloadsPairs.push_back(cpr::Pair("message_batch[0][is_spoof_warning]", false));
    payloadsPairs.push_back(cpr::Pair("message_batch[0][timestamp]", std::to_string(time(NULL))));

    payloadsPairs.push_back(cpr::Pair("message_batch[0][manual_retry_cnt]", "0"));
    payloadsPairs.push_back(cpr::Pair("message_batch[0][threading_id]", ""));

    std::string threadingID = Facechat::generateOfflineThreadingID();
    payloadsPairs.push_back(cpr::Pair("message_batch[0][message_id]", threadingID));
    payloadsPairs.push_back(cpr::Pair("message_batch[0][offline_threading_id]", threadingID));

    mPostSession.SetPayload(cpr::Payload {range_to_initializer_list(payloadsPairs.begin(), payloadsPairs.end())});
    mPostSession.SetUrl(cpr::Url {send_message_url});
    mPostSession.Post();
}

void Facechat::addUserToGroup(UserID userID, ThreadID group)
{
    std::vector<cpr::Pair> payloadsPairs;
    defaultPayload(payloadsPairs);

    payloadsPairs.push_back(cpr::Pair("client", "mercury"));
    payloadsPairs.push_back(cpr::Pair("message_batch[0][action_type]", "ma-type:log-message"));
    payloadsPairs.push_back(cpr::Pair("message_batch[0][author]", "fbid:"+mUserID));
    payloadsPairs.push_back(cpr::Pair("message_batch[0][thread_id]", ""));
    payloadsPairs.push_back(cpr::Pair("message_batch[0][source]", "source:chat:web"));
    payloadsPairs.push_back(cpr::Pair("message_batch[0][source_tags][0]", "source:chat"));
    payloadsPairs.push_back(cpr::Pair("message_batch[0][log_message_type]", "log:subscribe"));
    payloadsPairs.push_back(cpr::Pair("message_batch[0][status]", "0"));

    payloadsPairs.push_back(cpr::Pair("message_batch[0][thread_fbid]", std::to_string(group)));

    payloadsPairs.push_back(cpr::Pair("message_batch[0][log_message_data][added_participants][0]", "fbid:" + std::to_string(userID)));

    payloadsPairs.push_back(cpr::Pair("message_batch[0][is_unread]", false));
    payloadsPairs.push_back(cpr::Pair("message_batch[0][is_cleared]", false));
    payloadsPairs.push_back(cpr::Pair("message_batch[0][is_forward]", false));
    payloadsPairs.push_back(cpr::Pair("message_batch[0][is_filtered_content]", false));
    payloadsPairs.push_back(cpr::Pair("message_batch[0][is_filtered_content_bh]", false));
    payloadsPairs.push_back(cpr::Pair("message_batch[0][is_filtered_content_account]", false));
    payloadsPairs.push_back(cpr::Pair("message_batch[0][is_spoof_warning]", false));
    payloadsPairs.push_back(cpr::Pair("message_batch[0][timestamp]", std::to_string(time(NULL))));

    payloadsPairs.push_back(cpr::Pair("message_batch[0][manual_retry_cnt]", "0"));
    payloadsPairs.push_back(cpr::Pair("message_batch[0][threading_id]", ""));

    std::string threadingID = Facechat::generateOfflineThreadingID();
//    payloadsPairs.push_back(cpr::Pair("message_batch[0][message_id]", "6064083385041342768"));
    payloadsPairs.push_back(cpr::Pair("message_batch[0][message_id]", threadingID));
    payloadsPairs.push_back(cpr::Pair("message_batch[0][offline_threading_id]", threadingID));

    mPostSession.SetPayload(cpr::Payload {range_to_initializer_list(payloadsPairs.begin(), payloadsPairs.end())});
    mPostSession.SetUrl(cpr::Url {send_message_url});
    mPostSession.Post();
}

void Facechat::removeUserFromGroup(UserID userID, ThreadID group)
{
    std::vector<cpr::Pair> payloadsPairs;
    defaultPayload(payloadsPairs);

    payloadsPairs.push_back(cpr::Pair("uid", std::to_string(userID)));
    payloadsPairs.push_back(cpr::Pair("tid", std::to_string(group)));

    mPostSession.SetPayload(cpr::Payload {range_to_initializer_list(payloadsPairs.begin(), payloadsPairs.end())});
    mPostSession.SetUrl(cpr::Url {remove_user_from_group_url});
    mPostSession.Post();
}

std::vector<UserID> Facechat::getFriendList(UserID id)
{
    std::vector<cpr::Pair> payloadsPairs;
    defaultPayload(payloadsPairs);
    mPostSession.SetPayload(cpr::Payload {range_to_initializer_list(payloadsPairs.begin(), payloadsPairs.end())});
    mPostSession.SetUrl(cpr::Url {get_friends_list_part_1 + std::to_string(id)});
    cpr::Response r = mPostSession.Get();

    std::vector<UserID> friends;

    int pos = r.text.find("AllFriendsAppCollectionPagelet");
    if(pos == std::string::npos)
        pos = r.text.find("FriendsAppCollectionPagelet");
    if(pos == std::string::npos)
        return friends;

    r.text.erase(0, pos);
    r.text.erase(0, r.text.find("\"token\":\"") + 9);
    std::string token = r.text.substr(0, r.text.find("\""));

    pos = r.text.find("&quot;eng_tid&quot;:&quot;");
    r.text = r.text.substr(pos, r.text.rfind("&quot;eng_tid&quot;:&quot;") + 55 - pos);

    pos = 0;
    while((pos = r.text.find("&quot;eng_tid&quot;:&quot;", pos) + 26) != std::string::npos + 26)
        friends.push_back(std::stoll(r.text.substr(pos, r.text.find("&quot;", pos) - pos)));

    json data;
    data["collection_token"] = token;
    data["tab_key"] = "friends";
    data["sk"] = "friends";
    data["profile_id"] = id;
    data["overview"] = false;
    data["ftid"].clear();
    data["order"].clear();
    data["importer_state"].clear();

    payloadsPairs.push_back(cpr::Pair("", ""));
    mPostSession.SetUrl(cpr::Url {get_friends_list_part_2});

    while(true)
    {
        data["cursor"] = encodeBase64("0:not_structured:" + std::to_string(friends.back()));

        payloadsPairs.pop_back();
        payloadsPairs.push_back(cpr::Pair("data", data.dump()));
        mPostSession.SetPayload(cpr::Payload {range_to_initializer_list(payloadsPairs.begin(), payloadsPairs.end())});
        r = mPostSession.Post();

        int added = 0;
        json j = responseToJson(r)["jsmods"]["require"];
        for(auto element : j)
        {
            if(element[0] == "AddFriendButton")
                friends.push_back(element[3][1]), added++;
        }

        if(!added)
            break;
    }

    return friends;
}

std::vector<std::pair<UserID, time_t>> Facechat::getOnlineFriend(bool includeMobile)
{
    std::vector<cpr::Pair> payloadsPairs;
    defaultPayload(payloadsPairs);

    payloadsPairs.push_back(cpr::Pair("user", mUserID));
    payloadsPairs.push_back(cpr::Pair("fetch_mobile", (includeMobile ? true : false)));
    payloadsPairs.push_back(cpr::Pair("get_now_available_list", true));

    mPostSession.SetPayload(cpr::Payload {range_to_initializer_list(payloadsPairs.begin(), payloadsPairs.end())});
    mPostSession.SetUrl(cpr::Url {get_online_friends_url});
    cpr::Response r = mPostSession.Post();
    std::cout << r.text << std::endl;
    for(auto& c : r.cookies.map_)
        std::cout << c.first << ": " << c.second << std::endl;
    json j = responseToJson(r)["payload"]["buddy_list"];

    std::vector<std::pair<UserID, time_t>> onlines;

    for(json::iterator it = j["nowAvailableList"].begin(); it != j["nowAvailableList"].end(); it++)
    {
        UserID id = std::stoll(it.key());
        json jsonTimestamp = j["last_active_times"][std::to_string(id)];
        time_t timestamp = (jsonTimestamp.is_number() ? jsonTimestamp.get<time_t>() : 0);
        onlines.push_back(std::pair<UserID, time_t>(id, timestamp));
    }

    if(j["mobile_friends"].is_array())
    {
        for(UserID id : j["mobile_friends"])
        {
            json jsonTimestamp = j["last_active_times"][std::to_string(id)];
            time_t timestamp = (jsonTimestamp.is_number() ? jsonTimestamp.get<time_t>() : 0);
            onlines.push_back(std::pair<UserID, time_t>(id, timestamp));
        }
    }

    return onlines;
}

Facechat::UserInfo Facechat::getUserInfo(UserID id)
{
    std::vector<cpr::Pair> payloadsPairs;
    defaultPayload(payloadsPairs);

    payloadsPairs.push_back(cpr::Pair("ids[0]", std::to_string(id)));

    mPostSession.SetPayload(cpr::Payload {range_to_initializer_list(payloadsPairs.begin(), payloadsPairs.end())});
    mPostSession.SetUrl(cpr::Url {get_user_info});
    cpr::Response r = mPostSession.Post();
    json j = responseToJson(r)["payload"]["profiles"][std::to_string(id)];

    UserInfo info;
    info.completeName = j["name"].get<std::string>();
    info.firstName = j["firstName"].get<std::string>();
    info.gender = j["gender"];
    info.isFriend = j["is_friend"];
    info.id = std::stoll(j["id"].get<std::string>());
    info.profilePicture = j["thumbSrc"].get<std::string>();
    info.profileUrl = j["uri"].get<std::string>();
    info.vanity = j["vanity"].get<std::string>();
    return info;
}

std::vector<Facechat::UserSearchReturn> Facechat::findUser(std::string name)
{
    std::vector<cpr::Pair> payloadsPairs;
    defaultPayload(payloadsPairs);

    payloadsPairs.push_back(cpr::Pair("viewer", mUserID));
    payloadsPairs.push_back(cpr::Pair("rsp", "search"));
    payloadsPairs.push_back(cpr::Pair("context", "search"));
    payloadsPairs.push_back(cpr::Pair("path", "/home.php"));
    payloadsPairs.push_back(cpr::Pair("value", name));

    mPostSession.SetPayload(cpr::Payload {range_to_initializer_list(payloadsPairs.begin(), payloadsPairs.end())});
    mPostSession.SetUrl(cpr::Url {find_user_url});
    cpr::Response r = mPostSession.Post();
    json j = responseToJson(r)["payload"]["entries"];

    std::vector<UserSearchReturn> users;
    for(json element : j)
    {
        if(element["type"] != "user")
            continue;

        UserSearchReturn user;
        user.id = element["uid"];
        user.index = element["index_rank"];
        user.score = element["score"];
        user.name = element["names"][0].get<std::string>();
        user.profilePicture = element["photo"].get<std::string>();
        user.profileUrl = element["path"].get<std::string>();
        users.push_back(user);
    }

    return users;
}

Facechat::Thread Facechat::getThread(UniversalID id)
{
    int offset = 0;
    int end = 0;
    while(!end)
    {
        end = 1;
        for(Thread& thread : listThread(offset, 30))
        {
            end = 0;
            if(thread.threadID == id)
                return thread;

            offset++;
        }
    }
}

std::vector<Facechat::Thread> Facechat::listThread(int offset, int limit)
{
    std::vector<cpr::Pair> payloadsPairs;
    defaultPayload(payloadsPairs);
    payloadsPairs.push_back(cpr::Pair("inbox[offset]", offset));
    payloadsPairs.push_back(cpr::Pair("inbox[limit]", limit));

    mPostSession.SetPayload(cpr::Payload {range_to_initializer_list(payloadsPairs.begin(), payloadsPairs.end())});
    mPostSession.SetUrl(cpr::Url {list_thread_url});
    cpr::Response r = mPostSession.Post();

    std::vector<Facechat::Thread> threads;

    if(r.status_code)
    {
        json j = responseToJson(r)["payload"]["threads"];
        threads.reserve(j.size());

        for(json::iterator it = j.begin(); it != j.end(); ++it)
            threads.push_back(parseThread(*it));
    }

    return threads;
}

std::vector<Facechat::Thread> Facechat::findThread(std::string name, int offset, int limit)
{
    std::vector<cpr::Pair> payloadsPairs;
    defaultPayload(payloadsPairs);
    payloadsPairs.push_back(cpr::Pair("query", name));
    payloadsPairs.push_back(cpr::Pair("offset", (int)offset));
    payloadsPairs.push_back(cpr::Pair("limit", (int)limit));
    payloadsPairs.push_back(cpr::Pair("index", "fbid"));

    mPostSession.SetPayload(cpr::Payload {range_to_initializer_list(payloadsPairs.begin(), payloadsPairs.end())});
    mPostSession.SetUrl(cpr::Url {find_thread_url});
    cpr::Response r = mPostSession.Post();

    std::vector<Facechat::Thread> threads;

    if(r.status_code)
    {
        json j = responseToJson(r)["payload"]["mercury_payload"]["threads"];
        threads.reserve(j.size());

        for(json::iterator it = j.begin(); it != j.end(); ++it)
            threads.push_back(parseThread(*it));
    }

    return threads;
}


std::vector<Facechat::Message> Facechat::readThread(UniversalID id, int offset, int limit, time_t timestamp, bool isGroup)
{
    std::vector<cpr::Pair> payloadsPairs;
    defaultPayload(payloadsPairs);

    const std::string key = isGroup ? "thread_fbids" : "user_ids";

    payloadsPairs.push_back(cpr::Pair("messages[" + key + "][" + std::to_string(id) + "][offset]", (int)offset));
    payloadsPairs.push_back(cpr::Pair("messages[" + key + "][" + std::to_string(id) + "][limit]", (int)limit));
    payloadsPairs.push_back(cpr::Pair("messages[" + key + "][" + std::to_string(id) + "][timestamp]", std::to_string(timestamp)));
//    payloadsPairs.push_back(cpr::Pair("messages[" + key + "][" + std::to_string(id) + "][timestamp]", std::to_string(time(NULL) - 1000)));
//    payloadsPairs.push_back(cpr::Pair("messages[" + key + "][" + std::to_string(id) + "][timestamp]", ""));
//    payloadsPairs.push_back(cpr::Pair("messages[" + key + "][" + std::to_string(id) + "][timestamp]", 1466385476231));

    mPostSession.SetPayload(cpr::Payload {range_to_initializer_list(payloadsPairs.begin(), payloadsPairs.end())});

    mPostSession.SetUrl(cpr::Url {thread_info_url});
//    mPostSession.SetUrl(cpr::Url {test_url});

    cpr::Response r = mPostSession.Post();
//    std::cout << r.text << std::endl;

    std::vector<Facechat::Message> messages;

    if(r.status_code)
    {
        json j = responseToJson(r);
        j = j["payload"]["actions"];

        if(j.is_array())
        {
            for(json action : j)
            {
                if(action["action_type"].is_null())
                    break;
                    try
    {
                if(action["action_type"] == "ma-type:user-generated-message")
                    messages.push_back(parseMessage(action));
                    }
    catch(...)
    {}

//                std::cout << action.dump(4) << std::endl;
            }
        }
    }

    return messages;
}

void Facechat::deleteThread(UniversalID id)
{
    std::vector<cpr::Pair> payloadsPairs;
    defaultPayload(payloadsPairs);
    payloadsPairs.push_back(cpr::Pair("client", "mercury"));
    payloadsPairs.push_back(cpr::Pair("ids[0]", std::to_string(id)));

    mPostSession.SetPayload(cpr::Payload {range_to_initializer_list(payloadsPairs.begin(), payloadsPairs.end())});
    mPostSession.SetUrl(cpr::Url {delete_thread_url});
    cpr::Response r = mPostSession.Post();
    std::cout << r.status_code << r.text.size() << std::endl;

}

std::string Facechat::getUserID()
{
    return mUserID;
}

Facechat::Message Facechat::parseMessage(json& j)
{
    Facechat::Message message;

    message.body = (j["body"].is_string() ? j["body"].get<std::string>() : "");
    message.senderName = (j["sender_name"].is_string() ? j["sender_name"].get<std::string>() : "");
    message.messageID = (j["mid"].is_string() ? j["mid"].get<std::string>() : "");
    message.messageID = (j["message_id"].is_string() ? j["message_id"].get<std::string>() : message.messageID);
    message.to = (j["other_user_fbid"].is_number() ? j["other_user_fbid"].get<long long int>() : 0);

    message.timestamp = (j["timestamp"].is_number() ? j["timestamp"].get<time_t>() : 0);

    message.from = (j["sender_fbid"].is_number() ? j["sender_fbid"].get<long long int>() : 0);
    if(!message.from)
    {
        std::string id = (j["author"].is_string() ? j["author"].get<std::string>() : "");
        if(!id.empty() && (id.substr(0, 5) == "fbid:"))
            message.from = std::stoll(id.erase(0, 5));

        assert(message.from);
    }

    std::string threadID = (j["thread_fbid"].is_string() ? j["thread_fbid"].get<std::string>() : "");
    if(!threadID.empty())
    {
        if(threadID[0] == ':')
            threadID.erase(0, 1);

        message.conversationID = std::stoll(threadID);
    }

    if(j["group_thread_info"].is_object())
    {
        message.isGroup = true;
        message.group.groupParticipantCount = j["group_thread_info"]["participant_total_count"];

        for(const UserID& id : j["group_thread_info"]["participant_ids"])
            message.group.groupParticipantIDs.push_back(id);

        for(const std::string& name : j["group_thread_info"]["participant_names"])
            message.group.groupParticipantNames.push_back(name);
    }

    if(j["has_attachment"])
    {
        message.asAttachment = true;

        message.attachment.type = Attachment::stringToAttachmentType(j["attachments"][0]["attach_type"]);

        if(message.attachment.type == Attachment::STICKER)
        {
            message.attachment.stickerID = j["attachments"][0]["metadata"]["stickerID"];

            message.attachment.url = j["attachments"][0]["url"].get<std::string>();
            message.attachment.previewUrl = message.attachment.url;
        }
        else if(message.attachment.type == Attachment::PHOTO)
        {
            message.attachment.url = j["attachments"][0]["hires_url"].get<std::string>();
            message.attachment.previewUrl = j["attachments"][0]["preview_url"].get<std::string>();
        }
        else if(message.attachment.type == Attachment::VIDEO)
        {
            message.attachment.url = j["attachments"][0]["url"].get<std::string>();
            message.attachment.previewUrl = j["attachments"][0]["preview_url"].get<std::string>();
        }
        else
            message.asAttachment = false;
    }

    return message;
}

Facechat::Thread Facechat::parseThread(json& j)
{
    Thread thread;
    thread.messageCount = j["message_count"];
    thread.name = j["name"].get<std::string>();
    thread.threadID = std::stoll(j["thread_fbid"].get<std::string>());

    for(std::string id : j["participants"])
        thread.participants.push_back(std::stoll(id.substr(std::string("fbid:").size())));

    for(json id : j["former_participants"])
        thread.pastParticipant.push_back(std::stoll((id["id"]).get<std::string>().substr(std::string("fbid:").size())));

    return thread;
}
