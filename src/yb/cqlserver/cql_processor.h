//--------------------------------------------------------------------------------------------------
// Copyright (c) YugaByte, Inc.
//
// This module is to define CQL processor. Each processor will be handling one and only one request
// at a time. As a result all processing code in this module and the modules that it is calling
// does not need to be thread safe.
//--------------------------------------------------------------------------------------------------

#ifndef YB_CQLSERVER_CQL_PROCESSOR_H_
#define YB_CQLSERVER_CQL_PROCESSOR_H_

#include "yb/client/client.h"
#include "yb/cqlserver/cql_message.h"
#include "yb/cqlserver/cql_statement.h"
#include "yb/sql/sql_processor.h"
#include "yb/sql/statement.h"
#include "yb/rpc/cql_rpc.h"

namespace yb {
namespace cqlserver {

class CQLServiceImpl;

class CQLMetrics : public sql::SqlMetrics {
 public:
  explicit CQLMetrics(const scoped_refptr<yb::MetricEntity>& metric_entity);

  scoped_refptr<yb::Histogram> time_to_process_request_;
  scoped_refptr<yb::Histogram> time_to_get_cql_processor_;
  scoped_refptr<yb::Histogram> time_to_parse_cql_wrapper_;
  scoped_refptr<yb::Histogram> time_to_execute_cql_request_;

  scoped_refptr<yb::Histogram> time_to_queue_cql_response_;
  scoped_refptr<yb::Counter> num_errors_parsing_cql_;
  // Rpc level metrics
  yb::rpc::RpcMethodMetrics rpc_method_metrics_;
};


// A list of CQL processors and position in the list.
class CQLProcessor;
using CQLProcessorList = std::list<std::unique_ptr<CQLProcessor>>;
using CQLProcessorListPos = CQLProcessorList::iterator;

class CQLProcessor : public sql::SqlProcessor {
 public:
  // Constructor and destructor.
  explicit CQLProcessor(CQLServiceImpl* service_impl, const CQLProcessorListPos& pos);
  ~CQLProcessor();

  // Processing an inbound call.
  void ProcessCall(rpc::InboundCallPtr cql_call);

  // Process a PREPARE request.
  CQLResponse *ProcessPrepare(const PrepareRequest& req);

  // Process a EXECUTE request.
  void ProcessExecute(const ExecuteRequest& req, Callback<void(CQLResponse*)> cb);

  // Process a QUERY request.
  void ProcessQuery(const QueryRequest& req, Callback<void(CQLResponse*)> cb);

  // Process a BATCH request.
  void ProcessBatch(const BatchRequest& req, Callback<void(CQLResponse*)> cb, int idx = 0);

 private:
  // Run in response to ProcessExecute, PrepareQuery, ProcessBatch and ProcessCall
  void ProcessExecuteDone(
      const ExecuteRequest& req, std::shared_ptr<CQLStatement> stmt,
      Callback<void(CQLResponse*)> cb, const Status& s,
      const sql::ExecutedResult::SharedPtr& result);
  void ProcessQueryDone(
      const QueryRequest& req, Callback<void(CQLResponse*)> cb, const Status& s,
      const sql::ExecutedResult::SharedPtr& result);
  void ProcessBatchDone(
      const BatchRequest& req, int idx, std::shared_ptr<CQLStatement> stmt,
      Callback<void(CQLResponse*)> cb, const Status& s,
      const sql::ExecutedResult::SharedPtr& result);
  void ProcessCallDone(
      rpc::InboundCallPtr call, const CQLRequest* request, const MonoTime& start,
      CQLResponse* response);

  CQLResponse* ReturnResponse(
      const CQLRequest& req, std::shared_ptr<CQLStatement> stmt, Status s,
      const sql::ExecutedResult::SharedPtr& result);
  void SendResponse(rpc::InboundCallPtr call, CQLResponse* response);

  // Pointer to the containing CQL service implementation.
  CQLServiceImpl* const service_impl_;

  // CQL metrics.
  std::shared_ptr<CQLMetrics> cql_metrics_;

  // Position in the CQL processor list.
  const CQLProcessorListPos pos_;
};

}  // namespace cqlserver
}  // namespace yb

#endif  // YB_CQLSERVER_CQL_PROCESSOR_H_
