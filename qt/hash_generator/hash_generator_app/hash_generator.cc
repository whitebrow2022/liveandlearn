// Created by liangxu on 2022/11/24.
//
// Copyright (c) 2022 The HashGenerator Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "hash_generator.h"

#include <openssl/md5.h>

#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <memory>

#include "hash_generator_base/log/log_writer.h"
#include "openssl/sha.h"

namespace {

// hash sha256
void Sha256HashString(unsigned char hash[SHA256_DIGEST_LENGTH],
                      char output_buffer[65]) {
  for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
    sprintf(output_buffer + (i * 2), "%02x", hash[i]);  // NOLINT
  }
  output_buffer[64] = 0;
}
bool Sha256File(const char *path, char output_buffer[65]) {
  FILE *file = fopen(path, "rb");
  if (!file) {
    return false;
  }
  unsigned char hash[SHA256_DIGEST_LENGTH];
  SHA256_CTX sha256;
  SHA256_Init(&sha256);
  constexpr const int kBufSize = 32768;
  std::unique_ptr<unsigned char[]> unique_buff =
      std::make_unique<unsigned char[]>(kBufSize);
  unsigned char *buffer = unique_buff.get();
  int bytesRead = 0;
  if (!buffer) {
    return false;
  }
  while ((bytesRead = fread(buffer, 1, kBufSize, file))) {
    SHA256_Update(&sha256, buffer, bytesRead);
  }
  SHA256_Final(hash, &sha256);

  Sha256HashString(hash, output_buffer);
  fclose(file);
  return true;
}

// hash md5
void Md5HashString(unsigned char hash[MD5_DIGEST_LENGTH],
                   char output_buffer[33]) {
  for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
    sprintf(output_buffer + (i * 2), "%02x", hash[i]);  // NOLINT
  }
  output_buffer[32] = 0;
}
bool Md5File(const char *path, char output_buffer[33]) {
  FILE *file = fopen(path, "rb");
  if (!file) {
    return false;
  }
  unsigned char hash[MD5_DIGEST_LENGTH];
  MD5_CTX md5;
  MD5_Init(&md5);
  constexpr const int kBufSize = 32768;
  std::unique_ptr<unsigned char[]> unique_buff =
      std::make_unique<unsigned char[]>(kBufSize);
  unsigned char *buffer = unique_buff.get();
  int bytesRead = 0;
  if (!buffer) {
    return false;
  }
  while ((bytesRead = fread(buffer, 1, kBufSize, file))) {
    MD5_Update(&md5, buffer, bytesRead);
  }
  MD5_Final(hash, &md5);
  Md5HashString(hash, output_buffer);
  fclose(file);
  return true;
}
}  // namespace

namespace {
bool QtSha256File(const char *path, char output_buffer[65]) {
  FILE *file = fopen(path, "rb");
  if (!file) {
    return false;
  }
  constexpr const int kBufSize = 32768;
  std::unique_ptr<char[]> unique_buff = std::make_unique<char[]>(kBufSize);
  char *buffer = unique_buff.get();
  int bytesRead = 0;
  if (!buffer) {
    return false;
  }
  QCryptographicHash cryp_hash(QCryptographicHash::Sha256);
  while ((bytesRead = fread(buffer, 1, kBufSize, file))) {
    cryp_hash.addData(buffer, bytesRead);
  }
  auto hash = cryp_hash.result().toHex().toStdString();
  memset(output_buffer, 0, 65);
  strncpy(output_buffer, hash.c_str(), 64);
  fclose(file);
  return true;
}
}  // namespace

void HashWorker::DoWork(const QString &file_path) {
  QString result;

  // calc hash sha256
  {
    // ssl calc hash
    log_info << "ssl hash_generator_app hash (" << file_path.toStdString()
             << ") start:";
    auto time_start = std::chrono::high_resolution_clock::now();
    char hash_buf[65] = {0};
    if (Sha256File(file_path.toStdString().c_str(), hash_buf)) {
      result = QString(hash_buf);
      log_info << "ssl hash_generator_app hash (" << file_path.toStdString()
               << ") hash: " << hash_buf;
    }
    auto time_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = time_end - time_start;
    log_info << "ssl hash_generator_app hash (" << file_path.toStdString()
             << ") end, spend time:" << static_cast<int>(elapsed.count())
             << "ms";
  }
  {
    // qt calc hash
    QCryptographicHash cryp_hash(QCryptographicHash::Sha256);
    log_info << "qt hash_generator_app hash (" << file_path.toStdString()
             << ") start:";
    auto time_start = std::chrono::high_resolution_clock::now();
    QFile qfile(file_path);
    if (qfile.open(QFile::ReadOnly)) {
      if (cryp_hash.addData(&qfile)) {
        auto hash = cryp_hash.result().toHex().toStdString();
        result = QString(hash.c_str());
        log_info << "qt hash_generator_app hash (" << file_path.toStdString()
                 << ") hash: " << hash;
      }
    }
    auto time_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = time_end - time_start;
    log_info << "qt hash_generator_app hash (" << file_path.toStdString()
             << ") end, spend time:" << static_cast<int>(elapsed.count())
             << "ms";
  }
#if 0
  {
    // qt2 calc hash
    auto time_start = std::chrono::high_resolution_clock::now();
    log_info << "qt2 hash_generator_app hash (" << file_path.toStdString()
             << ") start:";
    char hash_buf[65] = {0};
    if (QtSha256File(file_path.toStdString().c_str(), hash_buf)) {
      result = QString(hash_buf);
      log_info << "qt2 hash_generator_app hash (" << file_path.toStdString()
               << ") hash: " << hash_buf;
    }
    auto time_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = time_end - time_start;
    log_info << "qt2 hash_generator_app hash (" << file_path.toStdString()
             << ") end, spend time:" << static_cast<int>(elapsed.count())
             << "ms";
  }
#endif

  {
    // ssl calc md5
    log_info << "ssl hash_generator_app md5 (" << file_path.toStdString()
             << ") start:";
    auto time_start = std::chrono::high_resolution_clock::now();
    char hash_buf[33] = {0};
    if (Md5File(file_path.toStdString().c_str(), hash_buf)) {
      result = QString(hash_buf);
      log_info << "ssl hash_generator_app md5 (" << file_path.toStdString()
               << ") hash: " << hash_buf;
    }
    auto time_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = time_end - time_start;
    log_info << "ssl hash_generator_app md5 (" << file_path.toStdString()
             << ") end, spend time:" << static_cast<int>(elapsed.count())
             << "ms";
  }
  {
    // qt calc md5
    QCryptographicHash cryp_hash(QCryptographicHash::Md5);
    log_info << "qt hash_generator_app md5 (" << file_path.toStdString()
             << ") start:";
    auto time_start = std::chrono::high_resolution_clock::now();
    QFile qfile(file_path);
    if (qfile.open(QFile::ReadOnly)) {
      if (cryp_hash.addData(&qfile)) {
        auto hash = cryp_hash.result().toHex().toStdString();
        result = QString(hash.c_str());
        log_info << "qt hash_generator_app md5 (" << file_path.toStdString()
                 << ") hash: " << hash;
      }
    }
    auto time_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = time_end - time_start;
    log_info << "qt hash_generator_app md5 (" << file_path.toStdString()
             << ") end, spend time:" << static_cast<int>(elapsed.count())
             << "ms";
  }
  emit HashReady(result);
}

HashGenerator::HashGenerator() {
  HashWorker *worker = new HashWorker;
  worker->moveToThread(&worker_);
  connect(&worker_, &QThread::finished, worker, &QObject::deleteLater);
  connect(this, &HashGenerator::Operate, worker, &HashWorker::DoWork);
  connect(worker, &HashWorker::HashReady, this, &HashGenerator::HandleHashCode);
  worker_.start();
}
HashGenerator::~HashGenerator() {
  worker_.quit();
  worker_.wait();
}

void HashGenerator::HandleHashCode(const QString &hash_code) {
  log_info << "hash code: " << hash_code.toStdString();
}
