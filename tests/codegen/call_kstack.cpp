#include "common.h"

namespace bpftrace {
namespace test {
namespace codegen {

TEST(codegen, call_kstack)
{
  auto result = NAME;

  // Mode doesn't directly affect codegen, so the programs below should
  // generate the same IR
  test("kprobe:f { @x = kstack(); @y = kstack(6) }", result);
  test("kprobe:f { @x = kstack(perf); @y = kstack(perf, 6) }", result);
  test("kprobe:f { @x = kstack(perf); @y = kstack(bpftrace) }", result);
}

TEST(codegen, call_kstack_mapids)
{
  BPFtrace bpftrace;
  Driver driver(bpftrace);
  FakeMap::next_mapfd_ = 1;

  ASSERT_EQ(driver.parse_str(
                "kprobe:f { @x = kstack(5); @y = kstack(6); @z = kstack(6) }"),
            0);

  ClangParser clang;
  clang.parse(driver.root_.get(), bpftrace);

  MockBPFfeature feature;
  ast::SemanticAnalyser semantics(driver.root_.get(), bpftrace, feature);
  ASSERT_EQ(semantics.analyse(), 0);
  ASSERT_EQ(semantics.create_maps(true), 0);

  ast::CodegenLLVM codegen(driver.root_.get(), bpftrace);
  codegen.compile();

  ASSERT_EQ(FakeMap::next_mapfd_, 7);
  ASSERT_EQ(bpftrace.maps.CountStackTypes(), 2U);

  StackType stack_type;
  stack_type.limit = 5;
  ASSERT_TRUE(bpftrace.maps.Has(stack_type));
  stack_type.limit = 6;
  ASSERT_TRUE(bpftrace.maps.Has(stack_type));
}

TEST(codegen, call_kstack_modes_mapids)
{
  BPFtrace bpftrace;
  Driver driver(bpftrace);
  FakeMap::next_mapfd_ = 1;

  ASSERT_EQ(driver.parse_str("kprobe:f { @x = kstack(perf); @y = "
                             "kstack(bpftrace); @z = kstack() }"),
            0);

  ClangParser clang;
  clang.parse(driver.root_.get(), bpftrace);

  MockBPFfeature feature;
  ast::SemanticAnalyser semantics(driver.root_.get(), bpftrace, feature);
  ASSERT_EQ(semantics.analyse(), 0);
  ASSERT_EQ(semantics.create_maps(true), 0);

  ast::CodegenLLVM codegen(driver.root_.get(), bpftrace);
  codegen.compile();

  ASSERT_EQ(FakeMap::next_mapfd_, 7);
  ASSERT_EQ(bpftrace.maps.CountStackTypes(), 2U);

  StackType stack_type;
  stack_type.mode = StackMode::perf;
  ASSERT_TRUE(bpftrace.maps.Has(stack_type));
  stack_type.mode = StackMode::bpftrace;
  ASSERT_TRUE(bpftrace.maps.Has(stack_type));
}

} // namespace codegen
} // namespace test
} // namespace bpftrace
