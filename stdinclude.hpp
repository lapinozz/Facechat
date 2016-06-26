#pragma once

#include "extlib/cpr/include/cpr.h"
#include "extlib/cpr/include/util.h"

#include <iostream>
#include <map>
#include <vector>
#include <initializer_list>
#include <stdexcept>
#include <type_traits>
#include <fstream>
#include <deque>
#include <utility>
#include <cassert>
#include <chrono>
#include <ctime>
#include <random>
#include <cctype>
#include "Utility.hpp"

#include "json/json.hpp"

using namespace nlohmann;

#include "extlib/cpr/include/session.h"

#include <algorithm>
#include <functional>
#include <string>

#include <curl/curl.h>

#include "extlib/cpr/include/curlholder.h"
#include "extlib/cpr/include/util.h"

namespace cpr {

class Session::Impl {
  public:
    Impl();

    void SetUrl(const Url& url);
    void SetParameters(const Parameters& parameters);
    void SetParameters(Parameters&& parameters);
    void SetHeader(const Header& header);
    void SetTimeout(const Timeout& timeout);
    void SetAuth(const Authentication& auth);
    void SetDigest(const Digest& auth);
    void SetPayload(Payload&& payload);
    void SetPayload(const Payload& payload);
    void SetProxies(Proxies&& proxies);
    void SetProxies(const Proxies& proxies);
    void SetMultipart(Multipart&& multipart);
    void SetMultipart(const Multipart& multipart);
    void SetRedirect(const bool& redirect);
//    void SetMaxRedirects(const MaxRedirects& max_redirects);
    void SetCookies(const Cookies& cookies);
    void SetBody(Body&& body);
    void SetBody(const Body& body);

    Response Delete();
    Response Get();
    Response Head();
    Response Options();
    Response Patch();
    Response Post();
    Response Put();

  private:
    std::unique_ptr<CurlHolder, std::function<void(CurlHolder*)>> curl_;
    Url url_;
    Parameters parameters_;
    Proxies proxies_;

    Response makeRequest(CURL* curl);
    static void freeHolder(CurlHolder* holder);
    static CurlHolder* newHolder();
};
}
