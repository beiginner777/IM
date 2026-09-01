#include "SensitiveFilter.h"
#include "ConfigManager.h"
#include <fstream>
#include <queue>
#include <algorithm>
#include <iostream>

SensitiveFilter::SensitiveFilter()
{
    // 从 config 读词表路径（默认 sensitive_words.txt，相对进程运行目录 linux-server/）
    std::string wordFile = ConfigManager::getInstance()["Sensitive"]["WordFile"];
    if (wordFile.empty()) {
        wordFile = "sensitive_words.txt";
    }
    root_ = std::make_unique<AhoNode>();
    if (!loadFromFile(wordFile)) {
        std::cerr << "[SensitiveFilter] load word file failed: " << wordFile
                  << ", sensitive filter disabled." << std::endl;
        return;
    }
    buildFailPointers();
    built_ = true;
    std::cout << "[SensitiveFilter] loaded word file: " << wordFile << std::endl;
}

bool SensitiveFilter::loadFromFile(const std::string& path)
{
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        return false;
    }
    std::string line;
    int hardCount = 0;
    int softCount = 0;
    while (std::getline(ifs, line)) {
        // 去掉 Windows 换行的 \r
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        // 去首尾空白
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) continue;
        size_t end = line.find_last_not_of(" \t");
        line = line.substr(start, end - start + 1);
        // 空行 / 注释
        if (line.empty() || line[0] == '#') continue;

        bool is_hard = false;
        std::string word;
        if (line.rfind("hard:", 0) == 0) {
            is_hard = true;
            word = line.substr(5);
        } else if (line.rfind("soft:", 0) == 0) {
            is_hard = false;
            word = line.substr(5);
        } else {
            word = line;  // 无前缀默认软违规
        }
        if (word.empty()) continue;

        // 插入 Trie（按 UTF-8 字节序列）
        AhoNode* p = root_.get();
        for (unsigned char c : word) {
            if (!p->next.count(c)) {
                p->next[c] = std::make_unique<AhoNode>();
            }
            p = p->next[c].get();
        }
        p->is_end = true;
        p->is_hard = is_hard;
        p->word_len = static_cast<int>(word.size());
        p->word = word;
        if (is_hard) hardCount++; else softCount++;
    }
    if (hardCount == 0 && softCount == 0) {
        return false;  // 空词表视为加载失败
    }
    std::cout << "[SensitiveFilter] " << hardCount << " hard words, "
              << softCount << " soft words." << std::endl;
    return true;
}

void SensitiveFilter::buildFailPointers()
{
    // BFS 构建 fail 指针：root 的直接子节点 fail 指向 root
    std::queue<AhoNode*> q;
    root_->fail = root_.get();
    for (auto& kv : root_->next) {
        kv.second->fail = root_.get();
        q.push(kv.second.get());
    }
    while (!q.empty()) {
        AhoNode* cur = q.front();
        q.pop();
        for (auto& kv : cur->next) {
            unsigned char c = kv.first;
            AhoNode* child = kv.second.get();
            AhoNode* f = cur->fail;
            while (f != root_.get() && !f->next.count(c)) {
                f = f->fail;
            }
            if (f->next.count(c)) {
                child->fail = f->next[c].get();
            } else {
                child->fail = root_.get();
            }
            q.push(child);
        }
    }
}

SensitiveFilterResult SensitiveFilter::filter(const std::string& content) const
{
    SensitiveFilterResult result;
    result.filtered_content = content;
    if (!built_ || content.empty()) {
        return result;
    }

    struct Match { int start; int end; bool is_hard; std::string word; };
    std::vector<Match> matches;

    AhoNode* state = root_.get();
    for (size_t i = 0; i < content.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(content[i]);
        while (state != root_.get() && !state->next.count(c)) {
            state = state->fail;
        }
        if (state->next.count(c)) {
            state = state->next[c].get();
        }
        // 沿 fail 链收集所有以当前位置结尾的模式串
        for (AhoNode* t = state; t != root_.get(); t = t->fail) {
            if (t->is_end) {
                int end = static_cast<int>(i);
                int start = end - t->word_len + 1;
                matches.push_back({start, end, t->is_hard, t->word});
            }
        }
    }

    if (matches.empty()) {
        return result;
    }

    for (auto& m : matches) {
        if (m.is_hard) {
            result.hard_violation = true;
        }
        result.hit_words.push_back(m.word);
    }
    // 去重 hit_words（保留顺序）
    std::sort(result.hit_words.begin(), result.hit_words.end());
    result.hit_words.erase(std::unique(result.hit_words.begin(), result.hit_words.end()),
                           result.hit_words.end());

    // 软违规词范围标记为掩码；硬违规不掩码（整条消息会被拦截，不落库不转发）
    std::vector<char> masked(content.size(), 0);
    for (auto& m : matches) {
        if (!m.is_hard) {
            for (int j = m.start; j <= m.end; ++j) {
                masked[j] = 1;
            }
        }
    }

    std::string out;
    out.reserve(content.size() + 8);
    for (size_t i = 0; i < content.size(); ++i) {
        if (masked[i]) {
            out += "***";
            while (i + 1 < content.size() && masked[i + 1]) {
                ++i;  // 跳过连续掩码段，整段合并为一个 ***
            }
        } else {
            out += content[i];
        }
    }
    result.filtered_content = out;
    return result;
}
