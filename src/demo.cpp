#include <regex>
#include <chrono>
#include <random>

#ifdef _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

#include <cqcppsdk/cqcppsdk.h>

using namespace cq;
using namespace std;
using Message = cq::message::Message;
using MessageSegment = cq::message::MessageSegment;

inline unsigned long long GetCycleCount() {
    return __rdtsc();
}

int diceHandler(int min, int max) {
    std::mt19937 gen(static_cast<unsigned int>(GetCycleCount()));
    std::uniform_int_distribution<int> dis(min, max);
    return dis(gen);
}

void rootHandler(const MessageEvent &event, string nickname) {
    string reqMsg = event.message;
    string resMsg = "";
    regex callRe("^上海人形$");
    regex diceRe("^(\\d+)d(\\d+)$", wregex::icase);
    regex timerRe("^timer(\\d+)$", wregex::icase);
    smatch match;
    if (regex_match(reqMsg, callRe)) {
        resMsg += "我在这里";
        const int index = diceHandler(1, 3);
        switch (index) {
        case 1:
            resMsg += "!";
            break;
        case 2:
            resMsg += "~";
            break;
        case 3:
            resMsg += "...";
            break;
        default:
            break;
        }
    } else if (regex_search(reqMsg, match, diceRe) && match.size() == 3) {
        const int num = stoi(match[1]);
        const int max = stoi(match[2]);
        if (num > 0 && max > 0 && num < 100 && max < 10000) {
            resMsg += nickname + "投掷:" + reqMsg + " 结果:";
            for (int i = 0; i < num; i++) {
                const int ret = diceHandler(1, max);
                resMsg += to_string(ret) + " ";
            }
        } else {
            resMsg += "骰子炸了";
        }
    } else if (regex_search(reqMsg, match, timerRe)) {
        const int minutes = stoi(match[1]);
        resMsg += "倒计时:" + to_string(minutes) + "分钟";
    }

    if (!resMsg.empty()) {
        send_message(event.target, resMsg);
    }
}

CQ_INIT {
    on_enable([] { logging::info("启用", "插件已启用"); });

    on_private_message([](const PrivateMessageEvent &event) {
        try {
            rootHandler(event, "");
        } catch (ApiError &err) {
            logging::warning("Private message", "Error code: " + to_string(err.code));
        }
    });

    on_message([](const MessageEvent &event) {
        logging::debug("消息", "收到消息: " + event.message + "\n实际类型: " + typeid(event).name());
    });

    on_group_message([](const GroupMessageEvent &event) {
        try {
            string nickname = get_group_member_info(event.group_id, event.user_id).nickname;
            rootHandler(event, nickname);
        } catch (ApiError &err) {
            logging::warning("Group message", "Error code: " + to_string(err.code));
        }
        event.block(); // 阻止当前事件传递到下一个插件
    });

    on_discuss_message([](const DiscussMessageEvent &event) {
        try {
            string nickname = get_group_member_info(event.discuss_id, event.user_id).nickname;
            rootHandler(event, nickname);
        } catch (ApiError &err) {
            logging::warning("Discuss message", "Error code: " + to_string(err.code));
        }
        event.block(); // 阻止当前事件传递到下一个插件
    });
}

CQ_MENU(menu_demo_1) {
    logging::info("菜单", "点击菜单1");
}

CQ_MENU(menu_demo_2) {
    send_private_message(10000, "测试");
}
