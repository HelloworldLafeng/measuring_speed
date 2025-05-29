#include "load_model.h"

#include "../../llama.cpp/include/llama.h"
#include "../../llama.cpp/src/llama-impl.h"
#include "../../llama.cpp/src/llama-model-loader.h"

#include <chrono>
#include <iostream>
#include <unordered_map>
#include <vector>

using Clock = std::chrono::steady_clock;
static double ms(const Clock::time_point& t){
    return std::chrono::duration<double,std::milli>(Clock::now()-t).count();
}

static size_t meta_bytes(size_t n){ return n*300 + (1ull<<20); }

bool load_model(const std::string& path,bool use_mmap,bool check,LoadStats& st){
  try{
    const auto t_total0 = Clock::now();

    // A. 解析 GGUF ----------------------------------------------------------
    auto t0 = Clock::now();
    std::vector<std::string> splits;
    llama_model_loader L(path,splits,use_mmap,check,nullptr,nullptr);
    st.t_meta_ms = ms(t0);

    // B. mmap / prefetch ----------------------------------------------------
    t0 = Clock::now();
    L.init_mappings(/*prefetch=*/true,nullptr);
    st.t_mapping_ms = ms(t0);

    // C. 建 meta-only ggml_context -----------------------------------------
    size_t ctx_sz = meta_bytes(L.n_tensors);
    std::vector<uint8_t> ctx_buf(ctx_sz);
    ggml_context* ctx = ggml_init({ctx_sz,ctx_buf.data(),true});
    if(!ctx) throw std::runtime_error("ggml_init failed");

    for(auto& kv:L.weights_map){
        auto* t = kv.second.tensor;
        L.create_tensor(ctx,kv.first,{t->ne[0],t->ne[1],t->ne[2],t->ne[3]});
    }
    L.done_getting_tensors();

    // D. 数据加载 -----------------------------------------------------------
    t0 = Clock::now();

    std::unordered_map<uint32_t,ggml_backend_buffer_t> bufs;
    ggml_backend_t backend_cpu = nullptr;

    if(use_mmap){
        // ★ FIX ①：直接预设 data 指针 -> mmap 区；不建 backend buffer
        for(ggml_tensor* cur=ggml_get_first_tensor(ctx); cur;
            cur=ggml_get_next_tensor(ctx,cur)){
            L.load_data_for(cur);                 // 只设指针，不复制
        }
    }else{
        // 非 mmap：分配 host buffer + fread
        backend_cpu = ggml_backend_cpu_init();
        auto buft_cpu = ggml_backend_cpu_buffer_type();

        uint32_t max_idx=0;
        for(auto& kv:L.weights_map) max_idx=std::max(max_idx,uint32_t(kv.second.idx));
        for(uint32_t idx=0;idx<=max_idx;++idx){
            size_t sz = L.files.at(idx)->size();
            bufs[idx]=ggml_backend_buft_alloc_buffer(buft_cpu,sz);
        }
        L.load_all_data(ctx,bufs,nullptr,nullptr,nullptr);
    }

    st.t_load_ms = ms(t0);

    // E. 汇总 ---------------------------------------------------------------
    st.t_total_ms = ms(t_total0);
    st.n_tensors  = L.n_tensors;
    st.bytes_total= L.n_bytes;

    ggml_free(ctx);
    for(auto& kv:bufs) ggml_backend_buffer_free(kv.second);
    if(backend_cpu) ggml_backend_free(backend_cpu);
    return true;
  }
  catch(const std::exception& e){
    std::cerr<<"Exception: "<<e.what()<<"\n";
    return false;
  }
}
