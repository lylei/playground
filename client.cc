#include <pthread.h>
#include <vector>
#include <iostream>
#include <sofa/pbrpc/pbrpc.h>
#include "hello.pb.h"

int count = 0;

struct AskParam {
  sofa::pbrpc::Liyuan::ServiceMachine_Stub* stub;
  sofa::pbrpc::RpcController* cntl;
  sofa::pbrpc::Liyuan::QARequest* request;
  sofa::pbrpc::Liyuan::QAResponse* response;
  google::protobuf::Closure* done;
};

void QuestionCallback(sofa::pbrpc::RpcController* cntl,
                      sofa::pbrpc::Liyuan::QARequest* request,
                      sofa::pbrpc::Liyuan::QAResponse* response,
                      pthread_mutex_t* lock) {
  if (cntl->Failed()) {
    SLOG(ERROR, "rpc failed: %s", cntl->ErrorText().c_str());
  }
  pthread_mutex_lock(lock);
  ++count;
  pthread_mutex_unlock(lock);
  SLOG(INFO, "count: %d", count);

  delete cntl;
  delete request;
  delete response;
}

void *DoAsk(void* args) {
  AskParam* parm = static_cast<AskParam*>(args);
  for (int i = 0; i < 100; ++i) {
    (parm->stub)->AskQuestion(parm->cntl, parm->request, parm->response, parm->done);
  }
}

int main() {
  // rpc init
  SOFA_PBRPC_SET_LOG_LEVEL(NOTICE);

  sofa::pbrpc::RpcClientOptions options;
  sofa::pbrpc::RpcClient rpc_client(options);

  sofa::pbrpc::RpcChannel rpc_channel(&rpc_client, "st01-spi-session0.st01.baidu.com:11220");
  sofa::pbrpc::Liyuan::ServiceMachine_Stub stub(&rpc_channel);

  sofa::pbrpc::Liyuan::QARequest* request = new sofa::pbrpc::Liyuan::QARequest();
  request->set_name("liyuan");
  request->set_question("what time is it?");
  sofa::pbrpc::Liyuan::QAResponse* response = new sofa::pbrpc::Liyuan::QAResponse();

  sofa::pbrpc::RpcController* cntl = new sofa::pbrpc::RpcController();
  cntl->SetTimeout(3000);

  pthread_mutex_t lock;
  pthread_mutex_init(&lock, NULL);
  google::protobuf::Closure* done = sofa::pbrpc::NewClosure(
          &QuestionCallback, cntl, request, response, &lock);

  // dispatch tasks
  AskParam param = {&stub, cntl, request, response, done};
  int thread_num = 10;
  std::vector<pthread_t> threads;

  for (int i = 0; i < thread_num; ++i) {
    pthread_t ntid;
    int err = pthread_create(&ntid, NULL, DoAsk, static_cast<void*>(&param));
    if (err != 0) {
      SLOG(ERROR, "create thread failed: %s", strerror(err));
    } else {
      SLOG(INFO, "create thread #%d", i);
      threads.push_back(ntid);
    }
  }

  std::cout << "finish generate threads" << std::endl;
  for (int i = 0; i < thread_num; ++i) {
    pthread_join(threads[i], NULL);
    std::cout << "join thread #" << i << std::endl;
  }

  sleep(2);
  return EXIT_SUCCESS;
}
