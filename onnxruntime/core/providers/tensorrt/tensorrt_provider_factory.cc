// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "core/providers/shared_library/provider_api.h"
#include "core/providers/tensorrt/tensorrt_provider_factory.h"
#include <atomic>
#include "tensorrt_execution_provider.h"
#include "tensorrt_provider_factory_creator.h"
#include "core/framework/provider_options.h"
#include "core/providers/tensorrt/tensorrt_provider_options.h"
#include <string.h>

using namespace onnxruntime;

namespace onnxruntime {

void InitializeRegistry();
void DeleteRegistry();

struct TensorrtProviderFactory : IExecutionProviderFactory {
  TensorrtProviderFactory(const TensorrtExecutionProviderInfo& info) : info_{info} {}
  ~TensorrtProviderFactory() override {}

  std::unique_ptr<IExecutionProvider> CreateProvider() override;

 private:
  TensorrtExecutionProviderInfo info_;
};

std::unique_ptr<IExecutionProvider> TensorrtProviderFactory::CreateProvider() {
  return std::make_unique<TensorrtExecutionProvider>(info_);
}

std::shared_ptr<IExecutionProviderFactory> TensorrtProviderFactoryCreator::Create(int device_id) {
  TensorrtExecutionProviderInfo info;
  info.device_id = device_id;
  info.has_trt_options = false;
  return std::make_shared<onnxruntime::TensorrtProviderFactory>(info);
}

struct Tensorrt_Provider : Provider {
  std::shared_ptr<IExecutionProviderFactory> CreateExecutionProviderFactory(int device_id) override {
    TensorrtExecutionProviderInfo info;
    info.device_id = device_id;
    info.has_trt_options = false;
    return std::make_shared<TensorrtProviderFactory>(info);
  }

  std::shared_ptr<IExecutionProviderFactory> CreateExecutionProviderFactory(const void* provider_options) override {
    auto& options = *reinterpret_cast<const OrtTensorRTProviderOptionsV2*>(provider_options);
    TensorrtExecutionProviderInfo info;
    info.device_id = options.device_id;
    info.has_user_compute_stream = options.has_user_compute_stream != 0;
    info.user_compute_stream = options.user_compute_stream;
    info.has_trt_options = true;
    info.max_partition_iterations = options.trt_max_partition_iterations;
    info.min_subgraph_size = options.trt_min_subgraph_size;
    info.max_workspace_size = options.trt_max_workspace_size;
    info.fp16_enable = options.trt_fp16_enable != 0;
    info.int8_enable = options.trt_int8_enable != 0;
    info.int8_calibration_table_name = options.trt_int8_calibration_table_name == nullptr ? "" : options.trt_int8_calibration_table_name;
    info.int8_use_native_calibration_table = options.trt_int8_use_native_calibration_table != 0;
    info.dla_enable = options.trt_dla_enable != 0;
    info.dla_core = options.trt_dla_core;
    info.dump_subgraphs = options.trt_dump_subgraphs != 0;
    info.engine_cache_enable = options.trt_engine_cache_enable != 0;
    info.engine_cache_path = options.trt_engine_cache_path == nullptr ? "" : options.trt_engine_cache_path;
    info.engine_decryption_enable = options.trt_engine_decryption_enable != 0;
    info.engine_decryption_lib_path = options.trt_engine_decryption_lib_path == nullptr ? "" : options.trt_engine_decryption_lib_path;
    info.force_sequential_engine_build = options.trt_force_sequential_engine_build != 0;
    info.context_memory_sharing_enable = options.trt_context_memory_sharing_enable != 0;
    return std::make_shared<TensorrtProviderFactory>(info);
  }

  void UpdateProviderOptions(void* provider_options, const ProviderOptions& options) override {
    auto internal_options = onnxruntime::TensorrtExecutionProviderInfo::FromProviderOptions(options);
    auto& trt_options = *reinterpret_cast<OrtTensorRTProviderOptionsV2*>(provider_options);
    trt_options.device_id = internal_options.device_id;
    trt_options.trt_max_partition_iterations = internal_options.max_partition_iterations;
    trt_options.trt_min_subgraph_size = internal_options.min_subgraph_size;
    trt_options.trt_max_workspace_size = internal_options.max_workspace_size;
    trt_options.trt_fp16_enable = internal_options.fp16_enable;
    trt_options.trt_int8_enable = internal_options.int8_enable;

    char* dest = nullptr;
    auto str_size = internal_options.int8_calibration_table_name.size();
    if (str_size == 0) {
      trt_options.trt_int8_calibration_table_name = nullptr;
    } else {
      dest = new char[str_size + 1];
#ifdef _MSC_VER
      strncpy_s(dest, str_size + 1, internal_options.int8_calibration_table_name.c_str(), str_size);
#else
      strncpy(dest, internal_options.int8_calibration_table_name.c_str(), str_size);
#endif
      dest[str_size] = '\0';
      trt_options.trt_int8_calibration_table_name = (const char*)dest;
    }

    trt_options.trt_int8_use_native_calibration_table = internal_options.int8_use_native_calibration_table;
    trt_options.trt_dla_enable = internal_options.dla_enable;
    trt_options.trt_dla_core = internal_options.dla_core;
    trt_options.trt_dump_subgraphs = internal_options.dump_subgraphs;
    trt_options.trt_engine_cache_enable = internal_options.engine_cache_enable;

    str_size = internal_options.engine_cache_path.size();
    if (str_size == 0) {
      trt_options.trt_engine_cache_path = nullptr;
    } else {
      dest = new char[str_size + 1];
#ifdef _MSC_VER
      strncpy_s(dest, str_size + 1, internal_options.engine_cache_path.c_str(), str_size);
#else
      strncpy(dest, internal_options.engine_cache_path.c_str(), str_size);
#endif
      dest[str_size] = '\0';
      trt_options.trt_engine_cache_path = (const char*)dest;
    }

    trt_options.trt_engine_decryption_enable = internal_options.engine_decryption_enable;

    str_size = internal_options.engine_decryption_lib_path.size();
    if (str_size == 0) {
      trt_options.trt_engine_decryption_lib_path = nullptr;
    } else {
      dest = new char[str_size + 1];
#ifdef _MSC_VER
      strncpy_s(dest, str_size + 1, internal_options.engine_decryption_lib_path.c_str(), str_size);
#else
      strncpy(dest, internal_options.engine_decryption_lib_path.c_str(), str_size);
#endif
      dest[str_size] = '\0';
      trt_options.trt_engine_decryption_lib_path = (const char*)dest;
    }

    trt_options.trt_force_sequential_engine_build = internal_options.force_sequential_engine_build;
    trt_options.trt_context_memory_sharing_enable = internal_options.context_memory_sharing_enable;
  }

  ProviderOptions GetProviderOptions(const void* provider_options) override {
    auto& options = *reinterpret_cast<const OrtTensorRTProviderOptions*>(provider_options);
    return onnxruntime::TensorrtExecutionProviderInfo::ToProviderOptions(options);
  }

  void Initialize() override {
    InitializeRegistry();
  }

  void Shutdown() override {
    DeleteRegistry();
  }

} g_provider;

}  // namespace onnxruntime

extern "C" {

ORT_API(onnxruntime::Provider*, GetProvider) {
  return &onnxruntime::g_provider;
}
}
