#ifndef SENSITIVE_FILTER_H
#define SENSITIVE_FILTER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include "SingleTon.h"

// AC 自动机敏感词过滤器：Trie + fail 指针 + 线性匹配
// 词表文件格式：每行一个词，`hard:` 前缀=硬违规（拦截），`soft:` 前缀=软违规（替换为 ***），`#` 开头为注释
// 词表在构造时一次性加载，加载后不可变；filter() 为 const 方法，多线程只读共享（无锁）
struct SensitiveFilterResult {
    bool hard_violation = false;              // 是否命中硬违规词
    std::string filtered_content;             // 过滤后内容（软违规替换为 ***）
    std::vector<std::string> hit_words;       // 命中的敏感词（去重后，用于日志/审计）
};

class SensitiveFilter : public SingleTon<SensitiveFilter>
{
    friend class SingleTon<SensitiveFilter>;
public:
    ~SensitiveFilter() = default;
    // 对文本做敏感词过滤（const，多线程安全）
    SensitiveFilterResult filter(const std::string& content) const;
private:
    SensitiveFilter();                        // 构造时从词表文件加载 + 构建 fail 指针
    bool loadFromFile(const std::string& path);
    void buildFailPointers();

    struct AhoNode {
        std::unordered_map<unsigned char, std::unique_ptr<AhoNode>> next;
        AhoNode* fail = nullptr;
        bool is_end = false;
        bool is_hard = false;                 // 硬违规词（命中则整条拦截）
        int word_len = 0;                     // 词长（字节数）
        std::string word;                     // 命中的词本身（日志/审计用）
    };
    std::unique_ptr<AhoNode> root_;
    bool built_ = false;
};
#endif
