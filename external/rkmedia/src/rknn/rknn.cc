// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "buffer.h"
#include "filter.h"

#include <rknn/rknn_runtime.h>

namespace easymedia {

class RKNNContext {
public:
  RKNNContext(const rknn_context rc) : ctx(rc) {}
  ~RKNNContext() { rknn_destroy(ctx); }
  const rknn_context &GetCtx() { return ctx; }

private:
  rknn_context ctx;
};

class RKNNFilter : public Filter {
public:
  RKNNFilter(const char *param);
  virtual ~RKNNFilter() = default;
  static const char *GetFilterName() { return "rknn"; }
  virtual int Process(std::shared_ptr<MediaBuffer> input,
                      std::shared_ptr<MediaBuffer> &output) override;

private:
  rknn_context ctx;
  std::shared_ptr<RKNNContext> rknn_ctx;
  rknn_input_output_num io_num;
  rknn_tensor_type tensor_type;
  rknn_tensor_format tensor_fmt;
  std::vector<bool> output_want_float;
};

static rknn_tensor_type GetTensorTypeByString(const std::string &type) {
  static std::map<std::string, rknn_tensor_type> tensor_type_map = {
      {NN_FLOAT32, RKNN_TENSOR_FLOAT32},
      {NN_FLOAT16, RKNN_TENSOR_FLOAT16},
      {NN_INT8, RKNN_TENSOR_INT8},
      {NN_UINT8, RKNN_TENSOR_UINT8},
      {NN_INT16, RKNN_TENSOR_INT16}};
  auto it = tensor_type_map.find(type);
  return (it != tensor_type_map.end()) ? it->second : (rknn_tensor_type)-1;
}

static rknn_tensor_format GetTensorFmtByString(const std::string &fmt) {
  static std::map<std::string, rknn_tensor_format> tensor_fmt_map = {
      {KEY_NCHW, RKNN_TENSOR_NCHW}, {KEY_NHWC, RKNN_TENSOR_NHWC}};
  auto it = tensor_fmt_map.find(fmt);
  return (it != tensor_fmt_map.end()) ? it->second : (rknn_tensor_format)-1;
}

static uint8_t *load_model(const char *filename, int *model_size) {
  FILE *fp = fopen(filename, "rb");
  if (!fp) {
    RKMEDIA_LOGI("Fail to fopen %s\n", filename);
    return nullptr;
  }
  fseek(fp, 0, SEEK_END);
  long model_len = ftell(fp);
  if (model_len <= 0) {
    RKMEDIA_LOGI("Fail to ftell %s\n", filename);
    fclose(fp);
    return nullptr;
  }
  // there is a buffer overflow buggy on librknn_runtime.so, malloc more
  uint8_t *model = (uint8_t *)malloc(model_len + 1024);
  if (model) {
    fseek(fp, 0, SEEK_SET);
    if (model_len != (long)fread(model, 1, model_len, fp)) {
      RKMEDIA_LOGI("Fail to fread %s\n", filename);
      free(model);
      fclose(fp);
      return nullptr;
    }
    *model_size = model_len;
  }
  fclose(fp);
  return model;
}

static void print_rknn_tensor(rknn_tensor_attr *attr) {
  RKMEDIA_LOGI(
      "index=%d name=%s n_dims=%d dims=[%d %d %d %d] n_elems=%d size=%d fmt=%d "
      "type=%d qnt_type=%d fl=%d zp=%d scale=%f\n",
      attr->index, attr->name, attr->n_dims, attr->dims[3], attr->dims[2],
      attr->dims[1], attr->dims[0], attr->n_elems, attr->size, 0, attr->type,
      attr->qnt_type, attr->fl, attr->zp, attr->scale);
}

static int print_rknn_attrs(const rknn_context &ctx,
                            const rknn_input_output_num &io_num) {
  RKMEDIA_LOGI("input tensors:\n");
  rknn_tensor_attr input_attrs[io_num.n_input];
  memset(input_attrs, 0, sizeof(input_attrs));
  for (uint32_t i = 0; i < io_num.n_input; i++) {
    input_attrs[i].index = i;
    int ret = rknn_query(ctx, RKNN_QUERY_INPUT_ATTR, &(input_attrs[i]),
                         sizeof(rknn_tensor_attr));
    if (ret != RKNN_SUCC) {
      RKMEDIA_LOGI("Fail to rknn_query INPUT_ATTR fail, ret=%d\n", ret);
      return ret;
    }
    print_rknn_tensor(&(input_attrs[i]));
  }
  RKMEDIA_LOGI("output tensors:\n");
  rknn_tensor_attr output_attrs[io_num.n_output];
  memset(output_attrs, 0, sizeof(output_attrs));
  for (uint32_t i = 0; i < io_num.n_output; i++) {
    output_attrs[i].index = i;
    int ret = rknn_query(ctx, RKNN_QUERY_OUTPUT_ATTR, &(output_attrs[i]),
                         sizeof(rknn_tensor_attr));
    if (ret != RKNN_SUCC) {
      RKMEDIA_LOGI("Fail to rknn_query OUTPUT_ATTR fail, ret=%d\n", ret);
      return ret;
    }
    print_rknn_tensor(&(output_attrs[i]));
  }
  return RKNN_SUCC;
}

RKNNFilter::RKNNFilter(const char *param) {
  memset(&io_num, 0, sizeof(io_num));
  std::map<std::string, std::string> params;
  std::list<std::pair<const std::string, std::string &>> req_list;
  std::string model_path;
  std::string str_tensor_type;
  std::string str_tensor_fmt;
  std::string str_output_want_float;
  req_list.push_back(
      std::pair<const std::string, std::string &>(KEY_PATH, model_path));
  req_list.push_back(std::pair<const std::string, std::string &>(
      KEY_TENSOR_TYPE, str_tensor_type));
  req_list.push_back(std::pair<const std::string, std::string &>(
      KEY_TENSOR_FMT, str_tensor_fmt));
  req_list.push_back(std::pair<const std::string, std::string &>(
      KEY_OUTPUT_WANT_FLOAT, str_output_want_float));
  int ret = parse_media_param_match(param, params, req_list);
  if (ret <= 0) {
    SetError(-EINVAL);
    return;
  }
  tensor_type = GetTensorTypeByString(str_tensor_type);
  tensor_fmt = GetTensorFmtByString(str_tensor_fmt);
  if (tensor_type < 0 || tensor_fmt < 0) {
    RKMEDIA_LOGI("incorrect tensor type or tensor fmt\n");
    SetError(-EINVAL);
    return;
  }
  int model_size = 0;
  uint8_t *model = load_model(model_path.c_str(), &model_size);
  if (!model) {
    RKMEDIA_LOGI("Fail to load model\n");
    SetError(-EINVAL);
    return;
  }
  ret = rknn_init(&ctx, model, model_size, 0);
  if (ret < 0) {
    RKMEDIA_LOGI("Fail to rknn_init, ret=%d\n", ret);
    goto err;
  }
  rknn_ctx = std::make_shared<RKNNContext>(ctx);
  if (!rknn_ctx) {
    rknn_destroy(ctx);
    goto err;
  }
  ret = rknn_query(ctx, RKNN_QUERY_IN_OUT_NUM, &io_num, sizeof(io_num));
  if (ret != RKNN_SUCC) {
    RKMEDIA_LOGI("Fail to rknn_query IN_OUT_NUM fail, ret=%d\n", ret);
    goto err;
  }
  RKMEDIA_LOGI("model input num: %d, output num: %d\n", io_num.n_input,
               io_num.n_output);
  ret = print_rknn_attrs(ctx, io_num);
  free(model);
  if (!str_output_want_float.empty()) {
    std::list<std::string> value_list;
    if (!parse_media_param_list(str_output_want_float.c_str(), value_list,
                                ',')) {
      SetError(-EINVAL);
      return;
    }
    if (value_list.size() != io_num.n_output) {
      RKMEDIA_LOGW("input want floats [%d] not match model n_output [%d]\n",
                   (int)value_list.size(), (int)io_num.n_output);
    }
    for (auto &s : value_list)
      output_want_float.push_back(!!std::stoi(s));
  }
  return;

err:
  SetError(-EINVAL);
  free(model);
}

class RKNNOutPut {
public:
  RKNNOutPut(const std::shared_ptr<RKNNContext> &ctx, uint32_t num)
      : rknn_ctx(ctx), n_output(num) {
    outputs = (rknn_output *)calloc(1, num * sizeof(rknn_output));
  }
  ~RKNNOutPut() {
    if (outputs)
      rknn_outputs_release(rknn_ctx->GetCtx(), n_output, outputs);
  }
  rknn_output *GetOutputs() { return outputs; }

private:
  std::shared_ptr<RKNNContext> rknn_ctx;
  rknn_output *outputs;
  uint32_t n_output;
};

static int __free_rknnoutputs(void *arg) {
  if (arg)
    delete static_cast<RKNNOutPut *>(arg);
  return 0;
}

int RKNNFilter::Process(std::shared_ptr<MediaBuffer> input,
                        std::shared_ptr<MediaBuffer> &output) {
  if (!input || input->GetType() != Type::Image || !input->IsValid())
    return -EINVAL;
  if (!output)
    return -EINVAL;
  rknn_input inputs[1];
  memset(inputs, 0, sizeof(inputs));
  inputs[0].index = 0;
  inputs[0].type = tensor_type;
  inputs[0].size = input->GetValidSize();
  inputs[0].fmt = tensor_fmt;
  inputs[0].buf = input->GetPtr();
  if (io_num.n_input != 1) {
    LOG_TODO();
  }
  int ret = rknn_inputs_set(ctx, io_num.n_input, inputs);
  if (ret < 0) {
    RKMEDIA_LOGI("Fail to rknn_input_set, ret=%d\n", ret);
    return -1;
  }
  ret = rknn_run(ctx, nullptr);
  if (ret < 0) {
    RKMEDIA_LOGI("Fail to rknn_run, ret=%d\n", ret);
    return -1;
  }
  rknn_output *outputs = nullptr;
  RKNNOutPut *out = new RKNNOutPut(rknn_ctx, io_num.n_output);
  if (!out || !(outputs = out->GetOutputs())) {
    LOG_NO_MEMORY();
    __free_rknnoutputs(out);
    return -1;
  }
  for (uint32_t i = 0; i < output_want_float.size(); i++)
    outputs[i].want_float = output_want_float[i] ? 1 : 0;
  ret = rknn_outputs_get(ctx, io_num.n_output, outputs, NULL);
  if (ret < 0) {
    RKMEDIA_LOGI("Fail to rknn_outputs_get, ret=%d\n", ret);
    free(outputs);
    return -1;
  }
  output->SetPtr(outputs);
  output->SetSize(io_num.n_output * sizeof(rknn_output));
  output->SetUserData(out, __free_rknnoutputs);
  output->SetValidSize(io_num.n_output);
  output->SetUSTimeStamp(input->GetUSTimeStamp());
  return 0;
}

DEFINE_COMMON_FILTER_FACTORY(RKNNFilter)
const char *FACTORY(RKNNFilter)::ExpectedInputDataType() {
  return TYPE_ANYTHING;
}
const char *FACTORY(RKNNFilter)::OutPutDataType() { return TYPE_ANYTHING; }

} // namespace easymedia
