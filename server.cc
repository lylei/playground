#include <sofa/pbrpc/pbrpc.h>
#include "hello.pb.h"

class ServiceMachineImpl : public sofa::pbrpc::Liyuan::ServiceMachine {
public:
  ServiceMachineImpl() {}
  virtual ~ServiceMachineImpl() {}

private:
  virtual void AskQuestion(google::protobuf::RpcController* controller,
                      const sofa::pbrpc::Liyuan::QARequest* request,
                      sofa::pbrpc::Liyuan::QAResponse* response,
                      google::protobuf::Closure* done) {
    sofa::pbrpc::RpcController* cntl =
            static_cast<sofa::pbrpc::RpcController*>(controller);
    SLOG(NOTICE, "AskQuestion(): request message from %s", cntl->RemoteAddress().c_str());
    response->set_answer("hello " + request->name() + ", I don't know the answer to '" +
                         request->question() + "'");
    done->Run();
  }
};

int main() {
  SOFA_PBRPC_SET_LOG_LEVEL(NOTICE);

  sofa::pbrpc::RpcServerOptions options;
  options.work_thread_num = 4;
  sofa::pbrpc::RpcServer rpc_server(options);

  if (!rpc_server.Start("st01-spi-session0.st01.baidu.com:11220")) {
      SLOG(ERROR, "start server failed");
      return EXIT_FAILURE;
  }

  sofa::pbrpc::Liyuan::ServiceMachine* answer_service = new ServiceMachineImpl();
  if (!rpc_server.RegisterService(answer_service)) {
      SLOG(ERROR, "register service failed");
      return EXIT_FAILURE;
  }

  rpc_server.Run();
  rpc_server.Stop();
  return EXIT_SUCCESS;
}
