// 공용 QoS/동작 옵션 파서 — publisher/subscriber 공통 사용
#pragma once

#include <fastdds/dds/core/policy/QosPolicies.hpp>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

using namespace eprosima::fastdds::dds;

struct QosArgs
{
    // 공통
    int domain = 0;
    std::string topic = "HelloWorldTopic";
    std::string name = "";

    // QoS
    ReliabilityQosPolicyKind reliability = RELIABLE_RELIABILITY_QOS;
    DurabilityQosPolicyKind durability = VOLATILE_DURABILITY_QOS;
    HistoryQosPolicyKind history = KEEP_LAST_HISTORY_QOS;
    int depth = 10;

    // Publisher 동작
    int samples = 20;       // 보낼 개수
    int interval_ms = 250;  // 송신 간격(ms)
    int keep_alive = 2;     // 송신 후 유지 시간(초). 늦은 Subscriber 테스트(3-2/3-3)는 크게.

    // Subscriber 동작
    int timeout = 300;      // 최대 대기(초)
};

inline const char* rel_str(ReliabilityQosPolicyKind k)
{
    return (k == RELIABLE_RELIABILITY_QOS) ? "RELIABLE" : "BEST_EFFORT";
}
inline const char* dur_str(DurabilityQosPolicyKind k)
{
    return (k == TRANSIENT_LOCAL_DURABILITY_QOS) ? "TRANSIENT_LOCAL" : "VOLATILE";
}
inline const char* his_str(HistoryQosPolicyKind k)
{
    return (k == KEEP_ALL_HISTORY_QOS) ? "KEEP_ALL" : "KEEP_LAST";
}

inline void print_qos(const char* role, const QosArgs& a)
{
    std::cout << "[" << role << "] QoS: domain=" << a.domain
              << " topic=" << a.topic
              << " reliability=" << rel_str(a.reliability)
              << " durability=" << dur_str(a.durability)
              << " history=" << his_str(a.history) << "(depth=" << a.depth << ")";
    if (!a.name.empty()) std::cout << " name=" << a.name;
    std::cout << std::endl;
}

inline void print_help(const char* prog)
{
    std::cout <<
        "사용법: " << prog << " [옵션]\n"
        "  --domain N            도메인 ID (기본 0)\n"
        "  --topic NAME          토픽 이름 (기본 HelloWorldTopic)\n"
        "  --reliability K       reliable | best_effort (기본 reliable)\n"
        "  --durability K        volatile | transient_local (기본 volatile)\n"
        "  --history K           keep_last | keep_all (기본 keep_last)\n"
        "  --depth N             KEEP_LAST 깊이 (기본 10)\n"
        "  --samples N           [Pub] 송신 개수 (기본 20)\n"
        "  --interval MS         [Pub] 송신 간격 ms (기본 250)\n"
        "  --keep-alive SEC      [Pub] 송신 후 유지 시간 초 (기본 2)\n"
        "  --timeout SEC         [Sub] 최대 대기 초 (기본 300)\n"
        "  --name STR            로그 식별용 이름\n"
        "  --help                도움말\n";
}

// 반환: true=계속, false=종료(도움말 등)
inline bool parse_args(int argc, char** argv, QosArgs& a)
{
    auto need = [&](int& i) -> const char* {
        if (i + 1 >= argc) { std::cerr << "옵션 값 누락: " << argv[i] << std::endl; std::exit(1); }
        return argv[++i];
    };
    for (int i = 1; i < argc; ++i)
    {
        std::string o = argv[i];
        if (o == "--help" || o == "-h") { print_help(argv[0]); return false; }
        else if (o == "--domain") a.domain = std::atoi(need(i));
        else if (o == "--topic") a.topic = need(i);
        else if (o == "--name") a.name = need(i);
        else if (o == "--depth") a.depth = std::atoi(need(i));
        else if (o == "--samples") a.samples = std::atoi(need(i));
        else if (o == "--interval") a.interval_ms = std::atoi(need(i));
        else if (o == "--keep-alive") a.keep_alive = std::atoi(need(i));
        else if (o == "--timeout") a.timeout = std::atoi(need(i));
        else if (o == "--reliability")
        {
            std::string v = need(i);
            if (v == "reliable" || v == "re") a.reliability = RELIABLE_RELIABILITY_QOS;
            else if (v == "best_effort" || v == "best-effort" || v == "be") a.reliability = BEST_EFFORT_RELIABILITY_QOS;
            else { std::cerr << "알 수 없는 reliability: " << v << std::endl; std::exit(1); }
        }
        else if (o == "--durability")
        {
            std::string v = need(i);
            if (v == "volatile" || v == "v") a.durability = VOLATILE_DURABILITY_QOS;
            else if (v == "transient_local" || v == "transient" || v == "tl") a.durability = TRANSIENT_LOCAL_DURABILITY_QOS;
            else { std::cerr << "알 수 없는 durability: " << v << std::endl; std::exit(1); }
        }
        else if (o == "--history")
        {
            std::string v = need(i);
            if (v == "keep_last" || v == "keeplast" || v == "kl") a.history = KEEP_LAST_HISTORY_QOS;
            else if (v == "keep_all" || v == "keepall" || v == "ka") a.history = KEEP_ALL_HISTORY_QOS;
            else { std::cerr << "알 수 없는 history: " << v << std::endl; std::exit(1); }
        }
        else { std::cerr << "알 수 없는 옵션: " << o << " (--help 참고)" << std::endl; std::exit(1); }
    }
    return true;
}
