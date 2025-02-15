#include "bpforc.h"
#include "bpftrace.h"
#include "clang_parser.h"
#include "codegen_llvm.h"
#include "driver.h"
#include "fake_map.h"
#include "field_analyser.h"
#include "mocks.h"
#include "semantic_analyser.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace bpftrace {
namespace test {
namespace probe {

using bpftrace::ast::AttachPoint;
using bpftrace::ast::AttachPointList;
using bpftrace::ast::Probe;

void gen_bytecode(const std::string &input, std::stringstream &out)
{
  auto bpftrace = get_mock_bpftrace();
  Driver driver(*bpftrace);
  FakeMap::next_mapfd_ = 1;

  ASSERT_EQ(driver.parse_str(input), 0);

  ast::FieldAnalyser fields(driver.root_.get(), *bpftrace);
  EXPECT_EQ(fields.analyse(), 0);

  ClangParser clang;
  clang.parse(driver.root_.get(), *bpftrace);

  MockBPFfeature feature;
  ast::SemanticAnalyser semantics(driver.root_.get(), *bpftrace, feature);
  ASSERT_EQ(semantics.analyse(), 0);
  ASSERT_EQ(semantics.create_maps(true), 0);

  ast::CodegenLLVM codegen(driver.root_.get(), *bpftrace);
  codegen.generate_ir();
  codegen.DumpIR(out);
}

void compare_bytecode(const std::string &input1, const std::string &input2)
{
  std::stringstream expected_output1;
  std::stringstream expected_output2;

  gen_bytecode(input1, expected_output1);
  gen_bytecode(input2, expected_output2);

  EXPECT_EQ(expected_output1.str(), expected_output2.str());
}

TEST(probe, short_name)
{
  compare_bytecode("tracepoint:a:b { args }", "t:a:b { args }");
  compare_bytecode("kprobe:f { pid }", "k:f { pid }");
  compare_bytecode("kretprobe:f { pid }", "kr:f { pid }");
  compare_bytecode("uprobe:sh:f { 1 }", "u:sh:f { 1 }");
  compare_bytecode("profile:hz:997 { 1 }", "p:hz:997 { 1 }");
  compare_bytecode("hardware:cache-references:1000000 { 1 }", "h:cache-references:1000000 { 1 }");
  compare_bytecode("software:faults:1000 { 1 }", "s:faults:1000 { 1 }");
  compare_bytecode("interval:s:1 { 1 }", "i:s:1 { 1 }");
}

#ifdef HAVE_LIBBPF_BTF_DUMP

#include "btf_common.h"

class probe_btf : public test_btf
{
};

TEST_F(probe_btf, short_name)
{
  compare_bytecode("kfunc:func_1 { 1 }", "f:func_1 { 1 }");
  compare_bytecode("kretfunc:func_1 { 1 }", "fr:func_1 { 1 }");
}

#endif // HAVE_LIBBPF_BTF_DUMP

} // namespace probe
} // namespace test
} // namespace bpftrace
