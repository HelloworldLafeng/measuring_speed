#include "loader/load_model.h"
#include <iostream>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " /path/to/model.gguf "
                     "[--no-mmap] [--no-check]\n";
        return 1;
    }

    const std::string model_path = argv[1];
    bool use_mmap      = true;
    bool check_tensors = true;

    for (int i = 2; i < argc; ++i) {
        std::string flag = argv[i];
        if (flag == "--no-mmap")  use_mmap      = false;
        if (flag == "--no-check") check_tensors = false;
    }

    LoadStats st;
    if (!load_model(model_path, use_mmap, check_tensors, st)) {
        return 2;
    }

    std::cout << "\n==== Load summary ====\n"
              << " tensors   : " << st.n_tensors << "\n"
              << " total MB  : " << st.bytes_total / (1024.0 * 1024.0) << "\n"
              << " meta(ms)  : " << st.t_meta_ms    << "\n"
              << " mmap(ms)  : " << st.t_mapping_ms << "\n"
              << " data(ms)  : " << st.t_load_ms    << "\n"
              << " total(ms) : " << st.t_total_ms   << "\n";
    return 0;
}
