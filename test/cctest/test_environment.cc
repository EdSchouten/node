#include "node_internals.h"
#include "libplatform/libplatform.h"

#include <string>
#include "gtest/gtest.h"
#include "node_test_fixture.h"

using node::Environment;
using node::IsolateData;
using node::CreateIsolateData;
using node::FreeIsolateData;
using node::CreateEnvironment;
using node::FreeEnvironment;
using node::AtExit;
using node::RunAtExit;
using node::ProcessArguments;

static bool called_cb_1 = false;
static bool called_cb_2 = false;
static void at_exit_callback1(void* arg);
static void at_exit_callback2(void* arg);
static std::string cb_1_arg;  // NOLINT(runtime/string)

class EnvironmentTest : public NodeTestFixture {
 public:
  class Env {
   public:
    Env(const v8::HandleScope& handle_scope,
        v8::Isolate* isolate,
        const Argv& argv,
        NodeTestFixture* test_fixture) {
      context_ = v8::Context::New(isolate);
      CHECK(!context_.IsEmpty());
      isolate_data_ = CreateIsolateData(isolate,
                                        NodeTestFixture::CurrentLoop(),
                                        test_fixture->Platform());
      CHECK_NE(nullptr, isolate_data_);
      ProcessArguments process_arguments;
      process_arguments.argc = 1;
      process_arguments.argv = *argv;
      process_arguments.exec_argv.insert(process_arguments.exec_argv.end(),
                                         *argv, *argv + argv.nr_args());
      environment_ = CreateEnvironment(isolate_data_, context_,
                                       process_arguments);
      CHECK_NE(nullptr, environment_);
    }

    ~Env() {
      environment_->CleanupHandles();
      FreeEnvironment(environment_);
      FreeIsolateData(isolate_data_);
    }

    Environment* operator*() const {
      return environment_;
    }

   private:
    v8::Local<v8::Context> context_;
    IsolateData* isolate_data_;
    Environment* environment_;
  };

 private:
  virtual void TearDown() {
    NodeTestFixture::TearDown();
    called_cb_1 = false;
    called_cb_2 = false;
  }
};

TEST_F(EnvironmentTest, AtExitWithEnvironment) {
  const v8::HandleScope handle_scope(isolate_);
  const Argv argv;
  Env env {handle_scope, isolate_, argv, this};

  AtExit(*env, at_exit_callback1);
  RunAtExit(*env);
  EXPECT_TRUE(called_cb_1);
}

TEST_F(EnvironmentTest, AtExitWithArgument) {
  const v8::HandleScope handle_scope(isolate_);
  const Argv argv;
  Env env {handle_scope, isolate_, argv, this};

  std::string arg{"some args"};
  AtExit(*env, at_exit_callback1, static_cast<void*>(&arg));
  RunAtExit(*env);
  EXPECT_EQ(arg, cb_1_arg);
}

TEST_F(EnvironmentTest, MultipleEnvironmentsPerIsolate) {
  const v8::HandleScope handle_scope(isolate_);
  const Argv argv;
  Env env1 {handle_scope, isolate_, argv, this};
  Env env2 {handle_scope, isolate_, argv, this};

  AtExit(*env1, at_exit_callback1);
  AtExit(*env2, at_exit_callback2);
  RunAtExit(*env1);
  EXPECT_TRUE(called_cb_1);
  EXPECT_FALSE(called_cb_2);

  RunAtExit(*env2);
  EXPECT_TRUE(called_cb_2);
}

static void at_exit_callback1(void* arg) {
  called_cb_1 = true;
  if (arg) {
    cb_1_arg = *static_cast<std::string*>(arg);
  }
}

static void at_exit_callback2(void* arg) {
  called_cb_2 = true;
}
