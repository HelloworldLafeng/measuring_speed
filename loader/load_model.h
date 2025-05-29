#pragma once
#include <cstddef>
#include <string>

struct LoadStats {
    double t_meta_ms   = 0.0;  // 解析 GGUF + 构造 weights_map
    double t_mapping_ms= 0.0;  // mmap / madvise / mlock
    double t_load_ms   = 0.0;  // 真正把 tensor->data 准备好
    double t_total_ms  = 0.0;  // 整体
    std::size_t n_tensors = 0;
    std::size_t bytes_total = 0;
};

/// \brief 按原始框架流程加载模型并收集时序
/// \param gguf_path      模型文件主分片（.gguf 或 -00001-of-000xx.gguf）
/// \param use_mmap       true = 零拷贝 mmap；false = fread 到内存
/// \param check_tensors  是否逐 tensor 做校验（生产可关掉）
/// \param stats[out]     返回分阶段时间&大小
/// \return true=成功
bool load_model(const std::string& gguf_path,
                bool use_mmap,
                bool check_tensors,
                LoadStats& stats);
