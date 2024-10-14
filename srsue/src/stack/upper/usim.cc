/**
 * Copyright 2013-2023 Software Radio Systems Limited
 *
 * This file is part of srsRAN.
 *
 * srsRAN is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * srsRAN is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * A copy of the GNU Affero General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 *
 */

#include "srsue/hdr/stack/upper/usim.h"
#include "srsran/common/bcd_helpers.h"
#include "srsran/common/standard_streams.h"
// machi：引入sha256算法所在头文件
#include "srsran/common/ssl.h"
#include "srsran/common/sha256.h"
#include <cstring> // for strlen
#include <sstream>

using namespace srsran;

namespace srsue {

// 定义静态成员
uint8_t usim::n[16] = {};

usim::usim(srslog::basic_logger& logger) : usim_base(logger) {}

int usim::init(usim_args_t* args)
{
  imsi_str = args->imsi;
  imei_str = args->imei;

  const char* imsi_c = args->imsi.c_str();
  const char* imei_c = args->imei.c_str();

  auth_algo = auth_algo_milenage;
  if ("xor" == args->algo) {
    auth_algo = auth_algo_xor;
  }

  if (32 == args->k.length()) {
    str_to_hex(args->k, k);
  } else {
    logger.error("Invalid length for K: %zu should be %d", args->k.length(), 32);
    srsran::console("Invalid length for K: %zu should be %d\n", args->k.length(), 32);
  }

  if (auth_algo == auth_algo_milenage) {
    if (args->using_op) {
      if (32 == args->op.length()) {
        str_to_hex(args->op, op);
        compute_opc(k, op, opc);
      } else {
        logger.error("Invalid length for OP: %zu should be %d", args->op.length(), 32);
        srsran::console("Invalid length for OP: %zu should be %d\n", args->op.length(), 32);
      }
    } else {
      if (32 == args->opc.length()) {
        str_to_hex(args->opc, opc);
      } else {
        logger.error("Invalid length for OPc: %zu should be %d", args->opc.length(), 32);
        srsran::console("Invalid length for OPc: %zu should be %d\n", args->opc.length(), 32);
      }
    }
  }

  if (15 == args->imsi.length()) {
    imsi = 0;
    for (int i = 0; i < 15; i++) {
      imsi *= 10;
      imsi += imsi_c[i] - '0';
    }
  } else {
    logger.error("Invalid length for IMSI: %zu should be %d", args->imsi.length(), 15);
    srsran::console("Invalid length for IMSI: %zu should be %d\n", args->imsi.length(), 15);
  }

  if (15 == args->imei.length()) {
    imei = 0;
    for (int i = 0; i < 15; i++) {
      imei *= 10;
      imei += imei_c[i] - '0';
    }
  } else {
    logger.error("Invalid length for IMEI: %zu should be %d", args->imei.length(), 15);
    srsran::console("Invalid length for IMEI: %zu should be %d\n", args->imei.length(), 15);
  }

  initiated = true;

  return SRSRAN_SUCCESS;
}

void usim::stop() {}

/*******************************************************************************
  NAS interface
*******************************************************************************/

auth_result_t usim::generate_authentication_response(uint8_t* rand,
                                                     uint8_t* autn_enb,
                                                     uint16_t mcc,
                                                     uint16_t mnc,
                                                     uint8_t* res,
                                                     int*     res_len,
                                                     uint8_t* k_asme_)
{
  auth_result_t auth_result;
  uint8_t       ak_xor_sqn[6];

  if (auth_algo_xor == auth_algo) {
    auth_result = gen_auth_res_xor(rand, autn_enb, res, res_len, ak_xor_sqn);
  } else {
    auth_result = gen_auth_res_milenage(rand, autn_enb, res, res_len, ak_xor_sqn);
  }

  if (auth_result == AUTH_OK) {
    // Generate K_asme
    security_generate_k_asme(ck, ik, ak_xor_sqn, mcc, mnc, k_asme_);
  }
  return auth_result;
}

// machi：5G-AKA（通用的实现，不需要与硬件SIM卡进行交互，直接模拟SIM卡的相关操作）
auth_result_t usim::generate_authentication_response_5g(uint8_t*    rand,
                                                        uint8_t*    autn_enb,
                                                        const char* serving_network_name,
                                                        uint8_t*    abba,
                                                        uint32_t    abba_len,
                                                        uint8_t*    res_star,
                                                        uint8_t*    k_amf)
{
  auth_result_t auth_result;
  // 定义认证相关变量
  uint8_t       ak_xor_sqn[6];
  uint8_t       res[16];
  uint8_t       k_ausf[32];
  uint8_t       k_seaf[32];
  int           res_len;

  // 根据变量auth_algo来选择认证算法
  if (auth_algo_xor == auth_algo) {
    auth_result = gen_auth_res_xor(rand, autn_enb, res, &res_len, ak_xor_sqn);
  } else {
    // 一般来说使用下面这个
    auth_result = gen_auth_res_milenage(rand, autn_enb, res, &res_len, ak_xor_sqn);
  }

  // 如果认证成功
  if (auth_result == AUTH_OK) {
    // Generate RES STAR
    security_generate_res_star(ck, ik, serving_network_name, rand, res, res_len, res_star);
    logger.debug(res_star, 16, "RES STAR");
    // Generate K_ausf
    security_generate_k_ausf(ck, ik, ak_xor_sqn, serving_network_name, k_ausf);
    logger.debug(k_ausf, 32, "K AUSF");
    // Generate K_seaf
    security_generate_k_seaf(k_ausf, serving_network_name, k_seaf);
    logger.debug(k_seaf, 32, "K SEAF");
    // Generate K_seaf
    logger.debug(abba, abba_len, "ABBA:");
    logger.debug("IMSI: %s", imsi_str.c_str());
    security_generate_k_amf(k_seaf, imsi_str.c_str(), abba, abba_len, k_amf);
    logger.debug(k_amf, 32, "K AMF");
  }
  return auth_result;
}

// machi：5G-RNAKA（通用的实现，不需要与硬件SIM卡进行交互，直接模拟SIM卡的相关操作）
auth_result_t usim::generate_authentication_response_5g_new(uint8_t*    rand,
                                                            uint8_t*    autn_enb,
                                                            uint8_t*    snmac,
                                                            const char* serving_network_name,
                                                            uint8_t*    abba,
                                                            uint32_t    abba_len,
                                                            uint8_t*    res_star,
                                                            uint8_t*    k_amf)
{
  // 打印
  logger.info("function generate_authentication_response_5g_new");

  auth_result_t auth_result;
  // 定义认证相关变量
  uint8_t       res[16];
  uint8_t       k_ausf[32];
  uint8_t       k_seaf[32];
  int           res_len;

  // 根据变量auth_algo来选择认证算法
  if (auth_algo_xor == auth_algo) {
    // 先默认不实用gen_auth_res_xor方法
    auth_result = AUTH_FAILED;
//    auth_result = gen_auth_res_xor(rand, autn_enb, res, &res_len, ak_xor_sqn);
  } else {
    // 一般来说使用下面这个
    auth_result = gen_auth_res_milenage_new(rand, autn_enb, snmac, res, &res_len, serving_network_name);
  }

  // 如果认证成功
  if (auth_result == AUTH_OK) {
    // Generate RES STAR
    security_generate_res_star(ck, ik, serving_network_name, rand, res, res_len, res_star);
    logger.debug(res_star, 16, "RES STAR");
    // Generate K_ausf
    // 由于不存在ak_xor_sqn，因此需要将其替换为AUTN的前48bit
    uint8_t temp[6];
    memcpy(temp, autn_enb, 6);
    security_generate_k_ausf(ck, ik, temp, serving_network_name, k_ausf);
    logger.debug(k_ausf, 32, "K AUSF");
    // Generate K_seaf
    security_generate_k_seaf(k_ausf, serving_network_name, k_seaf);
    logger.debug(k_seaf, 32, "K SEAF");
    // Generate K_seaf
    logger.debug(abba, abba_len, "ABBA:");
    logger.debug("IMSI: %s", imsi_str.c_str());
    security_generate_k_amf(k_seaf, imsi_str.c_str(), abba, abba_len, k_amf);
    logger.debug(k_amf, 32, "K AMF");
  }
  return auth_result;
}

/*******************************************************************************
  Helpers
*******************************************************************************/

// machi：认证算法-auth_algo_milenage（好像只验证了MAC对不对，并没有验证SQN对不对）
auth_result_t usim::gen_auth_res_milenage(uint8_t* rand, uint8_t* autn_enb, uint8_t* res, int* res_len, uint8_t* ak_xor_sqn)
{
  auth_result_t result = AUTH_OK;
  uint32_t      i;
  uint8_t       sqn[6];

  // 调用f2345函数进行计算
  // Use RAND and K to compute RES, CK, IK and AK
  security_milenage_f2345(k, opc, rand, res, ck, ik, ak);

  *res_len = 8;

  // 获取SQNHN
  // Extract sqn from autn
  for (i = 0; i < 6; i++) {
    sqn[i] = autn_enb[i] ^ ak[i];
  }
  // Extract AMF from autn
  for (i = 0; i < 2; i++) {
    amf[i] = autn_enb[6 + i];
  }

  // 调用f1函数来计算MAC
  // Generate MAC
  security_milenage_f1(k, opc, rand, sqn, amf, mac);

  // 这里的逻辑和协议本身有点不对，但是也是可以的。按照协议来说直接比较两个MAC就行，他这里是根据上面计算得到的重新构建了一个AUTN，然后比较的是两个AUTN的值
  // Construct AUTN
  for (i = 0; i < 6; i++) {
    autn[i] = sqn[i] ^ ak[i];
  }
  for (i = 0; i < 2; i++) {
    autn[6 + i] = amf[i];
  }
  for (i = 0; i < 8; i++) {
    autn[8 + i] = mac[i];
  }

  // 与比较MAC的作用相同
  // Compare AUTNs
  for (i = 0; i < 16; i++) {
    if (autn[i] != autn_enb[i]) {
      result = AUTH_FAILED;
    }
  }

  // 计算ak异或SNQ
  for (i = 0; i < 6; i++) {
    ak_xor_sqn[i] = sqn[i] ^ ak[i];
  }

  logger.debug(ck, CK_LEN, "CK:");
  logger.debug(ik, IK_LEN, "IK:");
  logger.debug(ak, AK_LEN, "AK:");
  logger.debug(sqn, 6, "sqn:");
  logger.debug(amf, 2, "amf:");
  logger.debug(mac, 8, "mac:");

  return result;
}

// machi：5G-RNAKA的认证算法-auth_algo_milenage（好像只验证了MAC对不对，并没有验证SQN对不对）
auth_result_t usim::gen_auth_res_milenage_new(uint8_t* rand, uint8_t* autn_enb, uint8_t* snmac, uint8_t* res, int* res_len, const char* serving_network_name)
{
  // 打印
  logger.info("function gen_auth_res_milenage_new");

  auth_result_t result = AUTH_OK;
  uint32_t      i;

  // 调用f2345函数进行计算（使用新的函数）
  // Use RAND and K to compute RES, CK, IK and AK
  security_milenage_f2345_new(k, opc, rand, res, ck, ik, ak_new);

  *res_len = 8;

  // 调用f1函数来计算MAC（使用新的函数）
  // Generate MAC
  security_milenage_f1_new(k, opc, rand, mac);

  // 调用f1star函数来计算AK
  security_milenage_f1_star_new(k, opc, rand, ak_new);

  // 这里的逻辑和协议本身有点不对，但是也是可以的。按照协议来说直接比较两个MAC就行，他这里是根据上面计算得到的重新构建了一个AUTN，然后比较的是两个AUTN的值
  // AUTN变为HNMAC xor AK
  // Construct AUTN
  for (i = 0; i < 8; i++) {
    autn_new[i] = mac[i] ^ ak_new[i];
  }
//  for (i = 0; i < 2; i++) {
//    autn[6 + i] = amf[i];
//  }
//  for (i = 0; i < 8; i++) {
//    autn[8 + i] = mac[i];
//  }

  // 与比较MAC的作用相同
  // AUTN变为64bit
  // Compare AUTNs
  for (i = 0; i < 8; i++) {
    if (autn_new[i] != autn_enb[i]) {
      result = AUTH_FAILED;
    }
  }

  // 计算ak异或SNQ
//  for (i = 0; i < 6; i++) {
//    ak_xor_sqn[i] = sqn[i] ^ ak[i];
//  }

  uint8_t snName[32] = {
      0x35, 0x47, 0x3a, 0x6d, 0x6e, 0x63, 0x30, 0x39,
      0x33, 0x2e, 0x6d, 0x63, 0x63, 0x32, 0x30, 0x38,
      0x2e, 0x33, 0x67, 0x70, 0x70, 0x6e, 0x65, 0x74,
      0x77, 0x6f, 0x72, 0x6b, 0x2e, 0x6f, 0x72, 0x67
  };

  std::vector<uint8_t> out(32);

  // 计算SNMAC
  uint8_t output[32];
  size_t total_len = 16 + 8 + 32;
  uint8_t input[total_len];
  size_t offset = 0;
  memcpy(input + offset, n, 16);
  offset += 16;
  memcpy(input + offset, mac, 8);
  offset += 8;
  memcpy(input + offset, snName, 32);
  sha256_hash(out.data(), input, total_len);

  for (i = 0; i < 8; i++) {
    xsnmac[i] = output[i + 24];
  }

  // 比较SNMAC
  for (i = 0; i < 8; i++) {
    if(xsnmac[i] != snmac[i]) {
      result = AUTH_FAILED;
    }
  }

  logger.debug(ck, CK_LEN, "CK:");
  logger.debug(ck, CK_LEN, "CK:");
  logger.debug(ik, IK_LEN, "IK:");
  logger.debug(ak_new, AK_LEN_NEW, "AK:");
//  logger.debug(sqn, 6, "sqn:");
//  logger.debug(amf, 2, "amf:");
  logger.debug(snmac, 8, "SNMAC:");
  logger.debug(xsnmac, 8, "XSNMAC:");
  logger.debug(autn_enb, 8, "AUTN_enb:");
  logger.debug(autn_new, 8, "AUTN:");
  logger.debug(mac, 8, "mac:");
  logger.debug(n, 16, "N:");
  logger.debug(snName, 32, "SNMAC: ");

  return result;
}

// machi: 认证算法-auth_algo_xor
// 3GPP TS 34.108 version 10.0.0 Section 8
auth_result_t usim::gen_auth_res_xor(uint8_t* rand, uint8_t* autn_enb, uint8_t* res, int* res_len, uint8_t* ak_xor_sqn)
{
  auth_result_t result = AUTH_OK;
  uint8_t       sqn[6];
  uint8_t       res_[16];

  logger.debug(k, 16, "K:");

  // Use RAND and K to compute RES, CK, IK and AK
  security_xor_f2345(k, rand, res_, ck, ik, ak);

  for (uint32_t i = 0; i < 8; i++) {
    res[i] = res_[i];
  }

  *res_len = 8;

  // Extract sqn from autn
  for (uint32_t i = 0; i < 6; i++) {
    sqn[i] = autn_enb[i] ^ ak[i];
  }
  // Extract AMF from autn
  for (uint32_t i = 0; i < 2; i++) {
    amf[i] = autn_enb[6 + i];
  }

  // Generate MAC
  security_xor_f1(k, rand, sqn, amf, mac);

  // Construct AUTN
  for (uint32_t i = 0; i < 6; i++) {
    autn[i] = sqn[i] ^ ak[i];
  }
  for (uint32_t i = 0; i < 2; i++) {
    autn[6 + i] = amf[i];
  }
  for (uint32_t i = 0; i < 8; i++) {
    autn[8 + i] = mac[i];
  }

  // Compare AUTNs
  for (uint32_t i = 0; i < 16; i++) {
    if (autn[i] != autn_enb[i]) {
      result = AUTH_FAILED;
    }
  }

  logger.debug(ck, CK_LEN, "CK:");
  logger.debug(ik, IK_LEN, "IK:");
  logger.debug(ak, AK_LEN, "AK:");
  logger.debug(sqn, 6, "sqn:");
  logger.debug(amf, 2, "amf:");
  logger.debug(mac, 8, "mac:");

  for (uint32_t i = 0; i < 6; i++) {
    ak_xor_sqn[i] = sqn[i] ^ ak[i];
  }

  return result;
}

std::string usim::get_mnc_str(const uint8_t* imsi_vec, std::string mcc_str)
{
  uint32_t           mcc_len = 3;
  uint32_t           mnc_len = 2;
  std::ostringstream mnc_oss;

  // US MCC uses 3 MNC digits
  if (!mcc_str.compare("310") || !mcc_str.compare("311") || !mcc_str.compare("312") || !mcc_str.compare("313") ||
      !mcc_str.compare("316")) {
    mnc_len = 3;
  }

  for (uint32_t i = mcc_len; i < mcc_len + mnc_len; i++) {
    mnc_oss << (int)imsi_vec[i];
  }

  return mnc_oss.str();
}

void usim::str_to_hex(std::string str, uint8_t* hex)
{
  uint32_t    i;
  const char* h_str = str.c_str();
  uint32_t    len   = str.length();

  for (i = 0; i < len / 2; i++) {
    if (h_str[i * 2 + 0] >= '0' && h_str[i * 2 + 0] <= '9') {
      hex[i] = (h_str[i * 2 + 0] - '0') << 4;
    } else if (h_str[i * 2 + 0] >= 'A' && h_str[i * 2 + 0] <= 'F') {
      hex[i] = ((h_str[i * 2 + 0] - 'A') + 0xA) << 4;
    } else {
      hex[i] = ((h_str[i * 2 + 0] - 'a') + 0xA) << 4;
    }

    if (h_str[i * 2 + 1] >= '0' && h_str[i * 2 + 1] <= '9') {
      hex[i] |= h_str[i * 2 + 1] - '0';
    } else if (h_str[i * 2 + 1] >= 'A' && h_str[i * 2 + 1] <= 'F') {
      hex[i] |= (h_str[i * 2 + 1] - 'A') + 0xA;
    } else {
      hex[i] |= (h_str[i * 2 + 1] - 'a') + 0xA;
    }
  }
}

} // namespace srsue
