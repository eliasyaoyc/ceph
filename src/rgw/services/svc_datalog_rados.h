// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab ft=cpp

/*
 * Ceph - scalable distributed file system
 *
 * Copyright (C) 2019 Red Hat, Inc.
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software
 * Foundation. See file COPYING.
 *
 */


#pragma once

#include "rgw/rgw_service.h"


class RGWDataChangesLog;
class RGWDataChangesLogInfo;
struct RGWDataChangesLogMarker;
struct rgw_data_change_log_entry;

namespace rgw {
  class BucketChangeObserver;
}

class RGWSI_DataLog_RADOS : public RGWServiceInstance
{
  std::unique_ptr<RGWDataChangesLog> log;

public:
  RGWSI_DataLog_RADOS(CephContext *cct);
  virtual ~RGWSI_DataLog_RADOS();

  struct Svc {
    RGWSI_Zone *zone{nullptr};
    RGWSI_Cls *cls{nullptr};
  } svc;

  int init(RGWSI_Zone *_zone_svc,
           RGWSI_Cls *_cls_svc);

  int do_start() override;
  void shutdown() override;

  RGWDataChangesLog *get_log() {
    return log.get();
  }

  void set_observer(rgw::BucketChangeObserver *observer);

  int get_log_shard_id(rgw_bucket& bucket, int shard_id);
  std::string get_oid(int shard_id) const;

  int get_info(int shard_id, RGWDataChangesLogInfo *info);

  int add_entry(const RGWBucketInfo& bucket_info, int shard_id);
  int list_entries(int shard, int max_entries,
		   std::vector<rgw_data_change_log_entry>& entries,
		   std::optional<std::string_view> marker,
		   std::string* out_marker, bool* truncated);
  int list_entries(int max_entries,
		   std::vector<rgw_data_change_log_entry>& entries,
		   RGWDataChangesLogMarker& marker, bool *ptruncated);
  int trim_entries(int shard_id, std::string_view marker);
};
